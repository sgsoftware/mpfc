/******************************************************************
 * Copyright (C) 2003 - 2013 by SG Software.
 *
 * SG MPFC. Main player functions implementation.
 * $Id$
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
#include <string.h>
#include <unistd.h>
#include <gst/gst.h>
#include <gst/audio/streamvolume.h>
#include <gst/audio/audio.h>
#include <json-glib/json-glib.h>
#include "types.h"
#include "browser.h"
#include "cfg.h"
#include "command.h"
#include "help_screen.h"
#include "json_helpers.h"
#include "logger.h"
#include "logger_view.h"
#include "main_types.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "server.h"
#include "test.h"
#include "undo.h"
#include "util.h"
#include "wnd.h"
#include "wnd_button.h"
#include "wnd_checkbox.h"
#include "wnd_combobox.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"
#include "wnd_label.h"
#include "wnd_listbox.h"
#include "wnd_multiview_dialog.h"
#include "wnd_radio.h"
#include "wnd_root.h"
#include "wnd_repval.h"
#include "info_rw_thread.h"
#include "genp.h"

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

/* Search string and criteria */
char *player_search_string = NULL;
int player_search_criteria = PLIST_SEARCH_TITLE;

/* Message text */
char *player_msg = NULL;

/* Player thread ID */
pthread_t player_tid = 0;

/* Player termination flag */
volatile bool_t player_end_thread = FALSE;

/* Timer termination flag */
volatile bool_t player_end_track = FALSE;

/* Player context */
player_context_t *player_context = NULL;

bool_t player_end_of_stream = FALSE;
GstElement *player_pipeline = NULL;

/* Edit boxes history lists */
editbox_history_t *player_hist_lists[PLAYER_NUM_HIST_LISTS];

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
int player_last_song = -1;
song_time_t player_last_song_time = -1;

/* Plugins manager */
pmng_t *player_pmng = NULL;

/* User configuration file name */
char player_cfg_file[MAX_FILE_NAME] = "";
char player_cfg_autosave_file[MAX_FILE_NAME] = "";
char player_cfg_dir[MAX_FILE_NAME] = "";

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
#define PLAYER_EB_WIDTH	(2 * WND_WIDTH(player_wnd) / 3)

/* enqueued songs */
int queued_songs[PLAYER_MAX_ENQUEUED];
int num_queued_songs = 0;

/* Main thread ID */
pthread_t player_main_tid = 0; 

#define VOLUME_SLIDER_RANGE (VOLUME_DEF * 2)

/* Forward decls */
static void player_info_dialog_set_if_readonly( dialog_t *dlg );
static bool_t player_handle_var_title_format( cfg_node_t *var, char *value, void *data );
static bool_t player_handle_color_scheme( cfg_node_t *var, char *value, void *data );
static bool_t player_handle_kbind_scheme( cfg_node_t *var, char *value, void *data );
static void player_audio_setup_dlg( void );
static void player_welcome_dialog( void );
static void player_utf8_dialog( void );

/*****
 *
 * Initialization/deinitialization functions
 *
 *****/

/* Load player state */
static void player_load_state( void )
{
	char *fname = util_strcat(getenv("HOME"), "/.mpfc/state", NULL);
	if (!fname)
		goto finally_fname;

	/* Parse */
	JsonParser *parser = json_parser_new();
	if (!json_parser_load_from_file(parser, fname, NULL))
	{
		logger_error(player_log, 1, "unable to parse player state");
		goto finally_fname;
	}

	JsonNode *root_node = json_parser_get_root(parser);
	if (!root_node || !JSON_NODE_HOLDS_OBJECT(root_node))
		goto finally_js;
	JsonObject *js_root = json_node_get_object(root_node);

	/* Load playlist */
	JsonArray *js_plist = js_get_array(js_root, "plist");
	if (js_plist)
		plist_import_from_json(player_plist, js_plist);

	/* Start playing from last stop */
	if (cfg_get_var_int(cfg_list, "play-from-stop"))
	{
		logger_debug(player_log, "Playing from stop");
		player_context->m_status = js_get_int(js_root, "player-status", PLAYER_STATUS_STOPPED);
		player_start = js_get_int(js_root, "player-start", 0) - 1;
		player_end = js_get_int(js_root, "player-end", 0) - 1;
		if (player_context->m_status != PLAYER_STATUS_STOPPED)
			player_play(js_get_int(js_root, "cur-song", -1),
					js_get_int(js_root, "cur-time", 0));
		player_context->m_volume = js_get_double(js_root, "volume", VOLUME_DEF);
	}

finally_js:
	g_object_unref(parser);

finally_fname:
	free(fname);
}

/* Save player state */
static void player_save_state( void )
{
	JsonObject *js_root = json_object_new();

	/* Save playlist */
	if (player_plist && cfg_get_var_int(cfg_list, "save-playlist-on-exit"))
	{
		json_object_set_array_member(js_root, "plist", plist_export_to_json(player_plist));
	}

	/* Save player state */
	json_object_set_int_member(js_root, "cur-song", player_plist->m_cur_song);
	json_object_set_int_member(js_root, "cur-time", player_context->m_cur_time);
	json_object_set_int_member(js_root, "player-status", player_context->m_status);
	json_object_set_int_member(js_root, "player-start", player_start + 1);
	json_object_set_int_member(js_root, "player-end", player_end + 1);
	json_object_set_double_member(js_root, "volume", player_context->m_volume);

	/* Save to a file */
	JsonGenerator *gen = json_generator_new();
	json_generator_set_root(gen, js_make_node(js_root));
	char *fname = util_strcat(getenv("HOME"), "/.mpfc/state", NULL);
	json_generator_to_file(gen, fname, NULL);

	/* Save some stuff through the cfg system */
	player_save_cfg();
}

/* Initialize player */
bool_t player_init( int argc, char *argv[] )
{
	int i;
	plist_set_t *set;
	time_t t;
	char *str_time;
	bool_t is_utf8 = TRUE;

	/* Set signal handlers */
	player_main_tid = pthread_self();
	signal(SIGINT, player_handle_signal);
	signal(SIGTERM, player_handle_signal);
	signal(SIGPIPE, player_handle_signal);
	
	/* Initialize configuration */
	snprintf(player_cfg_dir, sizeof(player_cfg_dir), 
			"%s/.mpfc", getenv("HOME"));
	snprintf(player_cfg_file, sizeof(player_cfg_file), 
			"%s/mpfcrc", player_cfg_dir);
	snprintf(player_cfg_autosave_file, sizeof(player_cfg_autosave_file), 
			"%s/autosave", player_cfg_dir);
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
	t = time(NULL);
	str_time = ctime(&t);
	logger_status_msg(player_log, 0, _("MPFC %s Log\n%s"), VERSION, str_time);
//	free(str_time);

	/* Initialize context */
	player_context = (player_context_t *)malloc(sizeof(*player_context));
	if (player_context == NULL)
	{
		logger_fatal(player_log, 0, _("No enough memory"));
		return FALSE;
	}
	player_context->m_cur_time = 0;
	player_context->m_bitrate = 0;
	player_context->m_freq = 0;
	player_context->m_depth = 0;
	player_context->m_channels = 0;
	player_context->m_status = PLAYER_STATUS_STOPPED;
	player_context->m_volume = VOLUME_DEF;

	/* Parse command line */
	logger_debug(player_log, "In player_init");
	logger_debug(player_log, "Parsing command line");
	if (!player_parse_cmd_line(argc, argv))
		return FALSE;

	/* Initialize window system */
	logger_debug(player_log, "Initializing window system");
	wnd_root = wnd_init(cfg_list, player_log);
	if (wnd_root == NULL)
	{
		logger_fatal(player_log, 0, _("Window system initialization failed"));
		return FALSE;
	}
	wnd_msg_add_handler(wnd_root, "destructor", player_root_destructor);

	/* Check that we have an utf8 locale */
	if (!util_check_utf8_mode())
	{
		is_utf8 = FALSE;
		logger_fatal(player_log, 0, _("Your locale is not UTF-8! Text handling will work incorrectly"));
	}

	/* Initialize play list window */
	logger_debug(player_log, "Initializing play list window");
	player_wnd = WND_OBJ(player_wnd_new(wnd_root));
	if (player_wnd == NULL)
	{
		logger_fatal(player_log, 0, _("Unable to initialize play list window"));
		return FALSE;
	}
	logger_attach_handler(player_log, player_on_log_msg, NULL);

	/* Initialize file browser directory */
	if (getcwd(player_fb_dir, sizeof(player_fb_dir)) == NULL)
		strcpy(player_fb_dir, "/");
	else
		strcat(player_fb_dir, "/");
	
	/* Initialize plugin manager */
	logger_debug(player_log, "Initializing plugin manager");
	player_pmng = pmng_init(cfg_list, player_log, wnd_root);
	if (player_pmng == NULL)
	{
		logger_fatal(player_log, 0, _("Unable to initialize plugin manager"));
		return FALSE;
	}
	player_pmng->m_player_wnd = player_wnd;
	player_pmng->m_player_context = player_context;

	/* Initialize info read/write thread */
	logger_debug(player_log, "Initializing info read/write thread");
	if (!irw_init())
	{
		logger_fatal(player_log, 0, 
				_("Unable to initialize info read/write thread"));
		return FALSE;
	}

	/* Initialize undo list */
	logger_debug(player_log, "Initializing undo list");
	player_ul = undo_new();
	if (player_ul == NULL)
	{
		logger_debug(player_log, 0, _("Unable to initialize undo list"));
	}

	/* Create a play list and add files to it */
	logger_debug(player_log, "Initializing play list");
	player_plist = plist_new(3);
	if (player_plist == NULL)
	{
		logger_fatal(player_log, 0, _("Play list initialization failed"));
		return FALSE;
	}
	player_pmng->m_playlist = player_plist;

	/* Make a set of files to add */
	logger_debug(player_log, "Initializing play list set");
	set = plist_set_new(FALSE);
	if (set == NULL)
	{
		logger_fatal(player_log, 0,
				_("Unable to initialize set of files for play list"));
		return FALSE;
	}
	for ( i = 0; i < player_num_files; i ++ )
		plist_set_add(set, player_files[i]);
	plist_add_set(player_plist, set);
	plist_set_free(set);

	/* Load saved play list if files list is empty */
	logger_debug(player_log, "Adding list.m3u");
	if (!player_num_files)
		player_load_state();

	/* Initialize history lists */
	logger_debug(player_log, "Initializing history");
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
		player_hist_lists[i] = editbox_history_new();

	/* Initialize playing thread */
	logger_debug(player_log, "Initializing player thread");
	if (pthread_create(&player_tid, NULL, player_thread, NULL))
	{
		logger_fatal(player_log, 0, _("Unable to initialize player thread"));
		return FALSE;
	}

	/* Initialize marks */
	for ( i = 0; i < PLAYER_NUM_MARKS; i ++ )
		player_marks[i] = -1;

	/* Start server */
	server_start();

	/* Exit */
	logger_message(player_log, 0, _("Player initialized"));

	/* Show startup dialogs */
	if (!is_utf8)
		player_utf8_dialog();
	player_welcome_dialog();

	return TRUE;
} /* End of 'player_init' function */

