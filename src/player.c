/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.c
 * PURPOSE     : SG MPFC. Main player functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 22.09.2004
 * NOTE        : Module prefix 'player'.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "browser.h"
#include "cfg.h"
#include "colors.h"
#include "eqwnd.h"
#include "file.h"
#include "help_screen.h"
#include "iwt.h"
#include "key_bind.h"
#include "logger.h"
#include "logger_view.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "sat.h"
#include "test.h"
#include "undo.h"
#include "util.h"
#include "vfs.h"
#include "wnd.h"
#include "wnd_button.h"
#include "wnd_checkbox.h"
#include "wnd_combobox.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"
#include "wnd_label.h"
#include "wnd_radio.h"

/*****
 *
 * Global variables
 *
 *****/

/* Files for player to play */
int player_num_files = 0;
char **player_files = NULL;

/* Play list */
plist_t *player_plist = NULL;

/* Command repeat value */
int player_repval = 0;
int player_repval_last_key = 0;

/* Search string and criteria */
char *player_search_string = NULL;
int player_search_criteria = PLIST_SEARCH_TITLE;

/* Message text */
char *player_msg = NULL;

/* Player thread ID */
pthread_t player_tid = 0;

/* Player termination flag */
bool_t player_end_thread = FALSE;

/* Timer thread ID */
pthread_t player_timer_tid = 0;

/* Timer termination flag */
bool_t player_end_timer = FALSE;
bool_t player_end_track = FALSE;

/* Current song playing time */
int player_cur_time = 0;

/* Player status */
int player_status = PLAYER_STATUS_STOPPED;

/* Current volume and balance */
int player_volume = 0;
int player_balance = 0;

/* Has equalizer value changed */
bool_t player_eq_changed = FALSE;

/* Edit boxes history lists */
editbox_history_t *player_hist_lists[PLAYER_NUM_HIST_LISTS];

/* Current input plugin */
in_plugin_t *player_inp = NULL;

/* Undo list */
undo_list_t *player_ul = NULL;

/* Do we story undo information now? */
bool_t player_store_undo = TRUE;

/* Var manager cursor position */
int player_var_mngr_pos = -1;

/* Play boundaries */
int player_start = -1, player_end = -1;

/* Marks */
#define PLAYER_NUM_MARKS ('z' - 'a' + 1)
int player_marks[PLAYER_NUM_MARKS];
int player_last_pos = -1;

/* Last position in song */
int player_last_song = -1, player_last_song_time = -1;

/* Current audio parameters */
int player_bitrate = 0, player_freq = 0, player_stereo = 0;
#define PLAYER_STEREO 1
#define PLAYER_MONO 2

/* Plugins manager */
pmng_t *player_pmng = NULL;

/* User configuration file name */
char player_cfg_file[MAX_FILE_NAME] = "";

/* Previous file browser session directory name */
char player_fb_dir[MAX_FILE_NAME] = "";

/* Root window */
wnd_t *wnd_root = NULL;

/* Play list window */
wnd_t *player_wnd = NULL;

/* Configuration list */
cfg_node_t *cfg_list = NULL;

/* Logger */
logger_t *player_log = NULL;
logger_view_t *player_logview = NULL;

/* Standard value for edit boxes width */
#define PLAYER_EB_WIDTH	50

/* VFS data */
vfs_t *player_vfs = NULL;

/*****
 *
 * Initialization/deinitialization functions
 *
 *****/

/* Initialize player */
bool_t player_init( int argc, char *argv[] )
{
	int i, l, r;
	plist_set_t *set;
	time_t t;
	char *str_time;

	/* Set signal handlers */
/*	signal(SIGINT, player_handle_signal);
	signal(SIGTERM, player_handle_signal);*/
	
	/* Initialize configuration */
	snprintf(player_cfg_file, sizeof(player_cfg_file), 
			"%s/.mpfcrc", getenv("HOME"));
	if (!player_init_cfg())
	{
		fprintf(stderr, _("Unable to initialize configuration"));
		return FALSE;
	}

	/* Initialize logger */
	player_log = logger_new(cfg_list, cfg_get_var(cfg_list, "log-file"));
	if (player_log == NULL)
	{
		fprintf(stderr, _("Unable to initialize log system"));
		return FALSE;
	}
	logger_attach_handler(player_log, player_on_log_msg, NULL);
	t = time(NULL);
	str_time = ctime(&t);
	logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_LOW, 
			_("MPFC 1.3.1 Log\n%s"), str_time);
	free(str_time);

	/* Parse command line */
	if (!player_parse_cmd_line(argc, argv))
		return FALSE;

	/* Initialize window system */
	wnd_root = wnd_init(cfg_list, player_log);
	if (wnd_root == NULL)
	{
		logger_message(player_log, LOGGER_MSG_FATAL, LOGGER_LEVEL_LOW,
				_("Window system initialization failed"));
		return FALSE;
	}
	player_wnd = player_wnd_new(wnd_root);
	if (player_wnd == NULL)
	{
		logger_message(player_log, LOGGER_MSG_FATAL, LOGGER_LEVEL_LOW,
				_("Unable to initialize play list window"));
		return FALSE;
	}
	player_logview = logview_new(wnd_root, player_log);
	wnd_set_focus(player_wnd);

	/* Initialize key bindings */
	kbind_init();

	/* Initialize colors */
	col_init();

	/* Initialize file browser directory */
	getcwd(player_fb_dir, sizeof(player_fb_dir));
	strcat(player_fb_dir, "/");
	
	/* Initialize plugin manager */
	player_pmng = pmng_init(cfg_list, player_log);

	/* Initialize VFS */
	player_vfs = vfs_init(player_pmng);

	/* Initialize song adder thread */
	sat_init();

	/* Initialize info writer thread */
	iwt_init();

	/* Initialize undo list */
	player_ul = undo_new();

	/* Create a play list and add files to it */
	player_plist = plist_new(3);
	if (player_plist == NULL)
	{
		logger_message(player_log, LOGGER_MSG_FATAL, LOGGER_LEVEL_LOW,
				_("Play list initialization failed"));
		return FALSE;
	}

	/* Make a set of files to add */
	set = plist_set_new(FALSE);
	for ( i = 0; i < player_num_files; i ++ )
		plist_set_add(set, player_files[i]);
	plist_add_set(player_plist, set);
	plist_set_free(set);

	/* Load saved play list if files list is empty */
	if (!player_num_files)
		plist_add(player_plist, "~/mpfc.m3u");

	/* Initialize history lists */
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
		player_hist_lists[i] = editbox_history_new();

	/* Initialize playing thread */
	pthread_create(&player_tid, NULL, player_thread, NULL);

	/* Get volume */
	outp_get_volume(player_pmng->m_cur_out, &l, &r);
	if (l == 0 && r == 0)
	{
		player_volume = 0;
		player_balance = 0;
	}
	else
	{
		if (l > r)
		{
			player_volume = l;
			player_balance = r * 50 / l;
		}
		else
		{
			player_volume = r;
			player_balance = 100 - l * 50 / r;
		}
	}

	/* Initialize marks */
	for ( i = 0; i < PLAYER_NUM_MARKS; i ++ )
		player_marks[i] = -1;

	/* Initialize equalizer */
	player_eq_changed = FALSE;

	/* Start playing from last stop */
	if (cfg_get_var_int(cfg_list, "play-from-stop") && !player_num_files)
	{
		player_status = cfg_get_var_int(cfg_list, "player-status");
		player_start = cfg_get_var_int(cfg_list, "player-start") - 1;
		player_end = cfg_get_var_int(cfg_list, "player-end") - 1;
		if (player_status != PLAYER_STATUS_STOPPED)
			player_play(cfg_get_var_int(cfg_list, "cur-song"),
					cfg_get_var_int(cfg_list, "cur-time"));
	}

	/* Exit */
	logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_LOW, 
			_("Player initialized"));
	return TRUE;
} /* End of 'player_init' function */

/* Initialize the player window */
wnd_t *player_wnd_new( wnd_t *parent )
{
	wnd_t *wnd;

	/* Allocate memory */
	wnd = (wnd_t *)malloc(sizeof(*wnd));
	if (wnd == NULL)
		return NULL;
	memset(wnd, 0, sizeof(*wnd));
	wnd->m_class = player_wnd_class_init(WND_GLOBAL(parent));

	/* Initialize the window */
	if (!wnd_construct(wnd, parent, _("Play list"), 0, 0, 10, 10,
				WND_FLAG_MAXIMIZED))
	{
		free(wnd);
		return NULL;
	}

	/* Set message map */
	wnd_msg_add_handler(wnd, "display", player_on_display);
	wnd_msg_add_handler(wnd, "keydown", player_on_keydown);
	wnd_msg_add_handler(wnd, "close", player_on_close);
	wnd_msg_add_handler(wnd, "mouse_ldown", player_on_mouse_ldown);
	wnd_msg_add_handler(wnd, "mouse_mdown", player_on_mouse_mdown);
	wnd_msg_add_handler(wnd, "mouse_ldouble", player_on_mouse_ldouble);
	wnd_msg_add_handler(wnd, "user", player_on_user);

	/* Set fields */
	wnd->m_cursor_hidden = TRUE;
	wnd_postinit(wnd);
	return wnd;
} /* End of 'player_wnd_new' function */

/* Unitialize player */
void player_deinit( void )
{
	int i;

	/* Save information about place in song where we stop */
	cfg_set_var_int(cfg_list, "cur-song", 
			player_plist->m_cur_song);
	cfg_set_var_int(cfg_list, "cur-time", player_cur_time);
	cfg_set_var_int(cfg_list, "player-status", player_status);
	cfg_set_var_int(cfg_list, "player-start", player_start + 1);
	cfg_set_var_int(cfg_list, "player-end", player_end + 1);
	player_save_cfg_vars(cfg_list, "cur-song;cur-time;player-status;"
			"player-start;player-end");
	
	/* End playing thread */
	sat_free();
	iwt_free();
	inp_set_next_song(player_inp, NULL);
	if (player_tid)
	{
		player_end_track = TRUE;
		player_end_thread = TRUE;
		pthread_join(player_tid, NULL);
		player_end_thread = FALSE;
		player_tid = 0;
	}
	
	/* Uninitialize plugin manager */
	pmng_free(player_pmng);
	player_pmng = NULL;

	/* Uninitialize key bindings */
	kbind_free();

	/* Uninitialize history lists */
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
	{
		editbox_history_free(player_hist_lists[i]);
		player_hist_lists[i] = NULL;
	}

	/* Free memory allocated for strings */
	if (player_search_string != NULL)
	{
		free(player_search_string);
		player_search_string = NULL;
	}
	
	/* Destroy all objects */
	if (player_plist != NULL)
	{
		/* Save play list */
		if (cfg_get_var_int(cfg_list, "save-playlist-on-exit"))
			plist_save(player_plist, "~/mpfc.m3u");
		
		plist_free(player_plist);
		player_plist = NULL;
	}
	undo_free(player_ul);
	player_ul = NULL;
	vfs_free(player_vfs);

	/* Free logger */
	if (player_log != NULL)
	{
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_LOW,
				_("Left MPFC"));
		logger_free(player_log);
	}

	/* Uninitialize configuration manager */
	cfg_free_node(cfg_list, TRUE);
	cfg_list = NULL;
} /* End of 'player_deinit' function */

/* Run player */
bool_t player_run( void )
{
	/* Run window message loop */
	wnd_main(wnd_root);
	wnd_root = NULL;
	player_wnd = NULL;
	return TRUE;
} /* End of 'player_run' function */

/* Parse program command line */
bool_t player_parse_cmd_line( int argc, char *argv[] )
{
	int i;

	/* Get options */
	for ( i = 1; i < argc && argv[i][0] == '-'; i ++ )
	{
		char *name, *val;
		char *str = argv[i];
		int name_start, name_end;
		
		/* Get variable name start */
		for ( name_start = 0; str[name_start] == '-'; name_start ++ );
		if (name_start >= strlen(str))
			continue;

		/* Get variable name end */
		for ( name_end = name_start; 
				str[name_end] && str[name_end] != '=' && str[name_end] != ' '; 
				name_end ++ );
		name_end --;

		/* Extract variable name */
		name = strndup(&str[name_start], name_end - name_start + 1);

		/* We have no value - assume it "1" */
		if (name_end == strlen(str) - 1)
		{
			val = strdup("1");
		}
		/* Extract value */
		else
		{
			val = strdup(&str[name_end + 2]);
		}
		
		/* Set respective variable */
		cfg_set_var(cfg_list, name, val);
		free(name);
		free(val);
	}

	/* Get file names from command line */
	player_num_files = argc - i;
	player_files = &argv[i];
	return TRUE;
} /* End of 'player_parse_cmd_line' function */

/* Initialize configuration */
bool_t player_init_cfg( void )
{
	char *log_file;

	/* Initialize root configuration list and variables handlers */
	cfg_list = cfg_new_list(NULL, "", NULL, CFG_NODE_BIG_LIST, 0);
	cfg_new_var(cfg_list, "cur-song", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "cur-song-name", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "cur-time", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-status", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-start", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-end", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "title-format", 0, NULL, 
			player_handle_var_title_format);
	cfg_new_var(cfg_list, "output-plugin", 0, NULL, 
			player_handle_var_outp);
	cfg_new_var(cfg_list, "color-scheme", 0, NULL,
			player_handle_color_scheme);
	cfg_new_var(cfg_list, "kbind-scheme", 0, NULL, 
			player_handle_kbind_scheme);

	/* Set default variable values */
	log_file = (char *)malloc(strlen(getenv("HOME")) + 
			strlen("/.mpfc/log") + 1);
	strcpy(log_file, getenv("HOME"));
	strcat(log_file, "/.mpfc/log");
	cfg_set_var(cfg_list, "log-file", log_file);
	free(log_file);
	cfg_set_var(cfg_list, "output-plugin", "oss");
	cfg_set_var_int(cfg_list, "mp3-quick-get-len", 1);
	cfg_set_var_int(cfg_list, "save-playlist-on-exit", 1);
	cfg_set_var_int(cfg_list, "play-from-stop", 1);
	cfg_set_var(cfg_list, "lib-dir", LIBDIR"/mpfc");
	cfg_set_var_int(cfg_list, "echo-delay", 500);
	cfg_set_var_int(cfg_list, "echo-volume", 50);
	cfg_set_var_int(cfg_list, "echo-feedback", 50);
	cfg_set_var_int(cfg_list, "fb-fname-len", 0);
	cfg_set_var_int(cfg_list, "fb-artist-len", 15);
	cfg_set_var_int(cfg_list, "fb-title-len", 30);
	cfg_set_var_int(cfg_list, "fb-album-len", 21);
	cfg_set_var_int(cfg_list, "fb-year-len", 4);
	cfg_set_var_int(cfg_list, "fb-comments-len", 0);
	cfg_set_var_int(cfg_list, "fb-genre-len", 0);
	cfg_set_var_int(cfg_list, "fb-track-len", 0);
	cfg_set_var_int(cfg_list, "fb-time-len", 5);

	/* Read rc file from home directory and from current directory */
	player_read_rcfile(cfg_list, "/etc/mpfcrc");
	player_read_rcfile(cfg_list, player_cfg_file);
	player_read_rcfile(cfg_list, ".mpfcrc");
	return TRUE;
} /* End of 'player_init_cfg' function */

/* Read configuration file */
void player_read_rcfile( cfg_node_t *list, char *name )
{
	file_t *fd;

	assert(list);
	assert(name);

	/* Try to open file */
	fd = file_open(name, "rt", NULL);
	if (fd == NULL)
		return;

	/* Read */
	while (!file_eof(fd))
	{
		str_t *str;
		
		/* Read line */
		str = file_get_str(fd);

		/* Parse this line */
		player_parse_cfg_line(list, STR_TO_CPTR(str));
		str_free(str);
	}

	/* Close file */
	file_close(fd);
} /* End of 'player_read_rcfile' function */

/* Read one line from the configuration file */
#define player_is_whitespace(c)	((c) <= 0x20)	
void player_parse_cfg_line( cfg_node_t *list, char *str )
{
	int i, j, len;
	char *name, *val;

	assert(list);
	assert(str);
	
	/* If string begins with '#' - it is comment */
	if (str[0] == '#')
		return;

	/* Skip white space */
	len = strlen(str);
	for ( i = 0; i < len && player_is_whitespace(str[i]); i ++ );

	/* Read until next white space */
	for ( j = i; j < len && str[j] != '=' && 
			!player_is_whitespace(str[j]); j ++ );
	if (player_is_whitespace(str[j]) || str[j] == '=')
		j --;

	/* Extract variable name */
	name = (char *)malloc(j - i + 2);
	memcpy(name, &str[i], j - i + 1);
	name[j - i + 1] = 0;

	/* Read '=' sign */
	for ( ; j < len && str[j] != '='; j ++ );

	/* Variable has no value - let it be "1" */
	if (j == len)
	{
		cfg_set_var(list, name, "1");
		free(name);
	}
	/* Read value */
	else
	{
		/* Get value begin */
		for ( i = j + 1; i < len && player_is_whitespace(str[i]); i ++ );

		/* Get value end */
		for ( j = i; j < len && !player_is_whitespace(str[j]); j ++ );
		if (player_is_whitespace(str[j]))
			j --;

		/* Extract value and set it */
		val = (char *)malloc(j - i + 2);
		memcpy(val, &str[i], j - i + 1);
		val[j - i + 1] = 0;
		cfg_set_var(list, name, val);
		free(name);
		free(val);
	}
} /* End of 'player_parse_cfg_line' function */

/* Save variables to main configuration file */
void player_save_cfg_vars( cfg_list_t *list, char *vars )
{
	char *name;
	cfg_node_t *tlist;
	int i, j;
	
	if (list == NULL)
		return;

	/* Initialize variables with initial values */
	tlist = cfg_new_list(NULL, "root", NULL, 0, 0);

	/* Read rc file */
	player_read_rcfile(tlist, player_cfg_file);

	/* Update variables */
	for ( i = 0, j = 0;; i ++ )
	{
		/* End of variable name */
		if (vars[i] == ';' || vars[i] == '\0')
		{
			name = strndup(&vars[j], i - j);
			j = i + 1;
			cfg_set_var(tlist, name, cfg_get_var(list, name));
			free(name);
			if (!vars[i])
				break;
		}
	}

	/* Save temporary list to configuration file */
	player_save_cfg_list(tlist, player_cfg_file);

	/* Free temporary list */
	cfg_free_node(tlist, TRUE);
} /* End of 'player_save_cfg_vars' function */

/* Save configuration list */
void player_save_cfg_list( cfg_node_t *list, char *fname )
{
	FILE *fd;
	int i;
	cfg_node_t *node;

	assert(list);
	assert(CFG_NODE_IS_LIST(list));

	/* Open file */
	fd = fopen(fname, "wt");
	if (fd == NULL)
		return;

	/* Write variables */
	for ( i = 0; i < CFG_LIST(list)->m_hash_size; i ++ )
	{
		struct cfg_list_hash_item_t *item;
		for ( item = CFG_LIST(list)->m_children[i];
				item != NULL; item = item->m_next )
		{
			node = item->m_node;
			if ((node->m_flags & CFG_NODE_VAR) && 
					!(node->m_flags & CFG_NODE_RUNTIME))
				fprintf(fd, "%s=%s\n", node->m_name, CFG_VAR_VALUE(node));
		}
	}

	/* Close file */
	fclose(fd);
} /* End of 'player_save_cfg_list' function */

/* Signal handler */
void player_handle_signal( int signum )
{
	if (signum == SIGINT || signum == SIGTERM)
		player_deinit();
} /* End of 'player_handle_signal' function */

/*****
 *
 * Message handlers
 *
 *****/