/* Initialize the player window */
player_wnd_t *player_wnd_new( wnd_t *parent )
{
	player_wnd_t *pwnd;
	wnd_t *wnd;

	/* Allocate memory */
	pwnd = (player_wnd_t *)malloc(sizeof(*pwnd));
	if (pwnd == NULL)
		return NULL;
	memset(pwnd, 0, sizeof(*pwnd));
	wnd = WND_OBJ(pwnd);
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
	wnd_msg_add_handler(wnd, "action", player_on_action);
	wnd_msg_add_handler(wnd, "close", player_on_close);
	wnd_msg_add_handler(wnd, "mouse_ldown", player_on_mouse_ldown);
	wnd_msg_add_handler(wnd, "mouse_mdown", player_on_mouse_mdown);
	wnd_msg_add_handler(wnd, "mouse_ldouble", player_on_mouse_ldouble);
	wnd_msg_add_handler(wnd, "user", player_on_user);

	/* Set fields */
	wnd->m_cursor_hidden = TRUE;
	wnd_postinit(wnd);
	return pwnd;
} /* End of 'player_wnd_new' function */

/* Root window destructor */
void player_root_destructor( wnd_t *wnd )
{
	logger_debug(player_log, "In player_root_destructor");

	/* Save player state */
	player_save_state();
	
	/* End playing thread */
	logger_debug(player_log, "Doing irw_free");
	irw_free();
	logger_debug(player_log, "Setting next song to NULL");
	if (player_tid)
	{
		logger_debug(player_log, "Stopping player thread");
		player_end_play(FALSE);
		player_end_track = TRUE;
		player_end_thread = TRUE;
		pthread_join(player_tid, NULL);
		logger_debug(player_log, "Player thread terminated");
		player_end_thread = FALSE;
		player_tid = 0;
	}
	
	/* Stop general plugins */
	pmng_stop_general_plugins(player_pmng);

	player_wnd = NULL;
	wnd_root = NULL;
	logger_debug(player_log, "player_root_destructor done");
} /* End of 'player_root_destructor' function */