/* Play list window closing handler */
wnd_msg_retcode_t player_on_close( wnd_t *wnd )
{
	assert(wnd);
	wnd_close(wnd_root);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_close' function */

/* Handle key function */
wnd_msg_retcode_t player_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	kbind_key2buf(key);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_handle_key' function */

/* Handle action */
void player_handle_action( int action )
{
	int was_pos;
	int was_song, was_time;

	/* Clear message string */
	//player_print_msg("");

	was_pos = player_plist->m_sel_end;
	was_song = player_plist->m_cur_song;
	was_time = player_cur_time;
	switch (action)
	{
	/* Exit MPFC */
	case KBIND_QUIT:
		wnd_close(wnd_root);
		break;

	/* Show help screen */
	case KBIND_HELP:
		help_new(wnd_root, HELP_PLAYER);
		break;

	/* Move cursor */
	case KBIND_MOVE_DOWN:
		plist_move(player_plist, (player_repval == 0) ? 1 : player_repval, 
				TRUE);
		break;
	case KBIND_MOVE_UP:
		plist_move(player_plist, (player_repval == 0) ? -1 : -player_repval, 
				TRUE);
		break;
	case KBIND_SCREEN_DOWN:
		plist_move(player_plist, (player_repval == 0) ? 
				PLIST_HEIGHT : 
				PLIST_HEIGHT * player_repval, TRUE);
		break;
	case KBIND_SCREEN_UP:
		plist_move(player_plist, (player_repval == 0) ? 
				-PLIST_HEIGHT : 
				-PLIST_HEIGHT * player_repval, TRUE);
		break;
	case KBIND_MOVE:
		plist_move(player_plist, (player_repval == 0) ? 
				player_plist->m_len - 1 : player_repval - 1, FALSE);
		break;

	/* Seek song */
	case KBIND_TIME_FW:
		player_seek((player_repval == 0) ? 10 : 10 * player_repval, TRUE);
		break;
	case KBIND_TIME_BW:
		player_seek((player_repval == 0) ? -10 : -10 * player_repval, TRUE);
		break;
	case KBIND_TIME_MOVE:
		player_seek((player_repval == 0) ? 0 : player_repval, FALSE);
		break;

	/* Increase/decrease volume */
	case KBIND_VOL_FW:
		player_set_vol((player_repval == 0) ? 5 : 5 * player_repval, TRUE);
		break;
	case KBIND_VOL_BW:
		player_set_vol((player_repval == 0) ? -5 : -5 * player_repval, TRUE);
		break;

	/* Increase/decrease balance */
	case KBIND_BAL_FW:
		player_set_bal((player_repval == 0) ? 5 : 5 * player_repval, TRUE);
		break;
	case KBIND_BAL_BW:
		player_set_bal((player_repval == 0) ? -5 : -5 * player_repval, TRUE);
		break;

	/* Centrize view */
	case KBIND_CENTRIZE:
		plist_centrize(player_plist, -1);
		break;

	/* Enter visual mode */
	case KBIND_VISUAL:
		player_plist->m_visual = !player_plist->m_visual;
		break;

	/* Resume playing */
	case KBIND_PLAY:
		if (player_status == PLAYER_STATUS_PAUSED)
		{
			inp_resume(player_inp);
			outp_resume(player_pmng->m_cur_out);
		}
		else
			player_play(player_plist->m_cur_song, 0);
		player_status = PLAYER_STATUS_PLAYING;
		break;

	/* Pause */
	case KBIND_PAUSE:
		if (player_status == PLAYER_STATUS_PLAYING)
		{
			player_status = PLAYER_STATUS_PAUSED;
			inp_pause(player_inp);
			outp_pause(player_pmng->m_cur_out);
		}
		else if (player_status == PLAYER_STATUS_PAUSED)
		{
			player_status = PLAYER_STATUS_PLAYING;
			inp_resume(player_inp);
			outp_resume(player_pmng->m_cur_out);
		}
		break;

	/* Stop */
	case KBIND_STOP:
		player_status = PLAYER_STATUS_STOPPED;
		player_end_play(FALSE);
		player_plist->m_cur_song = was_song;
		break;

	/* Play song */
	case KBIND_START_PLAY:
		if (!player_plist->m_len)
			break;
		player_status = PLAYER_STATUS_PLAYING;
		player_play(player_plist->m_sel_end, 0);
		break;

	/* Go to next song */
	case KBIND_NEXT:
		player_skip_songs((player_repval) ? player_repval : 1, TRUE);
		break;

	/* Go to previous song */
	case KBIND_PREV:
		player_skip_songs(-((player_repval) ? player_repval : 1), TRUE);
		break;

	/* Add a file */
	case KBIND_ADD:
		player_add_dialog();
		break;

	/* Add an object */
	case KBIND_ADD_OBJ:
		player_add_obj_dialog();
		break;

	/* Save play list */
	case KBIND_SAVE:
		player_save_dialog();
		break;

	/* Remove song(s) */
	case KBIND_REM:
		player_rem_dialog();
		break;

	/* Sort play list */
	case KBIND_SORT:
		player_sort_dialog();
		break;

	/* Song info dialog */
	case KBIND_INFO:
		player_info_dialog();
		break;

	/* Search */
	case KBIND_SEARCH:
		player_search_dialog();
		break;

	/* Advanced search */
	case KBIND_ADVANCED_SEARCH:
		player_advanced_search_dialog();
		break;

	/* Find next/previous search match */
	case KBIND_NEXT_MATCH:
	case KBIND_PREV_MATCH:
		if (!plist_search(player_plist, player_search_string, 
					(action == KBIND_NEXT_MATCH) ? 1 : -1, 
					player_search_criteria))
			logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
					_("String `%s' not found"), 
					player_search_string);
		else
			logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
					_("String `%s' found at position %d"), player_search_string,
					player_plist->m_sel_end);
		break;

	/* Show equalizer dialog */
	case KBIND_EQUALIZER:
		eqwnd_new(wnd_root);
		break;

	/* Set/unset shuffle mode */
	case KBIND_SHUFFLE:
		cfg_set_var_int(cfg_list, "shuffle-play",
				!cfg_get_var_int(cfg_list, "shuffle-play"));
		break;
		
	/* Set/unset loop mode */
	case KBIND_LOOP:
		cfg_set_var_int(cfg_list, "loop-play",
				!cfg_get_var_int(cfg_list, "loop-play"));
		break;

	/* Variables manager */
	case KBIND_VAR_MANAGER:
//		player_var_manager();
		break;
	case KBIND_VAR_MINI_MANAGER:
		player_var_mini_manager();
		break;

	/* Move play list */
	case KBIND_PLIST_DOWN:
		plist_move_sel(player_plist, (player_repval == 0) ? 1 : player_repval, 
				TRUE);
		break;
	case KBIND_PLIST_UP:
		plist_move_sel(player_plist, (player_repval == 0) ? -1 : 
				-player_repval, TRUE);
		break;
	case KBIND_PLIST_MOVE:
		plist_move_sel(player_plist, (player_repval == 0) ? 
				1 : player_repval - 1, FALSE);
		break;

	/* Undo/redo */
	case KBIND_UNDO:
		undo_bw(player_ul);
		break;
	case KBIND_REDO:
		undo_fw(player_ul);
		break;

	/* Reload info */
	case KBIND_RELOAD_INFO:
		player_info_reload_dialog();
		break;

	/* Set play boundaries */
	case KBIND_SET_PLAY_BOUNDS:
		PLIST_GET_SEL(player_plist, player_start, player_end);
		break;

	/* Clear play boundaries */
	case KBIND_CLEAR_PLAY_BOUNDS:
		player_start = player_end = -1;
		break;

	/* Set play boundaries and play from area beginning */
	case KBIND_PLAY_BOUNDS:
		PLIST_GET_SEL(player_plist, player_start, player_end);
		if (!player_plist->m_len)
			break;
		player_status = PLAYER_STATUS_PLAYING;
		player_play(player_start, 0);
		break;

	/* Execute outer command */
	case KBIND_EXEC:
		player_exec_dialog();
		break;

	/* Go back */
	case KBIND_TIME_BACK:
		player_time_back();
		break;

	/* Set mark */
	case KBIND_MARKA:
	case KBIND_MARKB:
	case KBIND_MARKC:
	case KBIND_MARKD:
	case KBIND_MARKE:
	case KBIND_MARKF:
	case KBIND_MARKG:
	case KBIND_MARKH:
	case KBIND_MARKI:
	case KBIND_MARKJ:
	case KBIND_MARKK:
	case KBIND_MARKL:
	case KBIND_MARKM:
	case KBIND_MARKN:
	case KBIND_MARKO:
	case KBIND_MARKP:
	case KBIND_MARKQ:
	case KBIND_MARKR:
	case KBIND_MARKS:
	case KBIND_MARKT:
	case KBIND_MARKU:
	case KBIND_MARKV:
	case KBIND_MARKW:
	case KBIND_MARKX:
	case KBIND_MARKY:
	case KBIND_MARKZ:
		player_set_mark(action - KBIND_MARKA + 'a');
		break;

	/* Go to mark */
	case KBIND_GOA:
	case KBIND_GOB:
	case KBIND_GOC:
	case KBIND_GOD:
	case KBIND_GOE:
	case KBIND_GOF:
	case KBIND_GOG:
	case KBIND_GOH:
	case KBIND_GOI:
	case KBIND_GOJ:
	case KBIND_GOK:
	case KBIND_GOL:
	case KBIND_GOM:
	case KBIND_GON:
	case KBIND_GOO:
	case KBIND_GOP:
	case KBIND_GOQ:
	case KBIND_GOR:
	case KBIND_GOS:
	case KBIND_GOT:
	case KBIND_GOU:
	case KBIND_GOV:
	case KBIND_GOW:
	case KBIND_GOX:
	case KBIND_GOY:
	case KBIND_GOZ:
		player_goto_mark(action - KBIND_GOA + 'a');
		break;

	/* Go back */
	case KBIND_GOBACK:
		player_go_back();
		break;

	/* Reload plugins */
	case KBIND_RELOAD_PLUGINS:
		pmng_load_plugins(player_pmng);
		break;

	/* Launch file browser */
	case KBIND_FILE_BROWSER:
		fb_new(wnd_root, player_fb_dir);
		break;

	/* Launch test dialog */
	case KBIND_TEST:
		player_test_dialog();
		break;

	/* Digit means command repeation value edit */
	case KBIND_DIG_1:
	case KBIND_DIG_2:
	case KBIND_DIG_3:
	case KBIND_DIG_4:
	case KBIND_DIG_5:
	case KBIND_DIG_6:
	case KBIND_DIG_7:
	case KBIND_DIG_8:
	case KBIND_DIG_9:
	case KBIND_DIG_0:
		player_repval_dialog(action - KBIND_DIG_0);
		break;
	}

	/* Flush repeat value */
	player_repval = 0;

	/* Save last position */
	if (player_plist->m_sel_end != was_pos)
		player_last_pos = was_pos;

	/* Save last time */
	if (player_plist->m_cur_song != was_song || player_cur_time != was_time)
	{
		player_last_song = was_song;
		player_last_song_time = was_time;
	}

	/* Repaint */
	wnd_invalidate(player_wnd);
} /* End of 'player_handle_action' function */