/* Unitialize player */
void player_deinit( void )
{
	int i;

	logger_debug(player_log, "In player_deinit");

	/* Stop server */
	server_stop();

	/* Uninitialize plugin manager */
	logger_debug(player_log, "Doing pmng_free");
	pmng_free(player_pmng);
	player_pmng = NULL;

	/* Uninitialize history lists */
	logger_debug(player_log, "Freeing history list");
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
	{
		editbox_history_free(player_hist_lists[i]);
		player_hist_lists[i] = NULL;
	}

	/* Free memory allocated for strings */
	logger_debug(player_log, "Freeing strings");
	if (player_search_string != NULL)
	{
		free(player_search_string);
		player_search_string = NULL;
	}
	
	/* Destroy all objects */
	if (player_plist != NULL)
	{
		logger_debug(player_log, "Destroying play list");
		plist_free(player_plist);
		player_plist = NULL;
	}
	logger_debug(player_log, "Freeing undo information");
	undo_free(player_ul);
	player_ul = NULL;
	if (player_context != NULL)
	{
		free(player_context);
		player_context = NULL;
	}

	/* Free logger */
	if (player_log != NULL)
	{
		logger_debug(player_log, "Stopping logger");
		logger_message(player_log, 0, _("Left MPFC"));
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
		int name_start, name_end, val_start;
		cfg_var_op_t op = CFG_VAR_OP_SET;

		/* Include file specification */
		if (argv[i][1] == 'f')
		{
			bool_t full_name = FALSE;
			char name[MAX_FILE_NAME];

			/* Full name specification */
			if (argv[i][2] == 'f' && argv[i][3] == 0)
				full_name = TRUE;
			/* Unknown key */
			else if (argv[i][2] != 0)
				continue;

			/* Get name */
			i ++;
			if (i >= argc)
				break;
			if (full_name)
				snprintf(name, sizeof(name), "%s", argv[i]);
			else 
				snprintf(name, sizeof(name), "%s/.mpfc/%s", getenv("HOME"),
						argv[i]);
			cfg_rcfile_read(cfg_list, name);
			continue;
		}
		
		/* Get variable name start */
		for ( name_start = 0; str[name_start] == '-'; name_start ++ );
		if (name_start >= strlen(str))
			continue;

		/* Get variable name end */
		for ( name_end = name_start; 
				str[name_end] && str[name_end] != '=' && 
				str[name_end] != ' '; name_end ++ )
		{
			val_start = name_end + 2;
			if ((str[name_end] == '+' || str[name_end] == '-')
					&& str[name_end + 1] == '=')
			{
				op = (str[name_end] == '+' ? CFG_VAR_OP_ADD : CFG_VAR_OP_REM);
				break;
			}
		}
		name_end --;

		/* Extract variable name */
		name = strndup(&str[name_start], name_end - name_start + 1);

		/* We have no value - assume it "1" */
		if (str[name_end + 1] == 0)
		{
			val = strdup("1");
		}
		/* Extract value */
		else
		{
			val = strdup(&str[val_start]);
		}
		
		/* Set respective variable */
		cfg_set_var_full(cfg_list, name, val, op);
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
	cfg_new_var(cfg_list, "cur-song-title", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "cur-time", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-status", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-start", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "player-end", CFG_NODE_RUNTIME, NULL, NULL);
	cfg_new_var(cfg_list, "title-format", 0, NULL, 
			player_handle_var_title_format);
	cfg_new_var(cfg_list, "color-scheme", 0, NULL,
			player_handle_color_scheme);
	cfg_new_var(cfg_list, "kbind-scheme", 0, NULL, 
			player_handle_kbind_scheme);

	/* Set default variable values */
	log_file = util_strcat(getenv("HOME"), "/.mpfc/log", NULL);
	cfg_set_var(cfg_list, "log-file", log_file);
	free(log_file);
	cfg_set_var_int(cfg_list, "save-playlist-on-exit", 1);
	cfg_set_var_int(cfg_list, "play-from-stop", 1);
	cfg_set_var(cfg_list, "lib-dir", LIBDIR"/mpfc");
	cfg_set_var_bool(cfg_list, "autosave-plugins-params", TRUE);
	cfg_set_var_bool(cfg_list, "search-nocase", TRUE);
	cfg_set_var_bool(cfg_list, "view-follows-cur-song", TRUE);

	/* Read configuration files */
	cfg_rcfile_read(cfg_list, player_cfg_autosave_file);
	cfg_rcfile_read(cfg_list, SYSCONFDIR"/mpfcrc");
	cfg_rcfile_read(cfg_list, player_cfg_file);
	cfg_rcfile_read(cfg_list, ".mpfcrc");
	return TRUE;
} /* End of 'player_init_cfg' function */

/* Save some variables to file */
void player_save_cfg( void )
{
	/* See if ~/.mpfc/ directory exists */
	DIR *dir = opendir(player_cfg_dir);
	if (!dir)
	{
		logger_message(player_log, 1, "creating directory %s\n", player_cfg_dir);
		mkdir(player_cfg_dir, 0770);
	}
	else
		closedir(dir);

	/* Open file */
	FILE *fd = fopen(player_cfg_autosave_file, "wt");
	if (fd == NULL)
		return;

	/* Save plugins parameters if need */
	if (cfg_get_var_bool(cfg_list, "autosave-plugins-params"))
	{
		cfg_rcfile_save_node(fd, cfg_search_node(cfg_list, "plugins"), NULL);
		cfg_rcfile_save_node(fd, cfg_search_node(cfg_list, "gstreamer"), NULL);
	}

	/* Save 'dont_show' values for startup information dialogs */
	cfg_rcfile_save_node(fd, cfg_search_node(cfg_list, "dont_show"), NULL);

	fclose(fd);
} /* End of 'player_save_cfg' function */

/* Signal handler */
void player_handle_signal( int signum )
{
	if (pthread_self() != player_main_tid)
		return;
	if (signum == SIGINT || signum == SIGTERM)
	{
		wnd_close(wnd_root);
		//player_deinit();
	}
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

/* Handle action */
wnd_msg_retcode_t player_on_action( wnd_t *wnd, char *action, int repval )
{
	bool_t long_jump = FALSE;
	int rp;

	/* Clear message string */
	player_msg = NULL;

	int was_pos = player_plist->m_sel_end;
	int was_song = player_plist->m_cur_song;
	song_time_t was_time = player_context->m_cur_time;

	if (repval > 0)
		rp = repval;
	else
		rp = player_repval;

	/* Exit MPFC */
	if (!strcasecmp(action, "quit"))
	{
		wnd_close(wnd_root);
	}
	/* Queue up a song */
	else if (!strcasecmp(action, "queue"))
	{
		player_queue_song();
	}
	/* Show help screen */
	else if (!strcasecmp(action, "help"))
	{
		help_new(wnd_root, HELP_PLAYER);
	}
	/* Move cursor down */
	else if (!strcasecmp(action, "move_down"))
	{
		plist_move(player_plist, (rp == 0) ? 1 : rp, 
				TRUE);
	}
	/* Move cursor up */
	else if (!strcasecmp(action, "move_up"))
	{
		plist_move(player_plist, (rp == 0) ? -1 : -rp, 
				TRUE);
	}
	/* Move cursor screen down */
	else if (!strcasecmp(action, "screen_down"))
	{
		plist_move(player_plist, (rp == 0) ? 
				PLIST_HEIGHT : 
				PLIST_HEIGHT * rp, TRUE);
	}
	/* Move cursor screen up */
	else if (!strcasecmp(action, "screen_up"))
	{
		plist_move(player_plist, (rp == 0) ? 
				-PLIST_HEIGHT : 
				-PLIST_HEIGHT * rp, TRUE);
	}
	/* Move cursor to play list begin */
	else if (!strcasecmp(action, "move_to_begin"))
	{
		plist_move(player_plist, 0, FALSE);
	}
	/* Move cursor to play list end */
	else if (!strcasecmp(action, "move_to_end"))
	{
		plist_move(player_plist, player_plist->m_len - 1, FALSE);
	}
	/* Move cursor to any position */
	else if (!strcasecmp(action, "move"))
	{
		plist_move(player_plist, (rp == 0) ? 
				player_plist->m_len - 1 : rp - 1, FALSE);
		long_jump = TRUE;
	}
	/* Seek song forward */
	else if (!strcasecmp(action, "time_fw"))
	{
		player_seek(SECONDS_TO_TIME((rp == 0) ? 10 : 10 * rp), TRUE);
	}
	/* Seek song backward */
	else if (!strcasecmp(action, "time_bw"))
	{
		player_seek(SECONDS_TO_TIME((rp == 0) ? -10 : -10 * rp), TRUE);
	}
	/* Long seek song forward */
	else if (!strcasecmp(action, "time_long_fw"))
	{
		player_seek(SECONDS_TO_TIME((rp == 0) ? 60 : 60 * rp), TRUE);
	}
	/* Long seek song backwards */
	else if (!strcasecmp(action, "time_long_bw"))
	{
		player_seek(SECONDS_TO_TIME((rp == 0) ? -60 : -60 * rp), TRUE);
	}
	/* Seek to any position */
	else if (!strcasecmp(action, "time_move"))
	{
		player_seek(SECONDS_TO_TIME((rp == 0) ? 0 : rp), FALSE);
	}
	/* Increase volume */
	else if (!strcasecmp(action, "vol_fw"))
	{
		player_set_vol((rp == 0) ? VOLUME_STEP : VOLUME_STEP * rp, TRUE);
	}
	/* Decrease volume */
	else if (!strcasecmp(action, "vol_bw"))
	{
		player_set_vol((rp == 0) ? -VOLUME_STEP : -VOLUME_STEP * rp, TRUE);
	}
	/* Set volume to default */
	else if (!strcasecmp(action, "vol_def"))
	{
		player_set_vol(VOLUME_DEF, FALSE);
	}
	/* Centrize view */
	else if (!strcasecmp(action, "centrize"))
	{
		plist_centrize(player_plist, -1);
		long_jump = TRUE;
	}
	/* Enter visual mode */
	else if (!strcasecmp(action, "visual"))
	{
		player_plist->m_visual = !player_plist->m_visual;
	}
	/* Resume playing */
	else if (!strcasecmp(action, "play"))
	{
		if (player_context->m_status != PLAYER_STATUS_PAUSED)
			player_play(player_plist->m_cur_song, 0);
		player_context->m_status = PLAYER_STATUS_PLAYING;

		pmng_hook(player_pmng, "player-status");
	}
	/* Pause */
	else if (!strcasecmp(action, "pause"))
	{
		player_pause_resume();
	}
	/* Stop */
	else if (!strcasecmp(action, "stop"))
	{
		player_stop();
	}
	/* Play song */
	else if (!strcasecmp(action, "start_play"))
	{
		if (player_plist->m_len > 0)
		{
			player_start_play(player_plist->m_sel_end, 0);
		}
	}
	/* Go to next song */
	else if (!strcasecmp(action, "next"))
	{
		player_skip_songs((rp) ? rp : 1, TRUE);
	}
	/* Go to previous song */
	else if (!strcasecmp(action, "prev"))
	{
		player_skip_songs(-((rp) ? rp : 1), TRUE);
	}
	/* Add a file */
	else if (!strcasecmp(action, "add"))
	{
		player_add_dialog();
	}
	/* Save play list */
	else if (!strcasecmp(action, "save"))
	{
		player_save_dialog();
	}
	/* Remove song(s) */
	else if (!strcasecmp(action, "rem"))
	{
		player_rem_dialog();
	}
	/* Sort play list */
	else if (!strcasecmp(action, "sort"))
	{
		player_sort_dialog();
	}
	/* Song info dialog */
	else if (!strcasecmp(action, "info"))
	{
		player_info_dialog();
	}
	/* Search */
	else if (!strcasecmp(action, "search"))
	{
		player_search_dialog();
	}
	/* Advanced search */
	else if (!strcasecmp(action, "advanced_search"))
	{
		player_advanced_search_dialog();
	}
	/* Find next/previous search match */
	else if (!strcasecmp(action, "next_match") ||
			!strcasecmp(action, "prev_match"))
	{
		if (!plist_search(player_plist, player_search_string, 
					(action[0] == 'n' || action[0] == 'N') ? 1 : -1, 
					player_search_criteria))
			logger_message(player_log, 1, _("String `%s' not found"), 
					player_search_string);
		else
			logger_message(player_log, 1,
					_("String `%s' found at position %d"), player_search_string,
					player_plist->m_sel_end);
		long_jump = TRUE;
	}
	/* Set/unset shuffle mode */
	else if (!strcasecmp(action, "shuffle"))
	{
		cfg_set_var_int(cfg_list, "shuffle-play",
				!cfg_get_var_int(cfg_list, "shuffle-play"));
	}
	/* Set/unset loop mode */
	else if (!strcasecmp(action, "loop"))
	{
		cfg_set_var_int(cfg_list, "loop-play",
				!cfg_get_var_int(cfg_list, "loop-play"));
	}
	/* Variables manager */
	else if (!strcasecmp(action, "var_manager"))
	{
		player_var_manager();
	}
	/* Audio output setup */
	else if (!strcasecmp(action, "audio_setup"))
	{
		player_audio_setup_dlg();
	}
	/* Move play list down */
	else if (!strcasecmp(action, "plist_down"))
	{
		plist_move_sel(player_plist, (rp == 0) ? 1 : rp, 
				TRUE);
	}
	/* Move play list up */
	else if (!strcasecmp(action, "plist_up"))
	{
		plist_move_sel(player_plist, (rp == 0) ? -1 : 
				-rp, TRUE);
	}
	/* Move play list */
	else if (!strcasecmp(action, "plist_move"))
	{
		plist_move_sel(player_plist, (rp == 0) ? 
				1 : rp - 1, FALSE);
	}
	/* Undo */
	else if (!strcasecmp(action, "undo"))
	{
		undo_bw(player_ul);
	}
	/* Redo */
	else if (!strcasecmp(action, "redo"))
	{
		undo_fw(player_ul);
	}
	/* Reload info */
	else if (!strcasecmp(action, "reload_info"))
	{
		player_info_reload_dialog();
	}
	/* Set play boundaries */
	else if (!strcasecmp(action, "set_play_bounds"))
	{
		PLIST_GET_SEL(player_plist, player_start, player_end);
	}
	/* Clear play boundaries */
	else if (!strcasecmp(action, "clear_play_bounds"))
	{
		player_start = player_end = -1;
	}
	/* Set play boundaries and play from area beginning */
	else if (!strcasecmp(action, "play_bounds"))
	{
		PLIST_GET_SEL(player_plist, player_start, player_end);
		if (player_plist->m_len > 0)
		{
			player_context->m_status = PLAYER_STATUS_PLAYING;
			player_play(player_start, 0);
		}
	}
	/* Execute outer command */
	else if (!strcasecmp(action, "exec"))
	{
		player_exec_dialog();
	}
	/* Go back */
	else if (!strcasecmp(action, "time_back"))
	{
		player_time_back();
	}
	/* Set mark */
	else if (!strncasecmp(action, "mark", 4))
	{
		char letter = action[4];
		if (letter >= 'a' && letter <= 'z')
			player_set_mark(letter);
	}
	/* Go back */
	else if (!strcasecmp(action, "goback"))
	{
		player_go_back();
	}
	/* Go to mark */
	else if (!strncasecmp(action, "go", 2))
	{
		char letter = action[2];
		if (letter >= 'a' && letter <= 'z')
			player_goto_mark(letter);
	}
	/* Launch file browser */
	else if (!strcasecmp(action, "file_browser"))
	{
		fb_new(wnd_root, player_fb_dir);
	}
	/* Launch test dialog */
	else if (!strcasecmp(action, "test"))
	{
		player_test_dialog();
	}
	/* Launch plugins manager */
	else if (!strcasecmp(action, "plugins_manager"))
	{
		player_pmng_dialog();
	}
	/* Launch logger view */
	else if (!strcasecmp(action, "log"))
	{
		if (player_logview == NULL)
		{
			player_logview = logview_new(wnd_root, player_log);
			wnd_msg_add_handler(WND_OBJ(player_logview), "close", 
					player_logview_on_close);
		}
	}
	/* Digit means command repeation value edit */
	else if (!strncasecmp(action, "dig_", 4))
	{
		char dig = action[4];
		if (dig >= '0' && dig <= '9')
			wnd_repval_new(wnd_root, player_repval_on_ok, dig - '0');
	}

	/* Flush repeat value */
	player_repval = 0;

	/* Save last position */
	if (long_jump && (player_plist->m_sel_end != was_pos))
		player_last_pos = was_pos;

	/* Save last time */
/*	if (player_plist->m_cur_song != was_song || player_context->m_cur_time != was_time)
	{
		player_last_song = was_song;
		player_last_song_time = was_time;
	}*/

	/* Repaint */
	wnd_invalidate(player_wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_handle_action' function */

/* Handle left-button click */
wnd_msg_retcode_t player_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Move cursor in play list */
	if (y >= player_plist->m_start_pos && 
			y < player_plist->m_start_pos + PLIST_HEIGHT)
	{
		int was_pos = player_plist->m_sel_end;
		plist_move(player_plist, y - player_plist->m_start_pos + 
				player_plist->m_scrolled, FALSE);
		if (was_pos != player_plist->m_sel_end)
			player_last_pos = was_pos;
		wnd_invalidate(wnd);
	}
	/* Set volume */
	else if (y == PLAYER_SLIDER_VOL_Y && x >= PLAYER_SLIDER_VOL_X &&
				x <= PLAYER_SLIDER_VOL_X + PLAYER_SLIDER_VOL_W)
	{
		player_set_vol((x - PLAYER_SLIDER_VOL_X) * VOLUME_SLIDER_RANGE / 
				PLAYER_SLIDER_VOL_W, FALSE);
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
		int was_pos = player_plist->m_sel_end;
		plist_centrize(player_plist, y - player_plist->m_start_pos + 
				player_plist->m_scrolled);
		if (was_pos != player_plist->m_sel_end)
			player_last_pos = was_pos;
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
			player_context->m_status = PLAYER_STATUS_PLAYING;
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
		plist_move(player_plist, (int)((intptr_t)data), FALSE);
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
	char aparams[256], *aparams_ptr;

	/* Display head */
	wnd_move(wnd, 0, 0, 0);
	if (player_plist->m_cur_song == -1)
	{
		char *shuffle_str, *loop_str;

		/* Print about */
		wnd_move(wnd, 0, 0, 0);
		wnd_apply_style(wnd, "about-style");
		wnd_printf(wnd, 0, 0, _("SG Software Media Player For Console"));

		/* Set root title */
		wnd_set_global_title("MPFC");
		
		/* Print shuffle mode */
		shuffle_str = _("Shuffle");
		loop_str = _("Loop");
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - utf8_width(shuffle_str) - 
					utf8_width(loop_str) - 2, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, shuffle_str);
		}
		
		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - utf8_width(loop_str) - 1, 0);
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
		bool_t show_rem;
		char *shuffle_str, *loop_str;
		
		/* Print current song title */
		s = player_plist->m_list[player_plist->m_cur_song];
		wnd_move(wnd, 0, 0, 0);
		wnd_apply_style(wnd, "title-style");
		wnd_printf(wnd, WND_PRINT_ELLIPSES, WND_WIDTH(wnd) - 1, "%s", 
				STR_TO_CPTR(s->m_title));

		/* Set root title */
		char *root_title = util_strcat("MPFC :: ", STR_TO_CPTR(s->m_title), NULL);
		wnd_set_global_title(root_title);
		free(root_title);
		
		/* Print shuffle mode */
		shuffle_str = _("Shuffle");
		loop_str = _("Loop");
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - utf8_width(shuffle_str) - 
					utf8_width(loop_str) - 2, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, shuffle_str);
		}
		
		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - utf8_width(loop_str) - 1, 0);
			wnd_apply_style(wnd, "play-modes-style");
			wnd_printf(wnd, 0, 0, loop_str);
		}

		/* Print current time */
		song_time_t t = (show_rem = cfg_get_var_int(cfg_list, "show-time-remaining")) ? 
			s->m_len - player_context->m_cur_time : player_context->m_cur_time;
		int ts = TIME_TO_SECONDS(t);
		int l = TIME_TO_SECONDS(s->m_len);
		wnd_move(wnd, 0, 0, 1);
		wnd_apply_style(wnd, "time-style");
		wnd_printf(wnd, 0, 0, "%s%i:%02i/%i:%02i\n", 
				show_rem ? "-" : "", ts / 60, ts % 60, l / 60, l % 60);
	}

	/* Display current audio parameters */
	strcpy(aparams, "");
	aparams_ptr = aparams;
	if (player_context->m_bitrate)
		aparams_ptr += sprintf(aparams_ptr, "%d kbps ", player_context->m_bitrate);
	if (player_context->m_freq)
		aparams_ptr += sprintf(aparams_ptr, "%d Hz ", player_context->m_freq);
	if (player_context->m_depth)
		aparams_ptr += sprintf(aparams_ptr, "%db ", player_context->m_depth);
	if (player_context->m_channels)
	{
		switch (player_context->m_channels)
		{
		case 1:
			aparams_ptr += sprintf(aparams_ptr, "Mono");
			break;
		case 2:
			aparams_ptr += sprintf(aparams_ptr, "Stereo");
			break;
		case 4:
			aparams_ptr += sprintf(aparams_ptr, "Quadro");
			break;
		default:
			aparams_ptr += sprintf(aparams_ptr, "%d-chan", player_context->m_channels);
			break;
		}
	}
	wnd_move(wnd, 0, WND_WIDTH(player_wnd) - utf8_width(aparams) - 1, 
			PLAYER_SLIDER_VOL_Y - 1);
	wnd_apply_style(wnd, "audio-params-style");
	wnd_printf(wnd, 0, 0, "%s", aparams);

	/* Display various slidebars */
	player_display_slider(wnd, PLAYER_SLIDER_TIME_X, PLAYER_SLIDER_TIME_Y, 
			PLAYER_SLIDER_TIME_W, player_context->m_cur_time, (s == NULL) ? -1 : s->m_len);
	player_display_slider(wnd, PLAYER_SLIDER_VOL_X, PLAYER_SLIDER_VOL_Y,
			PLAYER_SLIDER_VOL_W, player_context->m_volume, VOLUME_SLIDER_RANGE);
	
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
		double pos, double range )
{
	wnd_move(wnd, 0, x, y);
	int slider_pos = (range >= 0) ? (pos * width / range) : 0;
	wnd_apply_style(wnd, "slider-style");
	for ( int i = 0; i <= width; i ++ )
	{
		if (i == slider_pos)
			wnd_putchar(wnd, 0, 'O');
		else 
			wnd_put_special(wnd, WND_ACS_CODE(WACS_HLINE));
	}
} /* End of 'player_display_slider' function */

/* Handle new log message */
void player_on_log_msg( logger_t *log, void *data, 
		struct logger_message_t *msg )
{
	/* Print message to status line */
	player_msg = msg->m_message;
	wnd_invalidate(player_wnd);

	/* Add to logger view */
	if (player_logview != NULL)
	{
		scrollable_set_size(SCROLLABLE_OBJ(player_logview), 
				player_log->m_num_messages);
		wnd_invalidate(WND_OBJ(player_logview));
	}
} /* End of 'player_on_log_msg' function */

/*****
 *
 * Playing-related functions
 *
 *****/

/* Seek song */
void player_seek( song_time_t val, bool_t rel )
{
	if (player_plist->m_cur_song == -1)
		return;

	song_t *s = player_plist->m_list[player_plist->m_cur_song];

	song_time_t new_time = (rel ? (player_context->m_cur_time + val) : val);
	if (new_time < 0)
		new_time = 0;
	else if (new_time > s->m_len)
		new_time = s->m_len;

	player_save_time();
	guint64 tm = player_translate_time(s, new_time, TRUE);
	logger_debug(player_log, "gstreamer: seeking to time %lld", tm);
	if (!gst_element_seek(player_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
			GST_SEEK_TYPE_SET, tm, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
	{
		logger_error(player_log, 1, "gstreamer: gst_element_seek returned FALSE");
	}
	player_context->m_cur_time = new_time;
	wnd_invalidate(player_wnd);
	logger_debug(player_log, "after player_seek timer is %lld", player_context->m_cur_time);

	pmng_hook(player_pmng, "player-status");
} /* End of 'player_seek' function */

/* Play song */
void player_play( int song, song_time_t start_time )
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
	cfg_set_var(cfg_list, "cur-song-name", song_get_short_name(s));
	cfg_set_var(cfg_list, "cur-song-title", STR_TO_CPTR(s->m_title));
	player_plist->m_cur_song = song;
	player_context->m_cur_time = start_time;
//	player_context->m_status = PLAYER_STATUS_PLAYING;

	/* Move cursor to current song */
	if (cfg_get_var_bool(cfg_list, "view-follows-cur-song") && 
			(song < player_plist->m_scrolled ||
			 song >= player_plist->m_scrolled + PLIST_HEIGHT))
	{
		int was_pos = player_plist->m_sel_end;
		plist_centrize(player_plist, -1);
		if (was_pos != player_plist->m_sel_end)
			player_last_pos = was_pos;
	}
} /* End of 'player_play' function */

/* End playing song */
void player_end_play( bool_t rem_cur_song )
{
	int was_song = player_plist->m_cur_song;
	
	player_save_time();
	player_plist->m_cur_song = -1;
	player_end_track = TRUE;
//	player_context->m_status = PLAYER_STATUS_STOPPED;
	if (!rem_cur_song)
		player_plist->m_cur_song = was_song;
	cfg_set_var(cfg_list, "cur-song-name", "");
	cfg_set_var(cfg_list, "cur-song-title", "");
} /* End of 'player_end_play' function */

/* Go to next track */
void player_next_track( void )
{
	int next_track;
	
	next_track = player_skip_songs(1, FALSE);
	player_set_track(next_track);
} /* End of 'player_next_track' function */

/* Start track */
void player_set_track( int track )
{
	if (track < 0)
		player_end_play(TRUE);
	else
		player_play(track, 0);
	pmng_hook(player_pmng, "player-status");
} /* End of 'player_set_track' function */

/* Set volume */
void player_set_vol( double vol, bool_t rel )
{
	player_context->m_volume = (rel) ? player_context->m_volume + vol : vol;
	if (player_context->m_volume < VOLUME_MIN)
		player_context->m_volume = VOLUME_MIN;
	else if (player_context->m_volume > VOLUME_MAX)
		player_context->m_volume = VOLUME_MAX;
	player_update_vol();

	pmng_hook(player_pmng, "volume");
} /* End of 'player_set_vol' function */

/* Update volume */
void player_update_vol( void )
{
	if (player_pipeline)
	{
		gdouble v = player_context->m_volume;
		gdouble conv = gst_stream_volume_convert_volume(
				GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, v);
		logger_message(player_log, 1, "setting volume to %lg%% (linear = %lg%%)",
				v * 100, conv * 100);
		g_object_set(G_OBJECT(player_pipeline), "volume", conv, NULL);
	}
} /* End of 'player_update_vol' function */

/* Skip some songs */
int player_skip_songs( int num, bool_t play )
{
	int len, base, song;
	
	if (player_plist == NULL || !player_plist->m_len)
		return -1;
	
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
	if(num_queued_songs != 0)
	{
		int queue_loop;
		song = queued_songs[0];
		for(queue_loop = 0;queue_loop < num_queued_songs-1;queue_loop++)
		{
			queued_songs[queue_loop] = queued_songs[queue_loop+1];
		}
		num_queued_songs--;
	}

	/* Start or end play */
	if (play)
	{
		if (song == -1)
			player_end_play(TRUE);
		else
			player_play(song, 0);
		pmng_hook(player_pmng, "player-status");
	}
	return song;
} /* End of 'player_skip_songs' function */

/* Translate projected song time to real time */
song_time_t player_translate_time( song_t *s, song_time_t t, bool_t virtual2real )
{
	if (s == NULL || s->m_start_time < 0)
		return t;
	return (virtual2real ? s->m_start_time + t : t - s->m_start_time);
} /* End of 'player_translate_time' function */

/* Handle a TAG message */
static void player_handle_tag_msg( GstMessage *msg )
{
	GstTagList *tags = NULL;
	gst_message_parse_tag(msg, &tags);
	if (!tags)
		return;

	/* We are looking for bitrate changes.
	 * Actually it seems gstreamer doesn't report _current_ bitrate so
	 * this will be constant for a track */
	unsigned bitrate;
	if (gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &bitrate))
	{
		bitrate /= 1000;
		if (player_context->m_bitrate != bitrate)
		{
			player_context->m_bitrate = bitrate;
			wnd_invalidate(player_wnd);
		}
	}
	gst_tag_list_free(tags);
} /* End of 'player_handle_tag_msg' function */

/* GStreamer bus message handler */
static gboolean player_gst_bus_call( GstBus *bus, GstMessage *msg, gpointer data )
{
	gchar *debug;
	GError *error;

	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS:
		logger_debug(player_log, "gstreamer: EOS message arrived");
		player_end_of_stream = TRUE;
		break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);

		logger_error(player_log, 1, "gstreamer error: %s", error->message);
		g_error_free(error);
		break;

	case GST_MESSAGE_TAG:
		player_handle_tag_msg(msg);
		break;
	}

	return TRUE;
} /* End of 'player_gst_bus_call' function */

/* Handle audio caps setting */
static void player_on_caps_set(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	GstCaps *caps = gst_pad_get_current_caps(GST_PAD(obj));
	if (!caps)
		return;

	GstAudioInfo info;
	gst_audio_info_init(&info);
	if (gst_audio_info_from_caps(&info, caps))
	{
		int rate = GST_AUDIO_INFO_RATE(&info);
		int channels = GST_AUDIO_INFO_CHANNELS(&info);
		int depth = GST_AUDIO_INFO_DEPTH(&info);
		if (player_context->m_freq != rate ||
				player_context->m_channels != channels ||
				player_context->m_depth != depth)
		{
			player_context->m_freq = rate;
			player_context->m_channels = channels;
			player_context->m_depth = depth;
			wnd_invalidate(player_wnd);
		}
	}

	gst_caps_unref(caps);
} /* End of 'player_on_caps_set' function */

/* Handle 'audio-changed' signal */
static void player_on_audio_changed( GstElement *playbin, gpointer user_data )
{
	GstPad *pad = NULL;
	g_signal_emit_by_name(playbin, "get-audio-pad", 0, &pad);
	if (!pad)
		return;
	g_signal_connect(pad, "notify::caps", (GCallback)player_on_caps_set, NULL);
} /* End of 'player_on_audio_changed' function */