/* Handle left-button click */
wnd_msg_retcode_t player_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Move cursor in play list */
	if (y >= player_plist->m_start_pos && 
			y < player_plist->m_start_pos + PLIST_HEIGHT)
	{
		plist_move(player_plist, y - player_plist->m_start_pos + 
				player_plist->m_scrolled, FALSE);
		wnd_invalidate(wnd);
	}
	/* Set volume */
	else if (y == PLAYER_SLIDER_VOL_Y && x >= PLAYER_SLIDER_VOL_X &&
				x <= PLAYER_SLIDER_VOL_X + PLAYER_SLIDER_VOL_W)
	{
		player_set_vol((x - PLAYER_SLIDER_VOL_X) * 100 / 
				PLAYER_SLIDER_VOL_W, FALSE);
		wnd_invalidate(wnd);
	}
	/* Set balance */
	else if (y == PLAYER_SLIDER_BAL_Y && x >= PLAYER_SLIDER_BAL_X &&
				x <= PLAYER_SLIDER_BAL_X + PLAYER_SLIDER_BAL_W)
	{
		player_set_bal((x - PLAYER_SLIDER_BAL_X) * 100 / 
				PLAYER_SLIDER_BAL_W, FALSE);
		wnd_invalidate(wnd);
	}
	/* Set time */
	else if (y == PLAYER_SLIDER_TIME_Y && x >= PLAYER_SLIDER_TIME_X &&
				x <= PLAYER_SLIDER_TIME_X + PLAYER_SLIDER_TIME_W)
	{
		if (player_plist->m_cur_song >= 0)
		{
			song_t *s = player_plist->m_list[player_plist->m_cur_song];
			if (s != NULL)
			{
				player_seek((x - PLAYER_SLIDER_TIME_X) * 
						s->m_len / PLAYER_SLIDER_TIME_W, FALSE);
				wnd_invalidate(wnd);
			}
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_mouse_ldown' function */

/* Handle middle-button click */
wnd_msg_retcode_t player_on_mouse_mdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Centrize this song */
	if (y >= player_plist->m_start_pos && 
			y < player_plist->m_start_pos + PLIST_HEIGHT)
	{
		plist_centrize(player_plist, y - player_plist->m_start_pos + 
				player_plist->m_scrolled);
		wnd_invalidate(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_mouse_mdown' function */

/* Handle left-button double click */
wnd_msg_retcode_t player_on_mouse_ldouble( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Play song */
	if (y >= player_plist->m_start_pos && 
			y < player_plist->m_start_pos + PLIST_HEIGHT)
	{
		int s = y - player_plist->m_start_pos + player_plist->m_scrolled;

		if (s >= 0 && s < player_plist->m_len)
		{
			player_status = PLAYER_STATUS_PLAYING;
			player_play(s, 0);
			wnd_invalidate(wnd);
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_mouse_ldouble' function */

/* Handle user message */
wnd_msg_retcode_t player_on_user( wnd_t *wnd, int id, void *data )
{
	switch (id)
	{
	case PLAYER_MSG_INFO:
		plist_move(player_plist, (int)data, FALSE);
		player_info_dialog();
		break;
	case PLAYER_MSG_NEXT_FOCUS:
		wnd_next_focus(wnd_root);
		break;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_user' function */

/* Display player function */
wnd_msg_retcode_t player_on_display( wnd_t *wnd )
{
	int i;
	song_t *s = NULL;
	char aparams[256];

	/* Display head */
	wnd_move(wnd, 0, 0, 0);
	if (player_plist->m_cur_song == -1)
	{
		char *shuffle_str, *loop_str;

		/* Print about */
		wnd_move(wnd, 0, 0, 0);
		wnd_apply_style(wnd, "about-style");
		wnd_printf(wnd, 0, 0, _("SG Software Media Player For Console"));
		
		/* Print shuffle mode */
		shuffle_str = _("Shuffle");
		loop_str = _("Loop");
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - strlen(shuffle_str) - 
					strlen(loop_str) - 2, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, shuffle_str);
		}
		
		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - strlen(loop_str) - 1, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, loop_str);
		}

		/* Print version */
		wnd_move(wnd, 0, 0, 1);
		wnd_apply_style(wnd, "about-style");
		wnd_printf(wnd, 0, 0, _("version %s"), VERSION);
	}
	else
	{
		int t;
		bool_t show_rem;
		char *shuffle_str, *loop_str;
		
		/* Print current song title */
		s = player_plist->m_list[player_plist->m_cur_song];
		wnd_move(wnd, 0, 0, 0);
		wnd_apply_style(wnd, "title-style");
		wnd_printf(wnd, WND_PRINT_ELLIPSES, WND_WIDTH(wnd) - 1, "%s", 
				STR_TO_CPTR(s->m_title));

		/* Print shuffle mode */
		shuffle_str = _("Shuffle");
		loop_str = _("Loop");
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - strlen(shuffle_str) - 
					strlen(loop_str) - 2, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, shuffle_str);
		}
		
		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - strlen(loop_str) - 1, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, loop_str);
		}

		/* Print current time */
		t = (show_rem = cfg_get_var_int(cfg_list, "show-time-remaining")) ? 
			s->m_len - player_cur_time : player_cur_time;
		wnd_move(wnd, 0, 0, 1);
		wnd_apply_style(wnd, "time-style");
		wnd_printf(wnd, 0, 0, "%s%i:%02i/%i:%02i\n", 
				show_rem ? "-" : "", t / 60, t % 60,
				s->m_len / 60, s->m_len % 60);
	}

	/* Display current audio parameters */
	strcpy(aparams, "");
	if (player_bitrate)
		sprintf(aparams, "%s%d kbps ", aparams, player_bitrate);
	if (player_freq)
		sprintf(aparams, "%s%d Hz ", aparams, player_freq);
	if (player_stereo)
		sprintf(aparams, "%s%s", aparams, 
				(player_stereo == PLAYER_STEREO) ? "stereo" : "mono");
	wnd_move(wnd, 0, PLAYER_SLIDER_BAL_X - strlen(aparams) - 1, 
			PLAYER_SLIDER_BAL_Y);
	wnd_apply_style(wnd, "audio-params-style");
	wnd_printf(wnd, 0, 0, "%s", aparams);

	/* Display various slidebars */
	player_display_slider(wnd, PLAYER_SLIDER_BAL_X, PLAYER_SLIDER_BAL_Y,
			PLAYER_SLIDER_BAL_W, player_balance, 100.);
	player_display_slider(wnd, PLAYER_SLIDER_TIME_X, PLAYER_SLIDER_TIME_Y, 
			PLAYER_SLIDER_TIME_W, player_cur_time, (s == NULL) ? 0 : s->m_len);
	player_display_slider(wnd, PLAYER_SLIDER_VOL_X, PLAYER_SLIDER_VOL_Y,
			PLAYER_SLIDER_VOL_W, player_volume, 100.);
	
	/* Display play list */
	plist_display(player_plist, wnd);

	/* Print message */
	if (player_msg != NULL)
	{
		wnd_move(wnd, 0, 0, WND_HEIGHT(wnd) - 1);
		wnd_apply_style(wnd, "status-style");
		wnd_printf(wnd, 0, 0, "%s", player_msg);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_display' function */

/* Display slider */
void player_display_slider( wnd_t *wnd, int x, int y, int width, 
		int pos, int range )
{
	int i, slider_pos;
	
	wnd_move(wnd, 0, x, y);
	slider_pos = (range) ? (pos * width / range) : 0;
	wnd_apply_style(wnd, "slider-style");
	for ( i = 0; i <= width; i ++ )
	{
		if (i == slider_pos)
			wnd_putchar(wnd, 0, 'O');
		else 
			wnd_putchar(wnd, 0,  '=');
	}
} /* End of 'player_display_slider' function */

/* Handle new log message */
void player_on_log_msg( logger_t *log, void *data, 
		struct logger_message_t *msg )
{
	/* Print message to status line */
	player_msg = msg->m_message;
	wnd_invalidate(player_wnd);
	wnd_invalidate(WND_OBJ(player_logview));
} /* End of 'player_on_log_msg' function */

/*****
 *
 * Playing-related functions
 *
 *****/

/* Seek song */
void player_seek( int sec, bool_t rel )
{
	song_t *s;
	int new_time;
	
	if (player_plist->m_cur_song == -1)
		return;

	s = player_plist->m_list[player_plist->m_cur_song];

	new_time = (rel ? (player_cur_time + sec) : sec);
	if (new_time < 0)
		new_time = 0;
	else if (new_time > s->m_len)
		new_time = s->m_len;

	inp_seek(song_get_inp(s, NULL), new_time);
	player_cur_time = new_time;
	wnd_invalidate(player_wnd);
} /* End of 'player_seek' function */

/* Play song */
void player_play( int song, int start_time )
{
	song_t *s;

	/* End current playing */
	player_end_play(FALSE);

	/* Check that we have anything to play */
	if (song < 0 || song >= player_plist->m_len ||
			(s = player_plist->m_list[song]) == NULL)
	{
		player_plist->m_cur_song = -1;
		return;
	}

	/* Start new playing thread */
	cfg_set_var(cfg_list, "cur-song-name", 
			util_short_name(s->m_file_name));
	player_plist->m_cur_song = song;
	player_cur_time = start_time;
//	player_status = PLAYER_STATUS_PLAYING;
} /* End of 'player_play' function */

/* End playing song */
void player_end_play( bool_t rem_cur_song )
{
	int was_song = player_plist->m_cur_song;
	
	player_plist->m_cur_song = -1;
	player_end_track = TRUE;
//	player_status = PLAYER_STATUS_STOPPED;
	while (player_timer_tid)
		util_delay(0, 100000);
	if (!rem_cur_song)
		player_plist->m_cur_song = was_song;
	cfg_set_var(cfg_list, "cur-song-name", "");
} /* End of 'player_end_play' function */

/* Go to next track */
void player_next_track( void )
{
	int next_track;
	
	next_track = player_skip_songs(1, FALSE);
	inp_set_next_song(player_inp, next_track >= 0 ?
		player_plist->m_list[next_track]->m_file_name : NULL);
	player_set_track(next_track);
} /* End of 'player_next_track' function */

/* Start track */
void player_set_track( int track )
{
	if (track < 0)
		player_end_play(TRUE);
	else
		player_play(track, 0);
} /* End of 'player_set_track' function */

/* Set volume */
void player_set_vol( int vol, bool_t rel )
{
	player_volume = (rel) ? player_volume + vol : vol;
	if (player_volume < 0)
		player_volume = 0;
	else if (player_volume > 100)
		player_volume = 100;
	player_update_vol();
} /* End of 'player_set_vol' function */

/* Set balance */
void player_set_bal( int bal, bool_t rel )
{
	player_balance = (rel) ? player_balance + bal : bal;
	if (player_balance < 0)
		player_balance = 0;
	else if (player_balance > 100)
		player_balance = 100;
	player_update_vol();
} /* End of 'player_set_bal' function */

/* Update volume */
void player_update_vol( void )
{
	int l, r;

	if (player_balance < 50)
	{
		l = player_volume;
		r = player_volume * player_balance / 50;
	}
	else
	{
		r = player_volume;
		l = player_volume * (100 - player_balance) / 50;
	}
	outp_set_volume(player_pmng->m_cur_out, l, r);
} /* End of 'player_update_vol' function */

/* Skip some songs */
int player_skip_songs( int num, bool_t play )
{
	int len, base, song;
	
	if (player_plist == NULL || !player_plist->m_len)
		return;
	
	/* Change current song */
	song = player_plist->m_cur_song;
	len = (player_start < 0) ? player_plist->m_len : 
		(player_end - player_start + 1);
	base = (player_start < 0) ? 0 : player_start;
	if (cfg_get_var_int(cfg_list, "shuffle-play"))
	{
		int initial = song;

		while (song == initial)
			song = base + (rand() % len);
	}
	else 
	{
		int s, cur = song;
		if (player_start >= 0 && (cur < player_start || cur > player_end))
			s = -1;
		else 
			s = cur - base;
		s += num;
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			while (s < 0)
				s += len;
			s %= len;
			song = s + base;
		}
		else if (s < 0 || s >= len)
			song = -1;
		else
			song = s + base;
	}

	/* Start or end play */
	if (play)
	{
		if (song == -1)
			player_end_play(TRUE);
		else
			player_play(song, 0);
	}
	return song;
} /* End of 'player_skip_songs' function */

/* Stop timer thread */
void player_stop_timer( void )
{
	if (player_timer_tid != 0)
	{
		player_end_timer = TRUE;
		pthread_join(player_timer_tid, NULL);
		player_end_timer = FALSE;
		player_timer_tid = 0;
		player_cur_time = 0;
	}
} /* End of 'player_stop_timer' function */

/* Timer thread function */
void *player_timer_func( void *arg )
{
	time_t t = time(NULL);

	/* Start zero timer count */
	player_cur_time = 0;

	/* Timer loop */
	while (!player_end_timer)
	{
		time_t new_t = time(NULL);
		struct timespec tv;
		int tm;

		/* Update timer */
		if (player_status == PLAYER_STATUS_PAUSED)
		{
			t = new_t;
		}
		else
		{	
			tm = inp_get_cur_time(player_inp);
			if (tm < 0)
			{
				if (new_t > t)
				{
					player_cur_time += (new_t - t);
					t = new_t;
					wnd_invalidate(player_wnd);
				}
			}
			else if (tm - player_cur_time)
			{
				player_cur_time = tm;
				wnd_invalidate(player_wnd);
			}
		}
		util_wait();
	}

	player_cur_time = 0;
	player_end_timer = FALSE;
	player_timer_tid = 0;
	return NULL;
} /* End of 'player_timer_func' function */

/* Player thread function */
void *player_thread( void *arg )
{
	bool_t no_outp = FALSE;
	
	/* Main loop */
	while (!player_end_thread)
	{
		song_t *s;
		int ch = 0, freq = 0;
		dword fmt = 0;
		song_info_t si;
		in_plugin_t *inp;
		int was_pfreq, was_pbr, was_pstereo;
		int disp_count;
		dword in_flags, out_flags;
		file_t *fd;

		/* Skip to next iteration if there is nothing to play */
		if (player_plist->m_cur_song < 0 || 
				player_status == PLAYER_STATUS_STOPPED)
		{
			util_wait();
			continue;
		}

		/* Play track */
		s = player_plist->m_list[player_plist->m_cur_song];
		//player_status = PLAYER_STATUS_PLAYING;
		player_end_track = FALSE;
	
		/* Start playing */
		inp = song_get_inp(s, &fd);
		if (!inp_start(inp, s->m_file_name, fd))
		{
			player_next_track();
			logger_message(player_log, LOGGER_MSG_ERROR, LOGGER_LEVEL_LOW,
					_("Input plugin for file %s failed"), s->m_file_name);
			wnd_invalidate(player_wnd);
			continue;
		}
		if (player_cur_time > 0)
			inp_seek(inp, player_cur_time);
		in_flags = inp_get_flags(inp);
		out_flags = outp_get_flags(player_pmng->m_cur_out);
		no_outp = (in_flags & INP_OWN_OUT) || 
			((in_flags & INP_OWN_SOUND) && !(out_flags & OUTP_NO_SOUND));

		/* Get song length and information */
		song_update_info(s);

		/* Start output plugin */
		if (!no_outp && (player_pmng->m_cur_out == NULL || 
				(!cfg_get_var_int(cfg_list, "silent-mode") && 
					!outp_start(player_pmng->m_cur_out))))
		{
			logger_message(player_log, LOGGER_MSG_ERROR, LOGGER_LEVEL_LOW,
					_("Output plugin initialization failed"));
//			wnd_send_msg(wnd_root, WND_MSG_USER, PLAYER_MSG_END_TRACK);
			inp_end(inp);
			player_status = PLAYER_STATUS_STOPPED;
			wnd_invalidate(player_wnd);
			continue;
		}

		/* Set audio parameters */
		/*if (!no_outp)
		{
			outp_set_channels(player_pmng->m_cur_out, 2);
			outp_set_freq(player_pmng->m_cur_out, freq);
			outp_set_fmt(player_pmng->m_cur_out, fmt);
		}*/

		/* Save current input plugin */
		player_inp = inp;

		/* Set equalizer */
		inp_set_eq(inp);

		/* Start timer thread */
		pthread_create(&player_timer_tid, NULL, player_timer_func, 0);
		//wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
	
		/* Play */
		disp_count = 0;
		while (!player_end_track)
		{
			byte buf[8192];
			int size = 8192;
			struct timespec tv;

			if (player_status == PLAYER_STATUS_PLAYING)
			{
				/* Update equalizer if it's parameters have changed */
				if (player_eq_changed)
				{
					player_eq_changed = FALSE;
					inp_set_eq(inp);
				}
				
				/* Get stream from input plugin */
				if (size = inp_get_stream(inp, buf, size))
				{
					int new_ch, new_freq, new_br;
					dword new_fmt;
					
					/* Get audio parameters */
					inp_get_audio_params(inp, &new_ch, &new_freq, &new_fmt, 
							&new_br);
					was_pfreq = player_freq; was_pstereo = player_stereo;
					was_pbr = player_bitrate;
					player_freq = new_freq;
					player_stereo = (new_ch == 1) ? PLAYER_MONO : PLAYER_STEREO;
					new_br /= 1000;
					player_bitrate = new_br;
					if ((player_freq != was_pfreq || 
							player_stereo != was_pstereo ||
							player_bitrate != was_pbr) && !disp_count)
					{
						disp_count = cfg_get_var_int(cfg_list, 
								"audio-params-display-count");
						if (!disp_count)
							disp_count = 20;
						wnd_invalidate(player_wnd);
					}
					disp_count --;
					if (disp_count < 0)
						disp_count = 0;

					/* Update audio parameters if they have changed */
					if (!no_outp)
					{
						if (ch != new_ch || freq != new_freq || fmt != new_fmt)
						{
							ch = new_ch;
							freq = new_freq;
							fmt = new_fmt;
						
							outp_flush(player_pmng->m_cur_out);
							outp_set_channels(player_pmng->m_cur_out, ch);
							outp_set_freq(player_pmng->m_cur_out, freq);
							outp_set_fmt(player_pmng->m_cur_out, fmt);
						}

						/* Apply effects */
						size = pmng_apply_effects(player_pmng, buf, size, 
								fmt, freq, ch);
					
						/* Send to output plugin */
						outp_play(player_pmng->m_cur_out, buf, size);
					}
					else
						util_delay(0, 100000000);
				}
				else
				{
					break;
				}
			}
			else if (player_status == PLAYER_STATUS_PAUSED)
			{
				util_delay(0, 100000000);
			}
		}

		/* Wait until we really stop playing */
		if (!no_outp)
			outp_flush(player_pmng->m_cur_out);

		/* Stop timer thread */
		player_stop_timer();

		/* Send message about track end */
		if (!player_end_track)
			player_next_track();

		/* End playing */
		inp_end(inp);
		player_inp = NULL;
		player_bitrate = player_freq = player_stereo = 0;

		/* End output plugin */
		if (!no_outp)
			outp_end(player_pmng->m_cur_out);

		/* Update screen */
		wnd_invalidate(player_wnd);
	}
	return NULL;
} /* End of 'player_thread' function */

/*****
 *
 * Dialogs launching functions
 * 
 *****/

/* Display song adding dialog box */
void player_add_dialog( void )
{
	dialog_t *dlg;
	filebox_t *eb;

	dlg = dialog_new(wnd_root, _("Add songs"));
	eb = filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("File &name: "), 
			"name", "", 'n', 50);
	eb->m_vfs = player_vfs;
	EDITBOX_OBJ(eb)->m_history = player_hist_lists[PLAYER_HIST_LIST_ADD];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_add);
	dialog_arrange_children(dlg);
} /* End of 'player_add_dialog' function */