/* Player thread function */
bool_t player_set_audio_sink( void )
{
	char *audio_sink_name = cfg_get_var(cfg_list, "gstreamer.audio-sink");
	if (audio_sink_name)
	{
		GstElement *sink = gst_element_factory_make(audio_sink_name, "sink");
		if (sink)
		{
			cfg_node_t *cfg_node = cfg_search_node(cfg_list, "gstreamer.audio-sink-params");
			if (cfg_node)
			{
				cfg_list_iterator_t iter = cfg_list_begin_iteration(cfg_node);
				for ( ;; )
				{
					cfg_node_t *cfg_node = cfg_list_iterate(&iter);
					if (!cfg_node)
						break;

					if (CFG_NODE_IS_VAR(cfg_node))
					{
						char *name = cfg_node->m_name;
						char *val = CFG_VAR_VALUE(cfg_node);

						logger_debug(player_log, "gstreamer: setting audio sink param %s to %s",
								name, val);
						g_object_set(G_OBJECT(sink), name, val, NULL);
					}
				}
			}
			logger_debug(player_log, "gstreamer: setting audio sink to %s", audio_sink_name);
			g_object_set(G_OBJECT(player_pipeline), "audio-sink", sink, NULL);
		}
		else
		{
			logger_error(player_log, 1, _("Audio sink %s could not be created"), 
					audio_sink_name);
			return FALSE;
		}
	}

	return TRUE;
}

/* Player thread function */
void *player_thread( void *arg )
{
	logger_debug(player_log, "In player_thread");

	/* Main loop */
	while (!player_end_thread)
	{
		song_t *s, *song_played;
		int ch = 0, freq = 0, real_ch = 0, real_freq = 0;
		dword fmt = 0, real_fmt = 0;
		song_info_t si;
		dword in_flags, out_flags;
		bool_t song_finished;
		GMainLoop *loop = NULL;
		GstBus *bus = NULL;
		int was_status;
		char *uri = NULL;

		/* Skip to next iteration if there is nothing to play */
		if (player_plist->m_cur_song < 0 || 
				player_context->m_status == PLAYER_STATUS_STOPPED)
		{
			util_wait();
			continue;
		}

		/* Play track */
		s = player_plist->m_list[player_plist->m_cur_song];
		song_played = s;
		//player_context->m_status = PLAYER_STATUS_PLAYING;
		player_end_track = FALSE;
	
		logger_debug(player_log, "Playing track %s", s->m_fullname);

		/* Get song length and information */
		logger_debug(player_log, "Updating song info");
		song_update_info(s);

		/* Create gstreamer stuff */
		player_end_of_stream = FALSE;
		loop = g_main_loop_new(NULL, FALSE);
		if (!loop)
		{
			logger_error(player_log, 1, "g_main_loop_new failed");
			player_context->m_status = PLAYER_STATUS_STOPPED;
			goto cleanup;
		}
		player_pipeline = gst_element_factory_make("playbin", "play");
		if (!player_pipeline)
		{
			logger_error(player_log, 1, "gstreamer: unable to create playbin");
			player_context->m_status = PLAYER_STATUS_STOPPED;
			goto cleanup;
		}

		/* Set a user-specified audio sink */
		if (!player_set_audio_sink())
		{
			player_context->m_status = PLAYER_STATUS_STOPPED;
			goto cleanup;
		}

		/* Set fake videosink */
		GstElement *videosink = gst_element_factory_make("fakesink", "videosink");
		g_object_set(G_OBJECT(player_pipeline), "video-sink", videosink, NULL);

		/* Set volume */
		player_update_vol();

		/* Set bus message handler */
		bus = gst_pipeline_get_bus(GST_PIPELINE(player_pipeline));
		if (!bus)
		{
			logger_error(player_log, 1, "gst_pipeline_get_bus failed");
		}
		gst_bus_add_watch(bus, player_gst_bus_call, NULL);
		gst_object_unref(bus);
		g_signal_connect(player_pipeline, "audio-changed", (GCallback)player_on_audio_changed, NULL);

		g_object_set(G_OBJECT(player_pipeline), "uri", song_played->m_fullname, NULL);

		/* Start playing */
		gst_element_set_state(player_pipeline, GST_STATE_PLAYING);
		gst_element_get_state(player_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

		/* Seek to start time */
		logger_debug(player_log, "start time is %lld", song_played->m_start_time);
		logger_debug(player_log, "cur_time is %lld", player_context->m_cur_time);
		if (player_context->m_cur_time > 0 || song_played->m_start_time > -1)
		{
			guint64 tm = player_translate_time(song_played, player_context->m_cur_time, TRUE);
			logger_debug(player_log, "gstreamer: seeking to time %lld", tm);
			if (!gst_element_seek(player_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
					GST_SEEK_TYPE_SET, tm, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
			{
				logger_error(player_log, 1, "gstreamer: gst_element_seek returned FALSE");
			}
		}

		/* Play */
		logger_debug(player_log, "Going into track playing cycle");
		song_finished = FALSE;
		was_status = PLAYER_STATUS_PLAYING;
		while (!player_end_track)
		{
			g_main_context_iteration(NULL, FALSE);

			if (player_context->m_status != was_status)
			{
				switch (player_context->m_status)
				{
				case PLAYER_STATUS_PLAYING:
					gst_element_set_state(player_pipeline, GST_STATE_PLAYING);
					break;
				case PLAYER_STATUS_PAUSED:
					gst_element_set_state(player_pipeline, GST_STATE_PAUSED);
					break;
				case PLAYER_STATUS_STOPPED:
					gst_element_set_state(player_pipeline, GST_STATE_READY);
					break;
				}
			}

			if (player_context->m_status == PLAYER_STATUS_PLAYING)
			{
				guint64 tm;

				/* Update time */
				if (gst_element_query_position(player_pipeline, GST_FORMAT_TIME, &tm))
				{
					tm = player_translate_time(song_played, tm, FALSE);
					if (tm != player_context->m_cur_time)
					{
						int was_seconds = TIME_TO_SECONDS(player_context->m_cur_time);
						int new_seconds = TIME_TO_SECONDS(tm);
						player_context->m_cur_time = tm;

						if (was_seconds != new_seconds)
						{
							pmng_hook(player_pmng, "player-time");
							wnd_invalidate(player_wnd);
						}
					}
				}

				/* Check for end of projected song */
				if (song_played->m_end_time > -1 && 
						player_translate_time(song_played, player_context->m_cur_time, TRUE) >= 
						song_played->m_end_time)
				{
					logger_debug(player_log, "stopping at time %lld(%lld) with end_time=%lld.", 
							player_context->m_cur_time,
							player_translate_time(song_played, player_context->m_cur_time, TRUE),
							song_played->m_end_time);
					song_finished = TRUE;
					break;
				}
			}

			if (player_end_of_stream)
			{
				player_end_of_stream = FALSE;
				song_finished = TRUE;
				break;
			}

			was_status = player_context->m_status;
			util_wait();
		}
		logger_debug(player_log, "End playing track");

		gst_element_set_state(player_pipeline, GST_STATE_NULL);

		/* Send message about track end */
		if (!player_end_track)
		{
			logger_debug(player_log, "Going to the next track");
			player_next_track();
		}

		/* End playing */
		player_context->m_bitrate = player_context->m_freq = player_context->m_channels = player_context->m_depth = 0;

		/* Update screen */
		wnd_invalidate(player_wnd);

	cleanup:
		if (player_pipeline)
		{
			gst_object_unref(GST_OBJECT(player_pipeline));
			player_pipeline = NULL;
		}
		if (loop)
		{
			g_main_loop_unref(loop);
			loop = NULL;
		}
	}
	logger_debug(player_log, "Player thread finished");
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
	EDITBOX_OBJ(eb)->m_history = player_hist_lists[PLAYER_HIST_LIST_ADD];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_add);
	dialog_arrange_children(dlg);
} /* End of 'player_add_dialog' function */

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
	editbox_t *path = editbox_new_with_label(vbox, _("Full path: "), "full_path", "", 0,
			PLAYER_EB_WIDTH);
	path->m_editable = FALSE;
	button_t *reload = button_new(WND_OBJ(dlg->m_hbox), _("&Reload info"), "reload", 'r');

	/* Fill items with values */
	if (!player_info_dialog_fill(dlg, TRUE))
	{
		wnd_close(WND_OBJ(dlg));
		return;
	}

	/* Display dialog */
	wnd_msg_add_handler(WND_OBJ(reload), "clicked", player_on_info_dlg_reload);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_info);
	wnd_msg_add_handler(WND_OBJ(dlg), "destructor", player_on_info_close);
	dialog_arrange_children(dlg);
} /* End of 'player_info_dialog' function */

/* Set info edit box value */
void player_info_eb_set( editbox_t *eb, char *val, bool_t diff )
{
	editbox_set_text(eb, val);
	eb->m_modified = FALSE;
	eb->m_gray_non_modified = diff;
} /* End of 'player_info_eb_set' function */

static void player_add_genres(combo_t *cb)
{
	char *genres[] = {
		"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
		"Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
		"Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
		"Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
		"Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
		"Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock",
		"Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
		"Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
		"Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
		"Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
		"Native American", "Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes",
		"Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro",
		"Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk-Rock", "National Folk",
		"Swing", "Fast Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
		"Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
		"Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening",
		"Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata",
		"Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam",
		"Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul",
		"Freestyle", "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House",
		"Dance Hall"
	};

	for (int i = 0, e = sizeof(genres) / sizeof(*genres); i != e; ++i)
		combo_add_item(cb, genres[i]);
}

/* Fill info dialog with values */
bool_t player_info_dialog_fill( dialog_t *dlg, bool_t first_call )
{
	editbox_t *name, *artist, *album, *year, *track, *comments, *full_path_eb;
	combo_t *genre;
	label_t *own_data;
	song_t **songs_list;
	song_t *main_song;
	int num_songs, i, j;
	song_info_t *info;
	bool_t name_diff = FALSE, artist_diff = FALSE, album_diff = FALSE,
		   year_diff = FALSE, track_diff = FALSE, comment_diff = FALSE,
		   genre_diff = FALSE;
	bool_t main_readonly, all_readonly;
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
					_("Write in&fo in all the selected songs"),
					"write_in_all", 'f', TRUE);
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
		all_readonly = TRUE;
		for ( i = start, num_songs = 0; i <= end; i ++ )
		{
			/* Read info */
			song_t *song = player_plist->m_list[i];
			song_update_info(song);
			if (song->m_info == NULL)
				continue;
			if (!(song->m_flags & SONG_STATIC_INFO))
				all_readonly = FALSE;

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

		assert(main_song);
		main_readonly = (main_song->m_flags & SONG_STATIC_INFO);

		/* Save the list in dialog data */
		cfg_set_var_ptr(WND_OBJ(dlg)->m_cfg_list, "songs_list", songs_list);
		cfg_set_var_ptr(WND_OBJ(dlg)->m_cfg_list, "main_song", main_song);
		cfg_set_var_int(WND_OBJ(dlg)->m_cfg_list, "num_songs", num_songs);
		cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "main_readonly", main_readonly);
		cfg_set_var_bool(WND_OBJ(dlg)->m_cfg_list, "all_readonly", all_readonly);

		/* Set fields read-only if necessary */
		player_info_dialog_set_if_readonly(dlg);
	}
	/* Get the list */
	else
	{
		songs_list = cfg_get_var_ptr(WND_OBJ(dlg)->m_cfg_list, "songs_list");
		num_songs = cfg_get_var_int(WND_OBJ(dlg)->m_cfg_list, "num_songs");
		main_song = cfg_get_var_ptr(WND_OBJ(dlg)->m_cfg_list, "main_song");
		main_readonly = cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "main_readonly");
		all_readonly = cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "all_readonly");
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
	full_path_eb = EDITBOX_OBJ(dialog_find_item(dlg, "full_path"));
	assert(name && artist && album && year && track && comments &&
			genre && own_data && full_path_eb);

	/* Set items values */
	wnd_set_title(WND_OBJ(dlg), song_get_short_name(main_song));
	player_info_eb_set(name, info->m_name, name_diff);
	player_info_eb_set(artist, info->m_artist, artist_diff);
	player_info_eb_set(album, info->m_album, album_diff);
	player_info_eb_set(year, info->m_year, year_diff);
	player_info_eb_set(track, info->m_track, track_diff);
	player_info_eb_set(comments, info->m_comments, comment_diff);
	player_add_genres(genre);
	player_info_eb_set(EDITBOX_OBJ(genre), info->m_genre, genre_diff);
	combo_synch_list(genre);
	label_set_text(own_data, info->m_own_data);
	editbox_set_text(full_path_eb, song_get_name(main_song));
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

/* Display variables manager */
void player_var_manager( void )
{
	dialog_t *dlg;
	editbox_t *eb;
	button_t *btn;
	vbox_t *vbox;

	dlg = dialog_new(wnd_root, _("Variables manager"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Name: "),
			"name", "", 'n', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_VAR_NAME];
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Value: "),
			"value", "", 'v', PLAYER_EB_WIDTH);
	eb->m_history = player_hist_lists[PLAYER_HIST_LIST_VAR_VAL];
	btn = button_new(WND_OBJ(dlg->m_hbox), _("Vie&w current value"),
			"", 'w');
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("Operation"), 0);
	radio_new(WND_OBJ(vbox), _("S&et"), "set", 'e', TRUE);
	radio_new(WND_OBJ(vbox), _("&Add"), "add", 'a', FALSE);
	radio_new(WND_OBJ(vbox), _("&Remove"), "rem", 'r', FALSE);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_var);
	wnd_msg_add_handler(WND_OBJ(btn), "clicked", player_on_var_view);
	dialog_arrange_children(dlg);
} /* End of 'player_var_manager' function */

static void player_audio_setup_sync_device_box( wnd_t *wnd )
{
	editbox_t *sink_eb = EDITBOX_OBJ(wnd);
	dialog_t *dlg = DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog);
	assert(dlg);
	editbox_t *dev_eb = EDITBOX_OBJ(dialog_find_item(dlg, "device"));
	assert(dev_eb);

	/* Device is only active when sink is specified */
	dev_eb->m_editable = !EDITBOX_EMPTY(sink_eb);
}

static void player_on_audio_setup( wnd_t *wnd )
{
	editbox_t *sink_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "sink"));
	assert(sink_eb);
	editbox_t *dev_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "device"));
	assert(dev_eb);

	cfg_set_var(cfg_list, "gstreamer.audio-sink", 
			EDITBOX_EMPTY(sink_eb) ? EDITBOX_TEXT(sink_eb) : NULL);
	cfg_set_var(cfg_list, "gstreamer.audio-sink-params.device", 
			EDITBOX_EMPTY(dev_eb) ? EDITBOX_TEXT(dev_eb) : NULL);

	/* Restart playback */
	if (player_plist->m_cur_song >= 0)
		player_play(player_plist->m_cur_song, player_context->m_cur_time);
}

/* Display audio output setup dialog */
static void player_audio_setup_dlg( void )
{
	dialog_t *dlg = dialog_new(wnd_root, _("Audio output setup"));
	editbox_t *sink_eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Sink name: "),
			"sink", "", 's', PLAYER_EB_WIDTH);
	editbox_set_text(sink_eb, cfg_get_var(cfg_list, "gstreamer.audio-sink"));
	editbox_t *dev_eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Device name: "),
			"device", "", 'd', PLAYER_EB_WIDTH);
	editbox_set_text(dev_eb, cfg_get_var(cfg_list, "gstreamer.audio-sink-params.device"));
	player_audio_setup_sync_device_box(WND_OBJ(sink_eb));
	label_new(WND_OBJ(dlg->m_vbox),
			_("\nYou can specify GStreamer sink (e.g. alsasink) and \n"
			  "device here. Device can only be set if sink is specified.\n"
			  "If the sink is configured by some other properties, you can set \n"
			  "them directly with gstreamer.audio-sink-params.* variables.\n"),
			"help", LABEL_NOBOLD);

	wnd_msg_add_handler(WND_OBJ(sink_eb), "changed", player_audio_setup_sync_device_box);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_audio_setup);
	dialog_arrange_children(dlg);
} /* End of 'player_var_manager' function */

static void player_on_startup_msg_ok( wnd_t *wnd )
{
	char *var = cfg_get_var(wnd->m_cfg_list, "dont_show_cfg_name");
	cfg_set_var_bool(cfg_list, var, CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "dont_show"))->m_checked);
}

/* Display audio output setup dialog */
static void player_startup_msg_dialog( char *title, char *text, char *dont_show_cfg_name )
{
	if (cfg_get_var_bool(cfg_list, dont_show_cfg_name))
		return;

	dialog_t *dlg = dialog_new(wnd_root, title);
	label_new(WND_OBJ(dlg->m_vbox), text, "text", LABEL_NOBOLD);
	checkbox_new(WND_OBJ(dlg->m_vbox), _("&Don't show this anymore"), 
			"dont_show", 'd', FALSE);
	cfg_set_var(WND_OBJ(dlg)->m_cfg_list, "dont_show_cfg_name", dont_show_cfg_name);

	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", player_on_startup_msg_ok);
	dialog_arrange_children(dlg);
} /* End of 'player_var_manager' function */

static void player_welcome_dialog( void ) 
{
	player_startup_msg_dialog(_("Welcome"), 
			_("Welcome to MPFC, Music Player For Console!\n"
		      "To close this window, press <Enter>, and then '?' to see key bindings.\n"
			  "For more help please see the full documentation."),
			"dont_show.welcome");
}

static void player_utf8_dialog( void )
{
	player_startup_msg_dialog(_("Error"), 
			_("Your locale is not UTF-8! Text handling will work incorrectly"),
			"dont_show.utf8");
}

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
	int num;
	
	plist_rem(player_plist);
	num = was - player_plist->m_len;
	logger_message(player_log, 1,
			 ngettext("Removed %d song", "Removed %d songs", num), num);
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

/* Data for plugins manager */
enum
{
	PLAYER_PMNG_PLIST = 0,
	PLAYER_PMNG_GENERAL,
	PLAYER_PMNG_COUNT
};
typedef struct 
{
	char *m_title, *m_btn_title, *m_name;
	char m_letter;
	vbox_t *m_view;
	hbox_t *m_hbox;
	listbox_t *m_list;
	vbox_t *m_vbox;
	editbox_t *m_author, *m_desc;
	button_t *m_configure;
	checkbox_t *m_enabled_cb;
	button_t *m_start_stop_btn;
} player_pmng_view_t; 

/* Launch plugin manager dialog */
void player_pmng_dialog( void )
{
	mview_dialog_t *mvd;
	pmng_iterator_t iter;
	player_pmng_view_t *views;
	checkbox_t *cb;
	button_t *reload_btn, *start_stop_btn;
	int i;

	/* Initialize basic views data */
	views = (player_pmng_view_t *)malloc(sizeof(*views) * PLAYER_PMNG_COUNT);
	memset(views, 0, sizeof(*views) * PLAYER_PMNG_COUNT);
	views[PLAYER_PMNG_PLIST].m_title = _("Playlist");
	views[PLAYER_PMNG_PLIST].m_btn_title = _("&Playlist");
	views[PLAYER_PMNG_PLIST].m_name = "playlist";
	views[PLAYER_PMNG_PLIST].m_letter = 'p';
	views[PLAYER_PMNG_GENERAL].m_title = _("General");
	views[PLAYER_PMNG_GENERAL].m_btn_title = _("&General");
	views[PLAYER_PMNG_GENERAL].m_name = "general";
	views[PLAYER_PMNG_GENERAL].m_letter = 'g';

	/* Create dialog and views */
	mvd = mview_dialog_new(wnd_root, _("Plugins manager"));
	/* reload_btn = button_new(WND_OBJ(mvd), _("Reload plugins"), "reload", 'r');
	wnd_msg_add_handler(WND_OBJ(DIALOG_OBJ(mvd)->m_hbox), "clicked", 
			player_pmng_dialog_on_reload);*/
	cfg_set_var_ptr(WND_OBJ(mvd)->m_cfg_list, "views", views);
	wnd_msg_add_handler(WND_OBJ(mvd), "destructor",
			player_pmng_dialog_destructor);
	for ( i = 0; i < PLAYER_PMNG_COUNT; i ++ )
	{
		player_pmng_view_t *v = &views[i];
		v->m_view = vbox_new(WND_OBJ(mvd->m_views), v->m_title, 0);
		mview_dialog_add_view(mvd, v->m_view, v->m_btn_title, v->m_letter);
	}

	/* Create boxes */
	for ( i = 0; i < PLAYER_PMNG_COUNT; i ++ )
	{
		char list_id[20];
		player_pmng_view_t *v = &views[i];

		v->m_hbox = hbox_new(WND_OBJ(v->m_view), NULL, 1);
		snprintf(list_id, sizeof(list_id), "%s_list", v->m_name);
		v->m_list = listbox_new(WND_OBJ(v->m_hbox), NULL, list_id, 'l', 0, 20, 10);
		wnd_msg_add_handler(WND_OBJ(v->m_list), "changed", 
				player_pmng_dialog_on_list_change);
		v->m_vbox = vbox_new(WND_OBJ(v->m_hbox), NULL, 0);
	}

	/* Fill plugin lists */
	iter = pmng_start_iteration(player_pmng, PLUGIN_TYPE_ALL);
	for ( ;; )
	{
		int index = 0;
		plugin_t *p = pmng_iterate(&iter);
		if (p == NULL)
			break;

		if (p->m_type == PLUGIN_TYPE_PLIST)
			index = PLAYER_PMNG_PLIST;
		else if (p->m_type == PLUGIN_TYPE_GENERAL)
			index = PLAYER_PMNG_GENERAL;
		listbox_add(views[index].m_list, p->m_name, p);
	}

	/* Set plugins info controls */
	for ( i = 0; i < PLAYER_PMNG_COUNT; i ++ )
	{
		char desc_id[20], author_id[20], conf_id[20];
		player_pmng_view_t *v = &views[i];

		snprintf(desc_id, sizeof(desc_id), "%s_desc", v->m_name);
		v->m_desc = editbox_new_with_label(WND_OBJ(v->m_view), 
				_("&Description: "), desc_id, "", 'd', 70);
		v->m_desc->m_editable = FALSE;
		snprintf(author_id, sizeof(author_id), "%s_author", v->m_name);
		v->m_author = editbox_new_with_label(WND_OBJ(v->m_view), 
				_("&Author: "), author_id, "", 'a', 70);
		v->m_author->m_editable = FALSE;
		snprintf(conf_id, sizeof(conf_id), "%s_conf", v->m_name);
		v->m_configure = button_new(WND_OBJ(v->m_vbox), _("Con&figure"), 
				conf_id, 'f');
		wnd_msg_add_handler(WND_OBJ(v->m_configure), "clicked",
				player_pmng_dialog_on_configure);
	}
	views[PLAYER_PMNG_GENERAL].m_start_stop_btn = start_stop_btn =
		button_new(WND_OBJ(views[PLAYER_PMNG_GENERAL].m_vbox), 
				_("S&tart"), "start", 't');
	wnd_msg_add_handler(WND_OBJ(start_stop_btn), "clicked",
			player_pmng_dialog_on_start_stop_general);
	player_pmng_dialog_sync(DIALOG_OBJ(mvd));

	/* Set proper focus branches */
	dialog_arrange_children(DIALOG_OBJ(mvd));
	for ( i = PLAYER_PMNG_COUNT - 1; i >= 0; i -- )
		wnd_set_focus(WND_OBJ(views[i].m_list));
} /* End of 'player_pmng_dialog' function */