/* Display object adding dialog box */
void player_add_obj_dialog( void )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(wnd_root, _("Add object"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("Object &name: "),
			"name", "", 'n', 50);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_ADD_OBJ];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_add_obj);
	dialog_arrange_children(dlg);
} /* End of 'player_add_obj_dialog' function */

/* Display saving dialog box */
void player_save_dialog( void )
{
	dialog_t *dlg;
	filebox_t *eb;

	dlg = dialog_new(wnd_root, _("Save play list"));
	eb = filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("File &name: "), 
			"name", "", 'n', 50);
	EDITBOX_OBJ(eb)->m_history = player_hist_lists[PLAYER_HIST_LIST_SAVE];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_save);
	dialog_arrange_children(dlg);
} /* End of 'player_save_dialog' function */

/* Display external program execution dialog box */
void player_exec_dialog( void )
{
	dialog_t *dlg;
	filebox_t *eb;

	dlg = dialog_new(wnd_root, _("Execute external command"));
	eb = filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("C&ommand: "), 
			"command", "", 'o', 50);
	eb->m_command_box = TRUE;
	EDITBOX_OBJ(eb)->m_history = player_hist_lists[PLAYER_HIST_LIST_EXEC];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_exec);
	dialog_arrange_children(dlg);
} /* End of 'player_exec_dialog' function */

/* Launch sort play list dialog */
void player_sort_dialog( void )
{
	dialog_t *dlg;
	vbox_t *vbox;

	dlg = dialog_new(wnd_root, _("Sort play list"));
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("Sort by"), 0);
	radio_new(WND_OBJ(vbox), _("&Title"), "title", 't', TRUE);
	radio_new(WND_OBJ(vbox), _("&File name"), "file", 'f', FALSE);
	radio_new(WND_OBJ(vbox), _("P&ath and file name"), "path", 'a', FALSE);
	radio_new(WND_OBJ(vbox), _("T&rack"), "track", 'r', FALSE);
	checkbox_new(WND_OBJ(dlg->m_vbox), _("&Only selected area"), 
			"only_sel", 'o', FALSE);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_sort);
	dialog_arrange_children(dlg);
} /* End of 'player_sort_dialog' function */

/* Launch song info dialog */
void player_info_dialog( void )
{
	dialog_t *dlg;
	editbox_t *name, *artist, *album, *year, *track, *comment;
	combo_t *genre;
	genre_list_t *glist;
	label_t *own_info;
	int i, sel_start, sel_end, start, end;
	song_t *s;
	song_info_t *info = NULL;
	bool_t name_diff = FALSE, artist_diff = FALSE, album_diff = FALSE,
		   year_diff = FALSE, comment_diff = FALSE, genre_diff = FALSE,
		   track_diff = FALSE;
	char *file_name;
	wnd_t *vbox;
	hbox_t *hbox;
	button_t *reload;

	/* First create dialog */
	dlg = dialog_new(wnd_root, "");
	hbox = hbox_new(WND_OBJ(dlg->m_vbox), NULL, 1);
	vbox = WND_OBJ(vbox_new(WND_OBJ(hbox), NULL, 0));
	editbox_new_with_label(vbox, _("&Name: "), "name", "", 'n', 
			PLAYER_EB_WIDTH);
	editbox_new_with_label(vbox, _("&Artist: "), "artist", "", 'a', 
			PLAYER_EB_WIDTH);
	editbox_new_with_label(vbox, _("A&lbum: "), "album", "", 'l',
			PLAYER_EB_WIDTH);
	editbox_new_with_label(vbox, _("&Year: "), "year", "", 'y',
			PLAYER_EB_WIDTH);
	editbox_new_with_label(vbox, _("&Track No: "), "track", "", 't',
			PLAYER_EB_WIDTH);
	editbox_new_with_label(vbox, _("C&omments: "), "comments", "", 'o',
			PLAYER_EB_WIDTH);
	combo_new_with_label(vbox, _("&Genre: "), "genre", "", 'g',
			PLAYER_EB_WIDTH, 10);
	label_new(WND_OBJ(hbox), "", "own_data", LABEL_NOBOLD);
	reload = button_new(WND_OBJ(dlg->m_hbox), _("&Reload info"), "reload", 'r');

	/* Fill items with values */
	if (!player_info_dialog_fill(dlg, TRUE))
	{
		wnd_close(WND_OBJ(dlg));
		return;
	}

	/* Display dialog */
	wnd_msg_add_handler(WND_OBJ(reload), "clicked", player_on_info_dlg_reload);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_info);
	wnd_msg_add_handler(WND_OBJ(dlg), "close", player_on_info_close);
	dialog_arrange_children(dlg);
} /* End of 'player_info_dialog' function */

/* Set info edit box value */
void player_info_eb_set( editbox_t *eb, char *val, bool_t diff )
{
	editbox_set_text(eb, val);
	eb->m_modified = FALSE;
	eb->m_gray_non_modified = diff;
} /* End of 'player_info_eb_set' function */

/* Fill info dialog with values */
bool_t player_info_dialog_fill( dialog_t *dlg, bool_t first_call )
{
	editbox_t *name, *artist, *album, *year, *track, *comments;
	combo_t *genre;
	label_t *own_data;
	song_t **songs_list;
	song_t *main_song;
	int num_songs, i, j;
	song_info_t *info;
	char *file_name;
	bool_t name_diff = FALSE, artist_diff = FALSE, album_diff = FALSE,
		   year_diff = FALSE, track_diff = FALSE, comment_diff = FALSE,
		   genre_diff = FALSE;
	genre_list_t *glist;
	assert(dlg);

	/* Generate selected songs list when calling for the first time */
	if (first_call)
	{
		int sel_start, sel_end, start, end;

		/* Lock play list, so it can't be modified while building this list */
		plist_lock(player_plist);

		/* Get selection */
		sel_start = player_plist->m_sel_start;
		sel_end = player_plist->m_sel_end;
		PLIST_GET_SEL(player_plist, start, end);
		if (sel_end < 0 || !player_plist->m_len)
		{
			plist_unlock(player_plist);
			return FALSE;
		}

		/* Add the local flag checkbox */
		if (start != end)
		{
			checkbox_t *cb = checkbox_new(WND_OBJ(dlg->m_vbox), 
					_("Write info in &all the selected songs"),
					"write_in_all", 'a', TRUE);
			wnd_msg_add_handler(WND_OBJ(cb), "clicked", 
					player_on_info_cb_clicked);
		}

		/* Allocate memory for the list */
		songs_list = (song_t **)malloc((end - start + 1) * sizeof(song_t *));
		if (songs_list == NULL)
		{
			plist_unlock(player_plist);
			return FALSE;
		}

		/* Update songs info */
		for ( i = start, num_songs = 0; i <= end; i ++ )
		{
			/* Read info */
			song_t *song = player_plist->m_list[i];
			song_update_info(song);
			if (song->m_info == NULL)
				continue;

			/* Save this song in the list */
			songs_list[num_songs ++] = song_add_ref(song);
			main_song = song;
			info = song->m_info;
		}
		plist_unlock(player_plist);

		/* Check that we have any songs to display */
		if (num_songs == 0)
		{
			free(songs_list);
			return FALSE;
		}

		/* Save the list in dialog data */
		cfg_set_var_ptr(WND_OBJ(dlg)->m_cfg_list, "songs_list", songs_list);
		cfg_set_var_ptr(WND_OBJ(dlg)->m_cfg_list, "main_song", main_song);
		cfg_set_var_int(WND_OBJ(dlg)->m_cfg_list, "num_songs", num_songs);
	}
	/* Get the list */
	else
	{
		songs_list = cfg_get_var_ptr(WND_OBJ(dlg)->m_cfg_list, "songs_list");
		num_songs = cfg_get_var_int(WND_OBJ(dlg)->m_cfg_list, "num_songs");
		main_song = cfg_get_var_ptr(WND_OBJ(dlg)->m_cfg_list, "main_song");
		song_update_info(main_song);
		info = main_song->m_info;
		assert(songs_list && main_song && info && (num_songs > 0));
	}

	/* Compare fields */
	for ( i = 0; i < num_songs; i ++ )
	{
		song_info_t *cur;

		if (songs_list[i] == main_song)
			continue;

		/* Reload info */
		if (!first_call)
			song_update_info(songs_list[i]);
		cur = songs_list[i]->m_info;

		if (!name_diff && strcmp(info->m_name, cur->m_name))
			name_diff = TRUE;
		if (!album_diff && strcmp(info->m_album, cur->m_album))
			album_diff = TRUE;
		if (!artist_diff && strcmp(info->m_artist, cur->m_artist))
			artist_diff = TRUE;
		if (!year_diff && strcmp(info->m_year, cur->m_year))
			year_diff = TRUE;
		if (!track_diff && strcmp(info->m_track, cur->m_track))
			track_diff = TRUE;
		if (!comment_diff && strcmp(info->m_comments, cur->m_comments))
			comment_diff = TRUE;
		if (!genre_diff && strcmp(info->m_genre, cur->m_genre))
			genre_diff = TRUE;
	}

	/* Save the difference flags */
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "name_diff", name_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "artist_diff", artist_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "album_diff", album_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "year_diff", year_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "track_diff", track_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "comment_diff", comment_diff);
	cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "genre_diff", genre_diff);

	/* Get dialog items */
	name = EDITBOX_OBJ(dialog_find_item(dlg, "name"));
	artist = EDITBOX_OBJ(dialog_find_item(dlg, "artist"));
	album = EDITBOX_OBJ(dialog_find_item(dlg, "album"));
	year = EDITBOX_OBJ(dialog_find_item(dlg, "year"));
	track = EDITBOX_OBJ(dialog_find_item(dlg, "track"));
	comments = EDITBOX_OBJ(dialog_find_item(dlg, "comments"));
	genre = COMBO_OBJ(dialog_find_item(dlg, "genre"));
	own_data = LABEL_OBJ(dialog_find_item(dlg, "own_data"));
	assert(name && artist && album && year && track && comments &&
			genre && own_data);

	/* Set items values */
	file_name = cfg_get_var_int(cfg_list, "info-editor-show-full-name") ?
		main_song->m_file_name : util_short_name(main_song->m_file_name);
	wnd_set_title(WND_OBJ(dlg), file_name);
	player_info_eb_set(name, info->m_name, name_diff);
	player_info_eb_set(artist, info->m_artist, artist_diff);
	player_info_eb_set(album, info->m_album, album_diff);
	player_info_eb_set(year, info->m_year, year_diff);
	player_info_eb_set(track, info->m_track, track_diff);
	player_info_eb_set(comments, info->m_comments, comment_diff);
	glist = info->m_glist;
	for ( i = 0; glist != NULL && i < glist->m_size; i ++ )
		combo_add_item(genre, glist->m_list[i].m_name);
	player_info_eb_set(EDITBOX_OBJ(genre), info->m_genre, genre_diff);
	combo_synch_list(genre);
	label_set_text(own_data, info->m_own_data);

	/* Add the special functions buttons */
	if (first_call)
	{
		int num = inp_get_num_specs(main_song->m_inp);
		for ( i = 0; i < num; i ++ )
		{
			char *title = inp_get_spec_title(main_song->m_inp, i);
			button_t *btn = button_new(WND_OBJ(dlg->m_hbox), title, "",
					0);
			cfg_set_var_int(WND_OBJ(btn)->m_cfg_list, "fn_index", i);
			wnd_msg_add_handler(WND_OBJ(btn), "clicked", player_on_info_spec);
		}
	}
	return TRUE;
} /* End of 'player_info_dialog_fill' function */

/* Set gray-non-modified flag for info dialog edit box */
void player_info_dlg_change_eb_gray( dialog_t *dlg, char *id, bool_t gray )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(dlg, id));
	assert(eb);
	eb->m_gray_non_modified = gray;
	wnd_invalidate(WND_OBJ(eb));
} /* End of 'player_info_dlg_change_eb_gray' function */

/* Display search dialog box */
void player_search_dialog( void )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(wnd_root, _("Search"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("S&tring: "), 
			"string", "", 't', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_SEARCH];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_search);
	dialog_arrange_children(dlg);
} /* End of 'player_search_dialog' function */

/* Display mini configuration manager */
void player_var_mini_manager( void )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(wnd_root, _("Mini variables manager"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Name: "),
			"name", "", 'n', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_VAR_NAME];
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Value: "),
			"value", "", 'v', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_VAR_VAL];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_mini_var);
	dialog_arrange_children(dlg);
} /* End of 'player_var_mini_manager' function */