/* Synchronize plugin manager dialog items with current item */
void player_pmng_dialog_sync( dialog_t *dlg )
{
	int i;
	player_pmng_view_t *views = 
		(player_pmng_view_t *)cfg_get_var_ptr(WND_OBJ(dlg)->m_cfg_list,
			"views");
	assert(views);

	/* Synchronize views */
	for ( i = 0; i < PLAYER_PMNG_COUNT; i ++ )
	{
		player_pmng_view_t *v = &views[i];
		int index = v->m_list->m_cursor;
		plugin_t *p;

		/* Get info */
		if (!v->m_list->m_list_size)
			continue;
		p = (plugin_t *)v->m_list->m_list[index].m_data;
		char *author = plugin_get_author(p);
		char *desc = plugin_get_desc(p);

		/* Set labels */
		editbox_set_text(v->m_author, author == NULL ? "" : author);
		editbox_set_text(v->m_desc, desc == NULL ? "" : desc);

		if (i == PLAYER_PMNG_GENERAL)
		{
			bool_t started = genp_is_started(GENERAL_PLUGIN(p));
			wnd_set_title(WND_OBJ(v->m_start_stop_btn), started ? _("S&top") :
					_("S&tart"));
		}
	}
	wnd_invalidate(WND_OBJ(dlg));
} /* End of 'player_pmng_dialog_sync' function */

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

/* Handle 'ok_clicked' for save dialog */
wnd_msg_retcode_t player_on_save( wnd_t *wnd )
{
	bool_t res;
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	assert(eb);
	res = plist_save(player_plist, EDITBOX_TEXT(eb));
	if (res)
		logger_message(player_log, 1,
				_("Play list saved to %s"), EDITBOX_TEXT(eb));
	else
		logger_error(player_log, 1,
				_("Unable to save play list to %s (try naming it as a .m3u file)"), 
					EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_save' function */

/* Handle 'ok_clicked' for execution dialog */
wnd_msg_retcode_t player_on_exec( wnd_t *wnd )
{
	int fd;
	int len;
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "command"));
	char *text = _("\n\033[0;32;40mPress enter to continue");
	assert(eb);

	/* Close curses for a while */
	wnd_close_curses(wnd_root);

	/* Execute command */
	if (system(EDITBOX_TEXT(eb)) < 0)
	{
		logger_error(player_log, 0, _("Command execution failed"));
	}

	/* Display text (mere printf doesn't work) */
	fd = open("/dev/tty", O_WRONLY);
	len = strlen(text);
	if (write(fd, text, len) != len)
	{
		logger_error(player_log, 0, _("Writing to console failed"));
	}
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
	bool_t write_in_all = FALSE;

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
		song_update_title(songs_list[i]);
		wnd_invalidate(player_wnd);

		/* Save info */
		irw_push(songs_list[i], SONG_INFO_WRITE);
	}
} /* End of 'player_save_info_dialog' function */

/* Handle 'ok_clicked' for info dialog */
wnd_msg_retcode_t player_on_info( wnd_t *wnd )
{
	player_save_info_dialog(DIALOG_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info' function */

/* Handle 'close' for info dialog */
void player_on_info_close( wnd_t *wnd )
{
	logger_debug(player_log, "in player_on_info_close");

	/* Free songs list */
	song_t **list = cfg_get_var_ptr(wnd->m_cfg_list, "songs_list");
	logger_debug(player_log, "songs list is %p", list);
	if (list != NULL)
	{
		int i;
		int num_songs = cfg_get_var_int(wnd->m_cfg_list, "num_songs");
		logger_debug(player_log, "number of songs is %d", num_songs);
		for ( i = 0; i < num_songs; i ++ )
		{
			logger_debug(player_log, "freeing %p", list[i]);
			song_free(list[i]);
		}
		logger_debug(player_log, "freeing list");
		free(list);
	}
	logger_debug(player_log, "player_on_info_close done");
} /* End of 'player_on_info_close' function */

/* Handle 'clicked' for info dialog reload button */
wnd_msg_retcode_t player_on_info_dlg_reload( wnd_t *wnd )
{
	if (!player_info_dialog_fill(DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog), FALSE))
		wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_dlg_reload' function */

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

	/* Set fields read-only if necessary */
	player_info_dialog_set_if_readonly(dlg);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_info_cb_clicked' function */

/* Update read-only status of info dialog fields */
static void player_info_dialog_set_if_readonly( dialog_t *dlg )
{
	bool_t in_all = FALSE;
	dlgitem_t *in_all_cb = dialog_find_item(dlg, "write_in_all");
	if (in_all_cb)
		in_all = CHECKBOX_OBJ(in_all_cb)->m_checked;

	bool_t is_readonly = in_all ?
		cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "all_readonly") :
		cfg_get_var_bool(WND_OBJ(dlg)->m_cfg_list, "main_readonly");

	EDITBOX_OBJ(dialog_find_item(dlg, "name"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "artist"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "album"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "year"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "track"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "comments"))->m_editable = !is_readonly;
	EDITBOX_OBJ(dialog_find_item(dlg, "genre"))->m_editable = !is_readonly;
} /* End of 'player_info_dialog_set_if_readonly' function */

/* Handle 'ok_clicked' for search dialog */
wnd_msg_retcode_t player_on_search( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "string"));
	int was_pos = player_plist->m_sel_end;
	assert(eb);
	player_set_search_string(EDITBOX_TEXT(eb));
	player_search_criteria = PLIST_SEARCH_TITLE;
	if (!plist_search(player_plist, player_search_string, 1, 
				player_search_criteria))
		logger_message(player_log, 1, _("String `%s' not found"), 
				player_search_string);
	else
		logger_message(player_log, 1,
				_("String `%s' found at position %d"), player_search_string,
				player_plist->m_sel_end);
	if (was_pos != player_plist->m_sel_end)
		player_last_pos = was_pos;
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_search' function */

/* Handle 'ok_clicked' for advanced search dialog */
wnd_msg_retcode_t player_on_adv_search( wnd_t *wnd )
{
	dialog_t *dlg = DIALOG_OBJ(wnd);
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(dlg, "string"));
	int was_pos = player_plist->m_sel_end;
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
		logger_message(player_log, 1, _("String `%s' not found"), 
				player_search_string);
	else
		logger_message(player_log, 1,
				_("String `%s' found at position %d"), player_search_string,
				player_plist->m_sel_end);
	if (was_pos != player_plist->m_sel_end)
		player_last_pos = was_pos;
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

/* Handle 'ok_clicked' for variables manager */
wnd_msg_retcode_t player_on_var( wnd_t *wnd )
{
	editbox_t *name = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	editbox_t *val = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "value"));
	radio_t *set = RADIO_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "set"));
	radio_t *add = RADIO_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "add"));
	radio_t *rem = RADIO_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "rem"));
	cfg_var_op_t op = CFG_VAR_OP_SET;
	assert(name && val && set && add && rem);

	if (set->m_checked)
		op = CFG_VAR_OP_SET;
	else if (add->m_checked)
		op = CFG_VAR_OP_ADD;
	else if (rem->m_checked)
		op = CFG_VAR_OP_REM;
	cfg_set_var_full(cfg_list, EDITBOX_TEXT(name), EDITBOX_TEXT(val), op);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_var' function */

/* Handle 'clicked' for variables manager view value button */
wnd_msg_retcode_t player_on_var_view( wnd_t *wnd )
{
	char *value;
	editbox_t *eb_value = (editbox_t *)dialog_find_item(
			DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog), "value");
	editbox_t *eb_name = (editbox_t *)dialog_find_item(
			DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog), "name");
	assert(eb_value && eb_name);
	value = cfg_get_var(cfg_list, EDITBOX_TEXT(eb_name));
	editbox_set_text(eb_value, value == NULL ? "" : value);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_var_view' function */