/* Launch advanced search dialog */
void player_advanced_search_dialog( void )
{
	dialog_t *dlg;
	editbox_t *eb;
	vbox_t *vbox;

	dlg = dialog_new(wnd_root, _("Advanced search"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("S&tring: "),
			"string", "", 't', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_SEARCH];
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("Search in"), 0);
	radio_new(WND_OBJ(vbox), _("&Title"), "title", 't', TRUE);
	radio_new(WND_OBJ(vbox), _("&Name"), "name", 'n', FALSE);
	radio_new(WND_OBJ(vbox), _("&Artist"), "artist", 'a', FALSE);
	radio_new(WND_OBJ(vbox), _("A&lbum"), "album", 'l', FALSE);
	radio_new(WND_OBJ(vbox), _("&Year"), "year", 'y', FALSE);
	radio_new(WND_OBJ(vbox), _("&Genre"), "genre", 'g', FALSE);
	radio_new(WND_OBJ(vbox), _("T&rack"), "track", 'r', FALSE);
	radio_new(WND_OBJ(vbox), _("C&omment"), "comment", 'o', FALSE);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_adv_search);
	dialog_arrange_children(dlg);
} /* End of 'player_advanced_search_dialog' function */

/* Process remove song(s) dialog */
void player_rem_dialog( void )
{
	int was = player_plist->m_len;
	
	plist_rem(player_plist);
	logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
			_("Removed %d songs"), was - player_plist->m_len);
} /* End of 'player_rem_dialog' function */

/* Launch info reloading dialog */
void player_info_reload_dialog( void )
{
	dialog_t *dlg;

	dlg = dialog_new(wnd_root, _("Reload info"));
	checkbox_new(WND_OBJ(dlg->m_vbox), _("&Only in selected area"), 
			"only_sel", 'o', FALSE);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_info_reload);
	dialog_arrange_children(dlg);
} /* End of 'player_info_reload_dialog' function */

/* Launch repeat value dialog */
void player_repval_dialog( int dig )
{
	dialog_t *dlg;
	hbox_t *hbox;
	editbox_t *eb;
	char text[2];
	assert(dig >= 0 && dig <= 9);

	dlg = dialog_new(wnd_root, _("Repeat value"));
	hbox = hbox_new(WND_OBJ(dlg->m_vbox), NULL, 0);
	label_new(WND_OBJ(hbox), _("Enter count &value for the next command: "),
			NULL, 0);
	text[0] = dig + '0';
	text[1] = 0;
	player_repval_last_key = 0;
	eb = editbox_new(WND_OBJ(hbox), "count", text, 'v', 10);
	wnd_msg_add_handler(WND_OBJ(eb), "keydown", player_repval_on_keydown);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_repval_on_ok);
	dialog_arrange_children(dlg);
} /* End of 'player_repval_dialog' function */

/* Launch test management dialog */
void player_test_dialog( void )
{
	dialog_t *dlg;
	vbox_t *vbox;
	button_t *btn;

	dlg = dialog_new(wnd_root, _("Run/stop test"));
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("Tests"), 0);
	radio_new(WND_OBJ(vbox), _("Test &1. Window library perfomance"), "1", '1', 
			TRUE);
	btn = button_new(WND_OBJ(dlg->m_hbox), _("&Stop job"), "stop", 's');
	wnd_msg_add_handler(WND_OBJ(btn), "clicked", player_on_test_stop);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_test);
	dialog_arrange_children(dlg);
} /* End of 'player_test_dialog' function */

/*****
 *
 * Dialog messages handlers
 *
 *****/

/* Handle 'ok_clicked' for add songs dialog */
wnd_msg_retcode_t player_on_add( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	assert(eb);
	plist_add(player_plist, EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_add' function */

/* Handle 'ok_clicked' for add object dialog */
wnd_msg_retcode_t player_on_add_obj( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	assert(eb);
	plist_add_obj(player_plist, EDITBOX_TEXT(eb), NULL, -1);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_add_obj' function */

/* Handle 'ok_clicked' for save dialog */
wnd_msg_retcode_t player_on_save( wnd_t *wnd )
{
	bool_t res;
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	assert(eb);
	res = plist_save(player_plist, EDITBOX_TEXT(eb));
	if (res)
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
				_("Play list saved to %s"), EDITBOX_TEXT(eb));
	else
		logger_message(player_log, LOGGER_MSG_ERROR, LOGGER_LEVEL_DEFAULT,
				_("Unable to save play list to %s"), EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_save' function */

/* Handle 'ok_clicked' for execution dialog */
wnd_msg_retcode_t player_on_exec( wnd_t *wnd )
{
	int fd;
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "command"));
	char *text = _("\n\033[0;32;40mPress enter to continue");
	assert(eb);

	/* Close curses for a while */
	wnd_close_curses(wnd_root);

	/* Execute command */
	system(EDITBOX_TEXT(eb));

	/* Display text (mere printf doesn't work) */
	fd = open("/dev/tty", O_WRONLY);
	write(fd, text, strlen(text));
	close(fd);
	getchar();

	/* Restore curses */
	wnd_restore_curses(wnd_root);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_exec' function */

/* Handle 'ok_clicked' for sort dialog */
wnd_msg_retcode_t player_on_sort( wnd_t *wnd )
{
	dialog_t *dlg = DIALOG_OBJ(wnd);
	int by;
	bool_t global;
	if (RADIO_OBJ(dialog_find_item(dlg, "title"))->m_checked)
		by = PLIST_SORT_BY_TITLE;
	else if (RADIO_OBJ(dialog_find_item(dlg, "file"))->m_checked)
		by = PLIST_SORT_BY_NAME;
	else if (RADIO_OBJ(dialog_find_item(dlg, "path"))->m_checked)
		by = PLIST_SORT_BY_PATH;
	else if (RADIO_OBJ(dialog_find_item(dlg, "track"))->m_checked)
		by = PLIST_SORT_BY_TRACK;
	else
		return WND_MSG_RETCODE_OK;
	global = !CHECKBOX_OBJ(dialog_find_item(dlg, "only_sel"))->m_checked;
	plist_sort(player_plist, global, by);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_sort' function */

/* Save the info dialog contents */
void player_save_info_dialog( dialog_t *dlg )
{
	wnd_t *wnd = WND_OBJ(dlg);
	editbox_t *name, *album, *artist, *year, *track, *comments, *genre;
	song_t **songs_list;
	song_t *main_song;
	int num_songs, i;
	bool_t write_in_all;

	/* Get the values */
	name = EDITBOX_OBJ(dialog_find_item(dlg, "name"));
	album = EDITBOX_OBJ(dialog_find_item(dlg, "album"));
	artist = EDITBOX_OBJ(dialog_find_item(dlg, "artist"));
	year = EDITBOX_OBJ(dialog_find_item(dlg, "year"));
	track = EDITBOX_OBJ(dialog_find_item(dlg, "track"));
	comments = EDITBOX_OBJ(dialog_find_item(dlg, "comments"));
	genre = EDITBOX_OBJ(dialog_find_item(dlg, "genre"));
	assert(name && album && artist && year && track && comments && genre);

	/* Get the songs list */
	songs_list = cfg_get_var_ptr(wnd->m_cfg_list, "songs_list");
	main_song = cfg_get_var_ptr(wnd->m_cfg_list, "main_song");
	num_songs = cfg_get_var_int(wnd->m_cfg_list, "num_songs");
	assert(songs_list && main_song && (num_songs > 0));
	if (num_songs > 1)
	{
		checkbox_t *cb = CHECKBOX_OBJ(dialog_find_item(dlg, "write_in_all"));
		assert(cb);
		write_in_all = cb->m_checked;
	}

	/* Save the info */
	for ( i = 0; i < num_songs; i ++ )
	{
		song_info_t *info;

		/* If write-in-all flag is not set, write only in the main song */
		if (!write_in_all && (songs_list[i] != main_song))
			continue;

		/* Prepare the info */
		info = songs_list[i]->m_info;
		assert(info);
		if (name->m_modified)
			si_set_name(info, EDITBOX_TEXT(name));
		if (album->m_modified)
			si_set_album(info, EDITBOX_TEXT(album));
		if (artist->m_modified)
			si_set_artist(info, EDITBOX_TEXT(artist));
		if (year->m_modified)
			si_set_year(info, EDITBOX_TEXT(year));
		if (track->m_modified)
			si_set_track(info, EDITBOX_TEXT(track));
		if (comments->m_modified)
			si_set_comments(info, EDITBOX_TEXT(comments));
		if (genre->m_modified)
			si_set_genre(info, EDITBOX_TEXT(genre));

		/* Save info */
		iwt_push(songs_list[i]);
	}
} /* End of 'player_save_info_dialog' function */

/* Handle 'ok_clicked' for info dialog */
wnd_msg_retcode_t player_on_info( wnd_t *wnd )
{
	player_save_info_dialog(DIALOG_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info' function */

/* Handle 'close' for info dialog */
wnd_msg_retcode_t player_on_info_close( wnd_t *wnd )
{
	/* Free songs list */
	song_t **list = cfg_get_var_ptr(wnd->m_cfg_list, "songs_list");
	if (list != NULL)
	{
		int i;
		int num_songs = cfg_get_var_int(wnd->m_cfg_list, "num_songs");
		for ( i = 0; i < num_songs; i ++ )
			song_free(list[i]);
		free(list);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_close' function */

/* Handle 'clicked' for info dialog reload button */
wnd_msg_retcode_t player_on_info_dlg_reload( wnd_t *wnd )
{
	if (!player_info_dialog_fill(DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog), FALSE))
		wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_dlg_reload' function */

/* Handle 'clicked' for info dialog special function button */
wnd_msg_retcode_t player_on_info_spec( wnd_t *wnd )
{
	dlgitem_t *di;
	wnd_t *dlg;
	checkbox_t *cb;
	song_t **songs_list, *main_song;
	int num_songs;
	bool_t write_in_all = FALSE;
	int i, index;

	/* Get songs list */
	di = DLGITEM_OBJ(wnd);
	dlg = di->m_dialog;
	songs_list = cfg_get_var_ptr(dlg->m_cfg_list, "songs_list");
	main_song = cfg_get_var_ptr(dlg->m_cfg_list, "main_song");
	num_songs = cfg_get_var_int(dlg->m_cfg_list, "num_songs");
	assert(songs_list && main_song && (num_songs > 0));

	/* Get the function index */
	index = cfg_get_var_int(wnd->m_cfg_list, "fn_index");

	/* Save info if need */
	if (inp_get_spec_flags(main_song->m_inp, index) & INP_SPEC_SAVE_INFO)
		player_save_info_dialog(DIALOG_OBJ(dlg));

	/* Execute special function */
	cb = CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(dlg), "write_in_all"));
	if (cb != NULL)
		write_in_all = (cb->m_checked);
	for ( i = 0; i < num_songs; i ++ )
	{
		in_plugin_t *inp = songs_list[i]->m_inp;
		if ((!write_in_all && (songs_list[i] != main_song)) ||
				(inp != main_song->m_inp))
			continue;
		inp_spec_func(inp, index, songs_list[i]->m_file_name);
	}

	/* Reload the info */
	if (!player_info_dialog_fill(DIALOG_OBJ(dlg), FALSE))
		wnd_close(dlg);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_spec' function */

/* Handle 'clicked' for info dialog write-in-all checkbox */
wnd_msg_retcode_t player_on_info_cb_clicked( wnd_t *wnd )
{
	editbox_t *name, *artist, *album, *year, *track, *comments, *genre;
	dialog_t *dlg = DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog);
	bool_t not_check = !CHECKBOX_OBJ(wnd)->m_checked;

	/* Invert the gray-non-missed flags for the edit boxes */
	player_info_dlg_change_eb_gray(dlg, "name", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "name_diff"));
	player_info_dlg_change_eb_gray(dlg, "artist", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "artist_diff"));
	player_info_dlg_change_eb_gray(dlg, "album", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "album_diff"));
	player_info_dlg_change_eb_gray(dlg, "year", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "year_diff"));
	player_info_dlg_change_eb_gray(dlg, "track", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "track_diff"));
	player_info_dlg_change_eb_gray(dlg, "comments", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "comment_diff"));
	player_info_dlg_change_eb_gray(dlg, "genre", not_check ? FALSE :
			cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "genre_diff"));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_cb_clicked' function */

/* Handle 'ok_clicked' for search dialog */
wnd_msg_retcode_t player_on_search( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "string"));
	assert(eb);
	player_set_search_string(EDITBOX_TEXT(eb));
	player_search_criteria = PLIST_SEARCH_TITLE;
	if (!plist_search(player_plist, player_search_string, 1, 
				player_search_criteria))
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
				_("String `%s' not found"), 
				player_search_string);
	else
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
				_("String `%s' found at position %d"), player_search_string,
				player_plist->m_sel_end);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_search' function */

/* Handle 'ok_clicked' for advanced search dialog */
wnd_msg_retcode_t player_on_adv_search( wnd_t *wnd )
{
	dialog_t *dlg = DIALOG_OBJ(wnd);
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(dlg, "string"));
	assert(eb);
	player_set_search_string(EDITBOX_TEXT(eb));

	/* Choose search criteria */
	if (RADIO_OBJ(dialog_find_item(dlg, "title"))->m_checked)
		player_search_criteria = PLIST_SEARCH_TITLE;
	else if (RADIO_OBJ(dialog_find_item(dlg, "name"))->m_checked)
		player_search_criteria = PLIST_SEARCH_NAME;
	else if (RADIO_OBJ(dialog_find_item(dlg, "artist"))->m_checked)
		player_search_criteria = PLIST_SEARCH_ARTIST;
	else if (RADIO_OBJ(dialog_find_item(dlg, "album"))->m_checked)
		player_search_criteria = PLIST_SEARCH_ALBUM;
	else if (RADIO_OBJ(dialog_find_item(dlg, "year"))->m_checked)
		player_search_criteria = PLIST_SEARCH_YEAR;
	else if (RADIO_OBJ(dialog_find_item(dlg, "genre"))->m_checked)
		player_search_criteria = PLIST_SEARCH_GENRE;
	else if (RADIO_OBJ(dialog_find_item(dlg, "track"))->m_checked)
		player_search_criteria = PLIST_SEARCH_TRACK;
	else if (RADIO_OBJ(dialog_find_item(dlg, "comment"))->m_checked)
		player_search_criteria = PLIST_SEARCH_COMMENT;
	else
		return WND_MSG_RETCODE_OK;
	if (!plist_search(player_plist, player_search_string, 1, 
				player_search_criteria))
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
				_("String `%s' not found"), 
				player_search_string);
	else
		logger_message(player_log, LOGGER_MSG_NORMAL, LOGGER_LEVEL_DEFAULT,
				_("String `%s' found at position %d"), player_search_string,
				player_plist->m_sel_end);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_adv_search' function */

/* Handle 'ok_clicked' for info reload dialog */
wnd_msg_retcode_t player_on_info_reload( wnd_t *wnd )
{
	checkbox_t *cb = CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd),
				"only_sel"));
	assert(cb);
	plist_reload_info(player_plist, !cb->m_checked);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_reload' function */

/* Handle 'ok_clicked' for mini variables manager */
wnd_msg_retcode_t player_on_mini_var( wnd_t *wnd )
{
	editbox_t *name = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	editbox_t *val = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "value"));
	assert(name);
	assert(val);
	cfg_set_var(cfg_list, EDITBOX_TEXT(name), EDITBOX_TEXT(val));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_mini_var' function */

/* Handle 'keydown' for repeat value dialog edit box */
wnd_msg_retcode_t player_repval_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Let only digits be entered */
	if ((key >= ' ' && key < '0') || (key > '9' && key <= 0xFF))
	{
		player_repval_last_key = key;
		wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "ok_clicked", 
				dialog_msg_ok_new());
		return WND_MSG_RETCODE_STOP;
	}
	else if (key == KEY_BACKSPACE && EDITBOX_OBJ(wnd)->m_cursor == 0)
	{
		wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "cancel_clicked",
				dialog_msg_cancel_new());
		return WND_MSG_RETCODE_STOP;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_repval_on_keydown' function */

/* Handle 'ok_clicked' for repeat value dialog box */
wnd_msg_retcode_t player_repval_on_ok( wnd_t *wnd )
{
	/* Get count */
	player_repval = 
		atoi(EDITBOX_TEXT(dialog_find_item(DIALOG_OBJ(wnd), "count")));
	if (player_repval_last_key != 0)
		wnd_msg_send(player_wnd, "keydown", 
				wnd_msg_keydown_new(player_repval_last_key));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_repval_on_ok' function */

/* Handle 'clicked' for stop test button */
wnd_msg_retcode_t player_on_test_stop( wnd_t *wnd )
{
	test_stop();
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_test_stop' function */

/* Handle 'ok_clicked' for test dialog box */
wnd_msg_retcode_t player_on_test( wnd_t *wnd )
{
	radio_t *r;
	int sel = -1;

	/* Get selection */
	r = RADIO_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "1"));
	assert(r);
	if (r->m_checked)
		sel = TEST_WNDLIB_PERFOMANCE;
	if (sel < 0)
		return WND_MSG_RETCODE_OK;

	/* Start the test */
	test_start(sel);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_test' function */

/*****
 *
 * Variables change handlers
 *
 *****/

/* Handle 'title_format' variable setting */
bool_t player_handle_var_title_format( cfg_node_t *var, char *value )
{
	int i;

	for ( i = 0; i < player_plist->m_len; i ++ )
		song_update_title(player_plist->m_list[i]);
	wnd_invalidate(wnd_root);
	return TRUE;
} /* End of 'player_handle_var_title_format' function */

/* Handle 'output_plugin' variable setting */
bool_t player_handle_var_outp( cfg_node_t *var, char *value )
{
	int i;

	if (player_pmng == NULL)
		return TRUE;

	/* Choose new output plugin */
	for ( i = 0; i < player_pmng->m_num_outp; i ++ )
		if (!strcmp(player_pmng->m_outp[i]->m_name, 
					cfg_get_var(cfg_list, "output-plugin")))
		{
			player_pmng->m_cur_out = player_pmng->m_outp[i];
			break;
		}
	return TRUE;
} /* End of 'player_handle_var_outp' function */

/* Handle 'color-scheme' variable setting */
bool_t player_handle_color_scheme( cfg_node_t *node, char *value )
{
	char fname[MAX_FILE_NAME];
	
	/* Read configuration from respective file */
	snprintf(fname, sizeof(fname), "%s/.mpfc/colors/%s", 
			getenv("HOME"), cfg_get_var(cfg_list, node->m_name));
	player_read_rcfile(cfg_list, fname);
	return TRUE;
} /* End of 'player_handle_color_scheme' function */

/* Handle 'kbind-scheme' variable setting */
bool_t player_handle_kbind_scheme( cfg_node_t *node, char *value )
{
	char fname[MAX_FILE_NAME];
	
	/* Read configuration from respective file */
	snprintf(fname, sizeof(fname), "%s/.mpfc/kbinds/%s", 
			getenv("HOME"), cfg_get_var(cfg_list, node->m_name));
	player_read_rcfile(cfg_list, fname);
	return TRUE;
} /* End of 'player_handle_kbind_scheme' function */

/*****
 *
 * Player window class functions
 *
 *****/

/* Initialize class */
wnd_class_t *player_wnd_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "player", 
			wnd_basic_class_init(global), NULL, 
			player_class_set_default_styles);
	return klass;
} /* End of 'player_wnd_class_init' function */

/* Set player class default styles */
void player_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "plist-style", "white:black");
	cfg_set_var(list, "plist-playing-style", "red:black:bold");
	cfg_set_var(list, "plist-selected-style", "white:blue:bold");
	cfg_set_var(list, "plist-sel-and-play-style", "red:blue:bold");
	cfg_set_var(list, "plist-time-style", "green:black:bold");
	cfg_set_var(list, "title-style", "yellow:black:bold");
	cfg_set_var(list, "time-style", "green:black:bold");
	cfg_set_var(list, "audio-params-style", "green:black:bold");
	cfg_set_var(list, "about-style", "green:black:bold");
	cfg_set_var(list, "slider-style", "cyan:black:bold");
	cfg_set_var(list, "play-modes-style", "green:black:bold");
	cfg_set_var(list, "status-style", "red:black");
} /* End of 'player_class_set_default_styles' function */

/*****
 *
 * Miscellaneous functions
 *****/

/* Set a new search string */
void player_set_search_string( char *str )
{
	if (player_search_string != NULL)
		free(player_search_string);
	player_search_string = strdup(str);
} /* End of 'player_set_search_string' function */

/* Set mark */
void player_set_mark( char m )
{
	player_marks[m - 'a'] = player_plist->m_sel_end;
} /* End of 'player_set_mark' function */

/* Go to mark */
void player_goto_mark( char m )
{
	int pos = player_marks[m - 'a'];
	
	if (pos >= 0)
		plist_move(player_plist, pos, FALSE);
} /* End of 'player_goto_mark' function */

/* Go back in play list */
void player_go_back( void )
{
	if (player_last_pos >= 0)
	{
		plist_move(player_plist, player_last_pos, FALSE);
	}
} /* End of 'player_go_back' function */

/* Return to the last time */
void player_time_back( void )
{
	player_play(player_last_song, player_last_song_time);
} /* End of 'player_time_back' function */

/* End of 'player.c' file */