/* Handle 'ok_clicked' for repeat value dialog box */
wnd_msg_retcode_t player_repval_on_ok( wnd_t *wnd )
{
	int last_key;

	/* Get count */
	player_repval = 
		atoi(EDITBOX_TEXT(dialog_find_item(DIALOG_OBJ(wnd), "count")));
	last_key = cfg_get_var_int(WND_OBJ(wnd)->m_cfg_list, "last-key");
	if (last_key != 0)
		wnd_msg_send(player_wnd, "keydown", wnd_msg_keydown_new(last_key));
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

/* Handle 'close' message for logger view */
wnd_msg_retcode_t player_logview_on_close( wnd_t *wnd )
{
	player_logview = NULL;
	return WND_MSG_RETCODE_OK;
} /* End of 'player_logview_on_close' function */

/* Handle 'changed' message for plugins manager list boxes */
wnd_msg_retcode_t player_pmng_dialog_on_list_change( wnd_t *wnd, int index )
{
	player_pmng_dialog_sync(DIALOG_OBJ(DLGITEM_OBJ(wnd)->m_dialog));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_pmng_dialog_on_list_change' function */

/* Handle 'clicked' message for plugins manager configure buttons */
wnd_msg_retcode_t player_pmng_dialog_on_configure( wnd_t *wnd )
{
	player_pmng_view_t *v = NULL;
	player_pmng_view_t *views; 
	wnd_t *dlg;
	plugin_t *p;
	int i, index;

	/* Determine our view */
	dlg = DLGITEM_OBJ(wnd)->m_dialog;
	views = (player_pmng_view_t *)cfg_get_var_ptr(dlg->m_cfg_list, "views");
	assert(views);
	for ( i = 0; i < PLAYER_PMNG_COUNT; i ++ )
	{
		if (views[i].m_configure == (button_t *)wnd)
		{
			v = &views[i];
			break;
		}
	}
	assert(v);

	/* Get corresponding plugin */
	index = v->m_list->m_cursor;
	if (!v->m_list->m_list_size)
		return WND_MSG_RETCODE_OK;
	p = (plugin_t *)v->m_list->m_list[index].m_data;

	/* Call configure */
	plugin_configure(p, wnd_root);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_pmng_dialog_on_configure' function */

/* Handle 'clicked' message for plugins manager start/stop general 
 * plugin button */
wnd_msg_retcode_t player_pmng_dialog_on_start_stop_general( wnd_t *wnd )
{
	player_pmng_view_t *v = NULL;
	player_pmng_view_t *views; 
	wnd_t *dlg;
	plugin_t *p;
	int index;

	/* Determine our view */
	dlg = DLGITEM_OBJ(wnd)->m_dialog;
	views = (player_pmng_view_t *)cfg_get_var_ptr(dlg->m_cfg_list, "views");
	assert(views);
	v = &views[PLAYER_PMNG_GENERAL];
	assert(v);

	/* Get corresponding plugin */
	index = v->m_list->m_cursor;
	if (!v->m_list->m_list_size)
		return WND_MSG_RETCODE_OK;
	p = (plugin_t *)v->m_list->m_list[index].m_data;

	/* Change state */
	if (!genp_is_started(GENERAL_PLUGIN(p)))
		genp_start(GENERAL_PLUGIN(p));
	else
		genp_end(GENERAL_PLUGIN(p));
	player_pmng_dialog_sync(DIALOG_OBJ(dlg));
	return WND_MSG_RETCODE_OK;
} /* End of 'player_pmng_dialog_on_start_stop_general' function */

/* Handle 'clicked' message for plugins manager reload button */
wnd_msg_retcode_t player_pmng_dialog_on_reload( wnd_t *wnd )
{
	pmng_load_plugins(player_pmng);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_pmng_dialog_on_reload' function */

/* Destructor for plugins manager */
void player_pmng_dialog_destructor( wnd_t *wnd )
{
	player_pmng_view_t *views = 
		(player_pmng_view_t *)cfg_get_var_ptr(wnd->m_cfg_list, "views");
	assert(views);
	free(views);
} /* End of 'player_pmng_dialog_destructor' function */

/*****
 *
 * Variables change handlers
 *
 *****/

/* Handle 'title_format' variable setting */
static bool_t player_handle_var_title_format( cfg_node_t *var, char *value, 
		void *data )
{
	int i;

	if (player_plist == NULL)
		return TRUE;

	for ( i = 0; i < player_plist->m_len; i ++ )
		song_update_title(player_plist->m_list[i]);
	wnd_invalidate(wnd_root);
	return TRUE;
} /* End of 'player_handle_var_title_format' function */

/* Handle 'color-scheme' variable setting */
static bool_t player_handle_color_scheme( cfg_node_t *node, char *value, 
		void *data )
{
	char fname[MAX_FILE_NAME];
	
	/* Read configuration from respective file */
	snprintf(fname, sizeof(fname), "%s/.mpfc/colors/%s", 
			getenv("HOME"), cfg_get_var(cfg_list, node->m_name));
	cfg_rcfile_read(cfg_list, fname);
	wnd_invalidate(wnd_root);
	return TRUE;
} /* End of 'player_handle_color_scheme' function */

/* Handle 'kbind-scheme' variable setting */
static bool_t player_handle_kbind_scheme( cfg_node_t *node, char *value, 
		void *data )
{
	char fname[MAX_FILE_NAME];
	
	/* Read configuration from respective file */
	snprintf(fname, sizeof(fname), "%s/.mpfc/kbinds/%s", 
			getenv("HOME"), cfg_get_var(cfg_list, node->m_name));
	cfg_rcfile_read(cfg_list, fname);
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
			wnd_basic_class_init(global), player_get_msg_info, 
			player_free_handlers, player_class_set_default_styles);
	return klass;
} /* End of 'player_wnd_class_init' function */

/* Get message information */
wnd_msg_handler_t **player_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "command"))
	{
		if (callback != NULL)
			(*callback) = player_callback_command;
		return &PLAYER_WND(wnd)->m_on_command;
	}
	return NULL;
} /* End of 'player_get_msg_info' function */

/* Free message handlers */
void player_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(PLAYER_WND(wnd)->m_on_command);
} /* End of 'player_free_handlers' function */

/* Set player class default styles */
void player_class_set_default_styles( cfg_node_t *list )
{
	char letter;
	int dig;

	/* Initialize styles */
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

	/* Initialize kbinds */
	cfg_set_var(list, "kbind.queue", "'");
	cfg_set_var(list, "kbind.quit", "q;Q");
	cfg_set_var(list, "kbind.move_down", "j;<Ctrl-n>;<Down>");
	cfg_set_var(list, "kbind.move_up", "k;<Ctrl-p>;<Up>");
	cfg_set_var(list, "kbind.screen_down", "d;<Ctrl-v>;<PageDown>");
	cfg_set_var(list, "kbind.screen_up", "u;<Alt-v>;<PageUp>");
	cfg_set_var(list, "kbind.move_to_begin", "<Home>;gg;<Ctrl-a>");
	cfg_set_var(list, "kbind.move_to_end", "<End>;<Ctrl-e>");
	cfg_set_var(list, "kbind.move", "G");
	cfg_set_var(list, "kbind.start_play", "<Return>");
	cfg_set_var(list, "kbind.play", "x");
	cfg_set_var(list, "kbind.pause", "c");
	cfg_set_var(list, "kbind.stop", "v");
	cfg_set_var(list, "kbind.next", "b");
	cfg_set_var(list, "kbind.prev", "z");
	cfg_set_var(list, "kbind.time_fw", "\\>;lt");
	cfg_set_var(list, "kbind.time_bw", "\\<;ht");
	cfg_set_var(list, "kbind.time_long_fw", "<Right>;lT");
	cfg_set_var(list, "kbind.time_long_bw", "<Left>;hT");
	cfg_set_var(list, "kbind.time_move", "gt");
	cfg_set_var(list, "kbind.vol_fw", "+;lv");
	cfg_set_var(list, "kbind.vol_bw", "-;hv");
	cfg_set_var(list, "kbind.vol_def", "=");
	cfg_set_var(list, "kbind.info", "i");
	cfg_set_var(list, "kbind.add", "a");
	cfg_set_var(list, "kbind.rem", "r");
	cfg_set_var(list, "kbind.save", "s");
	cfg_set_var(list, "kbind.sort", "S");
	cfg_set_var(list, "kbind.visual", "V");
	cfg_set_var(list, "kbind.centrize", "C");
	cfg_set_var(list, "kbind.search", "/");
	cfg_set_var(list, "kbind.advanced_search", "\\\\");
	cfg_set_var(list, "kbind.next_match", "n");
	cfg_set_var(list, "kbind.prev_match", "N");
	cfg_set_var(list, "kbind.help", "?");
	cfg_set_var(list, "kbind.shuffle", "R");
	cfg_set_var(list, "kbind.var_manager", "o");
	cfg_set_var(list, "kbind.audio_setup", "A");
	cfg_set_var(list, "kbind.plist_down", "J");
	cfg_set_var(list, "kbind.plist_up", "K");
	cfg_set_var(list, "kbind.plist_move", "M");
	cfg_set_var(list, "kbind.undo", "U");
	cfg_set_var(list, "kbind.redo", "D");
	cfg_set_var(list, "kbind.reload_info", "I");
	cfg_set_var(list, "kbind.set_play_bounds", "ps");
	cfg_set_var(list, "kbind.clear_play_bounds", "pc");
	cfg_set_var(list, "kbind.play_bounds", "p<Return>");
	cfg_set_var(list, "kbind.exec", "!");
	cfg_set_var(list, "kbind.goback", "``");
	cfg_set_var(list, "kbind.time_back", "<Backspace>");
	cfg_set_var(list, "kbind.plugins_manager", "P");
	cfg_set_var(list, "kbind.file_browser", "B");
	//cfg_set_var(list, "kbind.test", "T");
	cfg_set_var(list, "kbind.log", "O");
	for ( letter = 'a'; letter <= 'z'; letter ++ )
	{
		char mark[12];
		char go[10];
		char mark_val[3] = {'m', 0, 0};
		char go_val[3] = {'`', 0, 0};

		sprintf(mark, "kbind.mark%c", letter);
		sprintf(go, "kbind.go%c", letter);
		mark_val[1] = letter;
		go_val[1] = letter;
		cfg_set_var(list, mark, mark_val);
		cfg_set_var(list, go, go_val);
	}
	for ( dig = 0; dig <= 9; dig ++ )
	{
		char d[12];
		char val[2] = {0, 0};
		sprintf(d, "kbind.dig_%c", dig + '0');
		val[0] = dig + '0';
		cfg_set_var(list, d, val);
	}
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
		int was_pos = player_plist->m_sel_end;
		plist_move(player_plist, player_last_pos, FALSE);
		player_last_pos = was_pos;
	}
} /* End of 'player_go_back' function */

/* Return to the last time */
void player_time_back( void )
{
	if (player_last_song == player_plist->m_cur_song)
		player_seek(player_last_song_time, FALSE);
	else
		player_play(player_last_song, player_last_song_time);
	pmng_hook(player_pmng, "player-status");
} /* End of 'player_time_back' function */

/* Save current song and time */
void player_save_time( void )
{
	player_last_song = player_plist->m_cur_song;
	player_last_song_time = player_context->m_cur_time;
} /* End of 'player_save_time' function */

/* High-level start play */
void player_start_play( int song, song_time_t start_time )
{
	player_context->m_status = PLAYER_STATUS_PLAYING;
	player_play(song, start_time);
	pmng_hook(player_pmng, "player-status");
} /* End of 'player_start_play' function */

/* High-level pause/resume */
void player_pause_resume( void )
{
	if (player_context->m_status == PLAYER_STATUS_PLAYING)
	{
		player_context->m_status = PLAYER_STATUS_PAUSED;
	}
	else if (player_context->m_status == PLAYER_STATUS_PAUSED)
	{
		player_context->m_status = PLAYER_STATUS_PLAYING;
	}

	pmng_hook(player_pmng, "player-status");
} /* End of 'player_pause_resume' function */

/* High-level stop play */
void player_stop( void )
{
	int was_song = player_plist->m_cur_song;

	player_context->m_status = PLAYER_STATUS_STOPPED;
	player_end_play(FALSE);
	player_plist->m_cur_song = was_song;
	pmng_hook(player_pmng, "player-status");
} /* End of 'player_stop' function */

/* Queue the selected song */
void player_queue_song( void )
{
	if(num_queued_songs < PLAYER_MAX_ENQUEUED)
		queued_songs[num_queued_songs++] = player_plist->m_sel_end;
} /* End of 'player_queue_song' function */

/* End of 'player.c' file */

