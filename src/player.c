/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.c
 * PURPOSE     : SG MPFC. Main player functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.08.2004
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
#include <stdio.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "browser.h"
#include "cfg.h"
#include "colors.h"
#include "eqwnd.h"
#include "error.h"
#include "file.h"
#include "help_screen.h"
#include "history.h"
#include "iwt.h"
#include "key_bind.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "sat.h"
#include "undo.h"
#include "util.h"
#include "wnd.h"

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

/* Objects to load */
char **player_objects = NULL;
int player_num_obj = 0;

/* Edit boxes history lists */
hist_list_t *player_hist_lists[PLAYER_NUM_HIST_LISTS];

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

/* Information about currently opened info editor dialog */
song_t *player_info_song = NULL;
bool_t player_info_local = TRUE;
int player_info_start = -1, player_info_end = -1;

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

/* Initialize player */
bool_t player_init( int argc, char *argv[] )
{
	int i, l, r;
	plist_set_t *set;

	/* Set signal handlers */
/*	signal(SIGINT, player_handle_signal);
	signal(SIGTERM, player_handle_signal);*/
	
	/* Initialize configuration */
	snprintf(player_cfg_file, sizeof(player_cfg_file), 
			"%s/.mpfcrc", getenv("HOME"));
	if (!player_init_cfg())
		return FALSE;

	/* Parse command line */
	if (!player_parse_cmd_line(argc, argv))
		return FALSE;

	/* Initialize window system */
	wnd_root = wnd_init(cfg_list);
	if (wnd_root == NULL)
		return FALSE;

	/* Create window for play list */
	player_wnd = wnd_new("Play list", wnd_root, 0, 0, 0, 0, 
			WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED);
	wnd_msg_add_handler(&player_wnd->m_on_display, player_on_display);
	wnd_msg_add_handler(&player_wnd->m_on_keydown, player_on_keydown);
	wnd_msg_add_handler(&player_wnd->m_on_close, player_on_close);
	wnd_msg_add_handler(&player_wnd->m_on_mouse_ldown, player_on_mouse_ldown);
	wnd_msg_add_handler(&player_wnd->m_on_mouse_mdown, player_on_mouse_mdown);
	wnd_msg_add_handler(&player_wnd->m_on_mouse_ldouble, 
			player_on_mouse_ldouble);
	player_wnd->m_cursor_hidden = TRUE;

	/* Initialize key bindings */
	kbind_init();

	/* Initialize colors */
	col_init();

	/* Initialize file browser directory */
	getcwd(player_fb_dir, sizeof(player_fb_dir));
	strcat(player_fb_dir, "/");
	
	/* Initialize plugin manager */
	player_pmng = pmng_init(cfg_list, player_print_msg);

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
		return FALSE;
	}
	for ( i = 0; i < player_num_obj; i ++ )
	{
		plist_add_obj(player_plist, player_objects[i], NULL, -1);
//		free(player_objects[i]);
	}
	free(player_objects);

	/* Make a set of files to add */
	set = plist_set_new();
	for ( i = 0; i < player_num_files; i ++ )
		plist_set_add(set, player_files[i]);
	plist_add_set(player_plist, set);
	plist_set_free(set);

	/* Load saved play list if files list is empty */
	if (!player_num_files && !player_num_obj)
		plist_add(player_plist, "~/mpfc.m3u");

	/* Initialize history lists */
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
		player_hist_lists[i] = hist_list_new();

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
	if (cfg_get_var_int(cfg_list, "play-from-stop") && 
			!player_num_files && !player_num_obj)
	{
		player_status = cfg_get_var_int(cfg_list, "player-status");
		player_start = cfg_get_var_int(cfg_list, "player-start") - 1;
		player_end = cfg_get_var_int(cfg_list, "player-end") - 1;
		if (player_status != PLAYER_STATUS_STOPPED)
			player_play(cfg_get_var_int(cfg_list, "cur-song"),
					cfg_get_var_int(cfg_list, "cur-time"));
	}

	/* Exit */
	return TRUE;
} /* End of 'player_init' function */

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
		hist_list_free(player_hist_lists[i]);
		player_hist_lists[i] = NULL;
	}

	/* Free memory allocated for strings */
	if (player_msg != NULL)
	{
		free(player_msg);
		player_msg = NULL;
	}
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

	/* Uninitialize configuration manager */
	cfg_free_node(cfg_list);
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

		/* Load object */
		if (!strcmp(name, "obj"))
		{
			if (i < argc - 1 && player_num_obj < 10)
			{
				player_objects = (char **)realloc(player_objects,
						sizeof(char *) * (player_num_obj + 1));
				player_objects[player_num_obj ++] = argv[++i];
			}
			free(name);
			continue;
		}

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

/* Handle key function */
wnd_msg_retcode_t player_on_keydown( wnd_t *wnd, wnd_key_t *keycode )
{
	if (!keycode->m_alt)
		kbind_key2buf(keycode->m_key);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_handle_key' function */

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
		/* Print about */
		wnd_move(wnd, 0, 0, 0);
		col_set_color(wnd, COL_EL_ABOUT);
		wnd_printf(wnd, 0, 0, _("SG Software Media Player For Console"));
		col_set_color(wnd, COL_EL_DEFAULT);
		
		/* Print shuffle mode */
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - 13, 0);
			col_set_color(wnd, COL_EL_PLAY_MODES);
			wnd_printf(wnd, 0, 0, "Shuffle");
			col_set_color(wnd, COL_EL_DEFAULT);
		}
		
		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - 5, 0);
			col_set_color(wnd, COL_EL_PLAY_MODES);
			wnd_printf(wnd, 0, 0, "Loop");
			col_set_color(wnd, COL_EL_DEFAULT);
		}

		/* Print version */
		wnd_move(wnd, 0, 0, 1);
		col_set_color(wnd, COL_EL_ABOUT);
		wnd_printf(wnd, 0, 0, _("version %s"), VERSION);
		col_set_color(wnd, COL_EL_DEFAULT);
	}
	else
	{
		int t;
		bool_t show_rem;
		
		/* Print current song title */
		s = player_plist->m_list[player_plist->m_cur_song];
		wnd_move(wnd, 0, 0, 0);
		col_set_color(wnd, COL_EL_CUR_TITLE);
		wnd_printf(wnd, WND_PRINT_ELLIPSES, WND_WIDTH(wnd) - 1, "%s", 
				STR_TO_CPTR(s->m_title));
		col_set_color(wnd, COL_EL_DEFAULT);

		/* Print shuffle mode */
		if (cfg_get_var_int(cfg_list, "shuffle-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - 13, 0);
			col_set_color(wnd, COL_EL_PLAY_MODES);
			wnd_printf(wnd, 0, 0, "Shuffle");
			col_set_color(wnd, COL_EL_DEFAULT);
		}

		/* Print loop mode */
		if (cfg_get_var_int(cfg_list, "loop-play"))
		{
			wnd_move(wnd, 0, WND_WIDTH(wnd) - 5, 0);
			col_set_color(wnd, COL_EL_PLAY_MODES);
			wnd_printf(wnd, 0, 0, "Loop");
			col_set_color(wnd, COL_EL_DEFAULT);
		}

		/* Print current time */
		t = (show_rem = cfg_get_var_int(cfg_list, "show-time-remaining")) ? 
			s->m_len - player_cur_time : player_cur_time;
		wnd_move(wnd, 0, 0, 1);
		col_set_color(wnd, COL_EL_CUR_TIME);
		wnd_printf(wnd, 0, 0, "%s%i:%02i/%i:%02i\n", 
				show_rem ? "-" : "", t / 60, t % 60,
				s->m_len / 60, s->m_len % 60);
		col_set_color(wnd, COL_EL_DEFAULT);
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
	col_set_color(wnd, COL_EL_AUDIO_PARAMS);
	wnd_printf(wnd, 0, 0, "%s", aparams);
	col_set_color(wnd, COL_EL_DEFAULT);

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
		col_set_color(wnd, COL_EL_STATUS);
		wnd_printf(wnd, 0, 0, "%s", player_msg);
		col_set_color(wnd, COL_EL_DEFAULT);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'player_display' function */

#if 0
/* Key handler function for command repeat value edit box */
void player_repval_handle_key( wnd_t *wnd, dword data )
{
	editbox_t *box = (editbox_t *)wnd;
	int key = (int)data;
	
	/* Remember last pressed key */
	box->m_last_key = key;

	/* Add character to text if it is a digit */
	if (key >= '0' && key <= '9')
		ebox_add(box, key);
	/* Move cursor */
	else if (key == KEY_RIGHT)
		ebox_move(box, TRUE, 1);
	else if (key == KEY_LEFT)
		ebox_move(box, TRUE, -1);
	else if (key == KEY_HOME)
		ebox_move(box, FALSE, 0);
	else if (key == KEY_END)
		ebox_move(box, FALSE, EBOX_LEN(box));
	/* Delete character */
	else if (key == KEY_BACKSPACE)
	{
		if (box->m_cursor)
			ebox_del(box, box->m_cursor - 1);
		else
			wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
	}
	else if (key == KEY_DC)
		ebox_del(box, box->m_cursor);
	/* Exit */
	else 
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
} /* End of 'player_repval_handle_key' function */
#endif

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

	inp_seek(song_get_inp(s), new_time);
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

		/* Skip to next iteration if there is nothing to play */
		if (player_plist->m_cur_song < 0 || 
				player_status == PLAYER_STATUS_STOPPED)
		{
			util_delay(0, 100000L);
			continue;
		}

		/* Play track */
		s = player_plist->m_list[player_plist->m_cur_song];
		//player_status = PLAYER_STATUS_PLAYING;
		player_end_track = FALSE;
	
		/* Start playing */
		inp = song_get_inp(s);
		if (!inp_start(inp, s->m_file_name))
		{
			player_next_track();
			error_set_code(ERROR_UNKNOWN_FILE_TYPE);
			player_print_msg("%s", error_text);
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
			player_print_msg(_("Unable to initialize output plugin"));
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

		/* Sleep a little */
		tv.tv_sec = 0;
		tv.tv_nsec = 100000L;
		nanosleep(&tv, NULL);
	}

	player_cur_time = 0;
	player_end_timer = FALSE;
	player_timer_tid = 0;
	return NULL;
} /* End of 'player_timer_func' function */

#if 0
/* Process add file dialog */
void player_add_dialog( void )
{
	file_input_box_t *fin;

	/* Create edit box for path input */
	fin = fin_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
			WND_WIDTH(wnd_root), _("Enter file (or directory) name: "));
	if (fin != NULL)
	{
		((editbox_t *)fin)->m_hist_list = 
			player_hist_lists[PLAYER_HIST_LIST_ADD];

		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
			plist_add(player_plist, EBOX_TEXT(fin));

		/* Destroy edit box */
		wnd_destroy(fin);
	}
} /* End of 'player_add_dialog' function */

/* Process save play list dialog */
void player_save_dialog( void )
{
	file_input_box_t *fin;

	/* Create edit box for path input */
	fin = fin_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root), 
			_("Enter file name: "));
	if (fin != NULL)
	{
		((editbox_t *)fin)->m_hist_list = 
			player_hist_lists[PLAYER_HIST_LIST_SAVE];

		/* Run message loop */
		wnd_run(fin);

		/* Save list if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
		{
			bool_t res = plist_save(player_plist, EBOX_TEXT(fin));
			player_print_msg((res) ? _("Play list saved") : 
					_("Unable to save play list"));
		}

		/* Destroy edit box */
		wnd_destroy(fin);
	}
} /* End of 'player_save_dialog' function */

/* Process remove song(s) dialog */
void player_rem_dialog( void )
{
	int was = player_plist->m_len;
	
	plist_rem(player_plist);
	player_print_msg(_("Removed %i songs"), was - player_plist->m_len);
} /* End of 'player_rem_dialog' function */

/* Process sort play list dialog */
void player_sort_dialog( void )
{
	choice_ctrl_t *ch;
	char choice;
	int t;
	bool_t g;

	/* Get sort globalness parameter */
	ch = choice_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
			1, _("Sort globally? (Yes/No)"), "yn");
	if (ch == NULL)
		return;
	wnd_run(ch);
	choice = ch->m_choice;
	wnd_destroy(ch);
	if (!CHOICE_VALID(choice))
		return;
	g = (choice == 'y');
	
	/* Get sort criteria */
	ch = choice_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
		1, _("Sort by: (T)itle, (F)ile name, (P)ath and file name, "
			"T(r)ack"), "tfpr");
	if (ch == NULL)
		return;
	wnd_run(ch);
	choice = ch->m_choice;
	wnd_destroy(ch);
	if (!CHOICE_VALID(choice))
		return;

	/* Sort */
	switch (choice)
	{
	case 't':
		t = PLIST_SORT_BY_TITLE;
		break;
	case 'f':
		t = PLIST_SORT_BY_NAME;
		break;
	case 'p':
		t = PLIST_SORT_BY_PATH;
		break;
	case 'r':
		t = PLIST_SORT_BY_TRACK;
		break;
	}
	plist_sort(player_plist, g, t);
} /* End of 'player_sort_dialog' function */

/* Process search play list dialog */
void player_search_dialog( int criteria )
{
	editbox_t *ebox;

	/* Display edit box for entering search text */
	ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
			1, 256, _("Enter search string: "), "");
	ebox->m_hist_list = 
		player_hist_lists[PLAYER_HIST_LIST_SEARCH];
	if (ebox != NULL)
	{
		wnd_run(ebox);

		/* Save search parameters and search */
		if (ebox->m_last_key == '\n')
		{
			player_set_search_string(EBOX_TEXT(ebox));
			player_search_criteria = criteria;
			if (!plist_search(player_plist, player_search_string, 1, 
						criteria))
				player_print_msg(_("String not found"));
			else
				player_print_msg(_("String found"));
		}
		wnd_destroy(ebox);
	}
} /* End of 'player_search_dialog' function */

/* Process song info dialog */
void player_info_dialog( void )
{
	dlgbox_t *dlg;
	song_t *s;
	editbox_t *name, *album, *artist, *year, *comments, *track;
	combobox_t *genre;
	genre_list_t *glist;
	int i, j, y = 1, num_specs = 0, max_len;
	int start, end;
	bool_t name_common = TRUE, album_common = TRUE, artist_common = TRUE, 
			year_common = TRUE,	comments_common = TRUE, track_common = TRUE, 
			genre_common = TRUE;
	bool_t local = TRUE;
	label_t *label;
	int sel_start = player_plist->m_sel_start, 
		sel_end = player_plist->m_sel_end;
	PLIST_GET_SEL(player_plist, start, end);

	/* Get song object */
	if (sel_end < 0 || !player_plist->m_len)
		return;
	else
		s = player_plist->m_list[sel_end];

	/* Ask whether to edit info locally in case of multi selection */
	if (sel_end != sel_start)
	{
		choice_ctrl_t *ch;
		int choice;
		
		ch = choice_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
			1, _("Info edit locally? (Yes/No)"), "yn");
		if (ch == NULL)
			return;
		wnd_run(ch);
		choice = ch->m_choice;
		wnd_destroy(ch);
		if (!CHOICE_VALID(choice))
			return;
		local = (choice == 'y');
	}
	
	/* Update songs information */
	if (local)
	{
		song_update_info(s);
		if (s->m_info == NULL)
			return;
	}
	else
	{
		if (s->m_info == NULL)
			s = NULL;
		
		for ( i = start; i <= end; i ++ )
		{
			song_t *song = player_plist->m_list[i];
			
			song_update_info(song);
			if (song->m_info == NULL)
				continue;
			if (s == NULL)
				s = song;

			/* Check for differences */
			for ( j = start; j < i; j ++ )
			{
				song_info_t *i1 = song->m_info, 
							*i2 = player_plist->m_list[j]->m_info;

				if (player_plist->m_list[j]->m_info == NULL)
					continue;

				if (name_common && strcmp(i1->m_name, i2->m_name))
					name_common = FALSE;
				if (artist_common && strcmp(i1->m_artist, i2->m_artist))
					artist_common = FALSE;
				if (album_common && strcmp(i1->m_album, i2->m_album))
					album_common = FALSE;
				if (year_common && strcmp(i1->m_year, i2->m_year))
					year_common = FALSE;
				if (genre_common && strcmp(i1->m_genre, i2->m_genre))
					genre_common = FALSE;
				if (track_common && strcmp(i1->m_track, i2->m_track))
					track_common = FALSE;
				if (comments_common && strcmp(i1->m_comments, i2->m_comments))
					comments_common = FALSE;
			}
		}
	}

	/* If there are no songs to edit info - exit */
	if (s == NULL)
		return;

	/* Create info dialog */
	dlg = dlg_new(wnd_root, 2, 2, WND_WIDTH(wnd_root) - 4, 21, 
			cfg_get_var_int(cfg_list, "info-editor-show-full-name") ? 
			s->m_file_name : util_short_name(s->m_file_name));
	wnd_register_handler(dlg, WND_MSG_NOTIFY, player_info_notify);
	if (!(s->m_info->m_flags & SI_ONLY_OWN))
	{
		name = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Song name: "), s->m_info->m_name);
		artist = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Artist name: "), s->m_info->m_artist);
		album = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Album name: "), s->m_info->m_album);
		year = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Year: "), s->m_info->m_year);
		track = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Track No: "), s->m_info->m_track);
		comments = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				-1, _("Comments: "), s->m_info->m_comments);
		genre = cbox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 25, 12,
				_("Genre: "));
		glist = s->m_info->m_glist;
		for ( i = 0; glist != NULL && i < glist->m_size; i ++ )
			cbox_list_add(genre, glist->m_list[i].m_name);
		cbox_set_text(genre, s->m_info->m_genre);

		/* Set IDs */
		WND_OBJ(name)->m_id = PLAYER_INFO_NAME;
		WND_OBJ(artist)->m_id = PLAYER_INFO_ARTIST;
		WND_OBJ(album)->m_id = PLAYER_INFO_ALBUM;
		WND_OBJ(year)->m_id = PLAYER_INFO_YEAR;
		WND_OBJ(track)->m_id = PLAYER_INFO_TRACK;
		WND_OBJ(comments)->m_id = PLAYER_INFO_COMMENTS;
		WND_OBJ(genre)->m_id = PLAYER_INFO_GENRE;

		/* Set commonness flags */
		name->m_grayed = !name_common;
		artist->m_grayed = !artist_common;
		album->m_grayed = !album_common;
		year->m_grayed = !year_common;
		track->m_grayed = !track_common;
		comments->m_grayed = !comments_common;
		genre->m_grayed = !genre_common;
	}

	/* Display own data */
	label = label_new(WND_OBJ(dlg), 2, y + 1, WND_WIDTH(dlg) - 6, 
			WND_HEIGHT(dlg) - y - 1, s->m_info->m_own_data);
	WND_OBJ(label)->m_id = PLAYER_INFO_LABEL;

	/* Get the maximal length of special function title */
	num_specs = inp_get_num_specs(s->m_inp);
	max_len = 0;
	for ( i = 0; i < num_specs; i ++ )
	{
		char *title = inp_get_spec_title(s->m_inp, i);
		if (title != NULL)
		{
			int len = strlen(title);
			if (len > max_len)
				max_len = len;
		}
	}

	/* Create buttons for special functions */
	for ( i = 0; i < num_specs; i ++ )
	{
		button_t *btn;

		btn = btn_new(WND_OBJ(dlg), WND_WIDTH(dlg) - max_len - 5, 
				y + 1 + i * 2, max_len + 2, inp_get_spec_title(s->m_inp, i));
		WND_OBJ(btn)->m_id = i;
	}

	/* Set global information about info editor (for notify function) */
	player_info_song = s;
	player_info_local = local;
	player_info_start = start;
	player_info_end = end;

	/* Display dialog */
	wnd_run(dlg);

	/* Unset information */
	player_info_song = NULL;
	player_info_local = TRUE;
	player_info_start = player_info_end = -1;

	/* Save */
	if (dlg->m_ok)
	{
		song_info_t *info;
		
		/* Remember information */
		for ( i = start; i <= end; i ++ )
		{
			if (!local)
			{
				s = player_plist->m_list[i];
				if (s->m_info == NULL)
					continue;
			}
			info = s->m_info;
			if (name->m_changed)
				si_set_name(info, EBOX_TEXT(name));
			if (artist->m_changed)
				si_set_artist(info, EBOX_TEXT(artist));
			if (album->m_changed)
				si_set_album(info, EBOX_TEXT(album));
			if (year->m_changed)
				si_set_year(info, EBOX_TEXT(year));
			if (comments->m_changed)
				si_set_comments(info, EBOX_TEXT(comments));
			if (track->m_changed)
				si_set_track(info, EBOX_TEXT(track));
			if (genre->m_changed)
				si_set_genre(info, CBOX_TEXT(genre));
		
			/* Save info */
			iwt_push(s);

			/* Update */
//			song_update_info(s);

			/* Break if local */
			if (local)
				break;
		}
	}
	
	wnd_destroy(dlg);
} /* End of 'player_info_dialog' function */
#endif

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

/* Display slider */
void player_display_slider( wnd_t *wnd, int x, int y, int width, 
		int pos, int range )
{
	int i, slider_pos;
	
	wnd_move(wnd, 0, x, y);
	slider_pos = (range) ? (pos * width / range) : 0;
	col_set_color(wnd, COL_EL_SLIDER);
	for ( i = 0; i <= width; i ++ )
	{
		if (i == slider_pos)
			wnd_printf(wnd, 0, 0, "O");
		else 
			wnd_printf(wnd, 0, 0, "=");
	}
	col_set_color(wnd, COL_EL_DEFAULT);
} /* End of 'player_display_slider' function */

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

#if 0
/* Launch variables manager */
void player_var_manager( void )
{
	dlgbox_t *dlg;
	listbox_t *var_lb;
	editbox_t *val_eb;
	button_t *btn;
	int i;

	/* Initialize dialog */
	dlg = dlg_new(wnd_root, 2, 2, WND_WIDTH(wnd_root) - 4, 20, 
			_("Variables manager"));
	wnd_register_handler(dlg, WND_MSG_NOTIFY, player_var_mngr_notify);
	var_lb = lbox_new(WND_OBJ(dlg), 2, 2, WND_WIDTH(dlg) - 4, 
			WND_HEIGHT(dlg) - 5, "");
	var_lb->m_minimalizing = FALSE;
	WND_OBJ(var_lb)->m_id = PLAYER_VAR_MNGR_VARS;
	val_eb = ebox_new(WND_OBJ(dlg), 2, WND_HEIGHT(dlg) - 3,
			WND_WIDTH(dlg) - 4, 1, -1, _("Value: "), 
			(cfg_list->m_num_vars) ? cfg_list->m_vars[0].m_val : "");
	WND_OBJ(val_eb)->m_id = PLAYER_VAR_MNGR_VAL;
	btn = btn_new(WND_OBJ(dlg), 2, WND_HEIGHT(dlg) - 2, -1, _("New variable"));
	WND_OBJ(btn)->m_id = PLAYER_VAR_MNGR_NEW;
	btn = btn_new(WND_OBJ(dlg), WND_X(btn) + WND_WIDTH(btn) + 1, 
			WND_HEIGHT(dlg) - 2, -1, _("Save"));
	WND_OBJ(btn)->m_id = PLAYER_VAR_MNGR_SAVE;
	btn = btn_new(WND_OBJ(dlg), WND_X(btn) + WND_WIDTH(btn) + 1,
		   	WND_HEIGHT(dlg) - 2, -1, _("Restore value"));
	WND_OBJ(btn)->m_id = PLAYER_VAR_MNGR_RESTORE;

	/* Fill variables list box */
	player_var_mngr_pos = -1;
	for ( i = 0; i < cfg_list->m_num_vars; i ++ )
	{
		if (!(cfg_get_var_flags(cfg_list, cfg_list->m_vars[i].m_name) & 
				CFG_RUNTIME))
			lbox_add(var_lb, cfg_list->m_vars[i].m_name);
	}
	lbox_move_cursor(var_lb, FALSE, 0, TRUE);

	/* Display dialog */
	wnd_run(dlg);

	/* Save variables */
	if (var_lb->m_cursor >= 0)
	{
		cfg_set_var(cfg_list, var_lb->m_list[var_lb->m_cursor].m_name, 
				EBOX_TEXT(val_eb));
	}

	/* Free memory */
	wnd_destroy(dlg);
} /* End of 'player_var_manager' function */

/* Launch add object dialog */
void player_add_obj_dialog( void )
{
	editbox_t *ebox;

	/* Create edit box for path input */
	ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
			WND_WIDTH(wnd_root), 1, MAX_FILE_NAME, _("Enter object name: "), "");
	if (ebox != NULL)
	{
		ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_ADD_OBJ];
		
		/* Run message loop */
		wnd_run(ebox);

		/* Add object if enter was pressed */
		if (ebox->m_last_key == '\n')
			plist_add_obj(player_plist, EBOX_TEXT(ebox), NULL, -1);

		/* Destroy edit box */
		wnd_destroy(ebox);
	}
} /* End of 'player_add_obj_dialog' function */
#endif

/* Handle action */
void player_handle_action( int action )
{
	char str[10];
//	editbox_t *ebox;
	bool_t dont_change_repval = FALSE;
	int was_pos;
	int was_song, was_time;

	/* Clear message string */
	player_print_msg("");

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
			inp_resume(player_inp);
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
		}
		else if (player_status == PLAYER_STATUS_PAUSED)
		{
			player_status = PLAYER_STATUS_PLAYING;
			inp_resume(player_inp);
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
//		player_add_dialog();
		break;

	/* Add an object */
	case KBIND_ADD_OBJ:
//		player_add_obj_dialog();
		break;

	/* Save play list */
	case KBIND_SAVE:
//		player_save_dialog();
		break;

	/* Remove song(s) */
	case KBIND_REM:
//		player_rem_dialog();
		break;

	/* Sort play list */
	case KBIND_SORT:
//		player_sort_dialog();
		break;

	/* Song info dialog */
	case KBIND_INFO:
//		player_info_dialog();
		break;

	/* Search */
	case KBIND_SEARCH:
//		player_search_dialog(PLIST_SEARCH_TITLE);
		break;

	/* Advanced search */
	case KBIND_ADVANCED_SEARCH:
//		player_advanced_search_dialog();
		break;

	/* Find next/previous search match */
	case KBIND_NEXT_MATCH:
	case KBIND_PREV_MATCH:
		if (!plist_search(player_plist, player_search_string, 
					(action == KBIND_NEXT_MATCH) ? 1 : -1, 
					player_search_criteria))
			player_print_msg(_("String not found"));
		else
			player_print_msg(_("String found"));
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
//		player_var_mini_mngr();
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
//		player_info_reload_dialog();
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
//		player_exec();
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

#if 0
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
		/* Create edit box */
		str[0] = (action - KBIND_DIG_0) + '0';
		str[1] = 0;
		ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
				WND_WIDTH(wnd_root), 1, 5, 
					_("Enter repeat value for the next command: "), str);
		if (ebox != NULL)
		{
			/* Run edit box message loop */
			wnd_register_handler(ebox, WND_MSG_KEYDOWN,
					player_repval_handle_key);
			wnd_run(ebox);

			/* Remember repeat value or handle last pressed command */
			if (ebox->m_last_key != 27)
			{
				player_repval = atoi(EBOX_TEXT(ebox));
				dont_change_repval = TRUE;
				player_handle_key(wnd_root, ebox->m_last_key);
			}
			
			/* Destroy edit box */
			wnd_destroy(ebox);
		}
		break;
#endif
	}

	/* Flush repeat value */
	if (!dont_change_repval)
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
} /* End of 'player_handle_action' function */

#if 0
/* Launch variables mini-manager */
void player_var_mini_mngr( void )
{
	editbox_t *ebox;
	char *name, *val;

	/* Create edit box for variable name input */
	ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
			WND_WIDTH(wnd_root), 1, -1, _("Enter variable name: "), "");
	if (ebox == NULL)
		return;
	ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_VAR_NAME];

	/* Run message loop */
	wnd_run(ebox);

	/* Check input */
	if (ebox->m_last_key != '\n' || !EBOX_LEN(ebox))
	{
		wnd_destroy(ebox);
		return;
	}
	name = strdup(EBOX_TEXT(ebox));
	wnd_destroy(ebox);

	/* Get value */
	ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
			WND_WIDTH(wnd_root), 1, -1, _("Enter variable value: "), "");
	if (ebox == NULL)
	{
		free(name);
		return;
	}
	ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_VAR_VAL];
	wnd_run(ebox);
	if (ebox->m_last_key != '\n')
	{
		free(name);
		wnd_destroy(ebox);
		return;
	}
	val = strdup(EBOX_LEN(ebox) ? EBOX_TEXT(ebox) : "1");
	wnd_destroy(ebox);

	/* Set variable value */
	cfg_set_var(cfg_list, name, val);
	free(name);
	free(val);
} /* End of 'player_var_mini_mngr' function */

/* Variables manager dialog notify handler */
void player_var_mngr_notify( wnd_t *wnd, dword data )
{
	dlgbox_t *dlg;
	short id, act;
	editbox_t *eb;
	listbox_t *lb;

	if (wnd == NULL)
		return;

	/* Get message data */
	dlg = (dlgbox_t *)wnd;
	id = WND_NOTIFY_ID(data);
	act = WND_NOTIFY_ACT(data);
	eb = (editbox_t *)dlg_get_item_by_id(dlg, PLAYER_VAR_MNGR_VAL);
	lb = (listbox_t *)dlg_get_item_by_id(dlg, PLAYER_VAR_MNGR_VARS);
	if (eb == NULL || lb == NULL)
		return;

	/* List box cursor movement */
	if (id == PLAYER_VAR_MNGR_VARS && act == LBOX_MOVE)
	{
		/* Save current edit box value to the respective variable */
		if (player_var_mngr_pos >= 0)
			cfg_set_var(cfg_list, 
					lb->m_list[player_var_mngr_pos].m_name, EBOX_TEXT(eb));
			
		/* Read new variable */
		player_var_mngr_pos = lb->m_cursor;
		ebox_set_text(eb, cfg_get_var(cfg_list, 
					lb->m_list[player_var_mngr_pos].m_name));
	}
	/* Save list */
	else if (id == PLAYER_VAR_MNGR_SAVE && act == BTN_CLICKED)
	{
		player_save_cfg_list(cfg_list, "~/.mpfcrc");
	}
	/* Restore value */
	else if (id == PLAYER_VAR_MNGR_RESTORE && act == BTN_CLICKED)
	{
		if (lb->m_cursor >= 0)
		{
			ebox_set_text(eb, cfg_get_var(cfg_list,
						lb->m_list[lb->m_cursor].m_name));
		}
	}
	/* New variable */
	else if (id == PLAYER_VAR_MNGR_NEW && act == BTN_CLICKED)
	{
		dlgbox_t *d;
		editbox_t *name, *val;

		d = dlg_new(WND_OBJ(dlg), 4, 4, 40, 5, _("New variable"));
		name = ebox_new(WND_OBJ(d), 2, 1, WND_WIDTH(d) - 5, 1, -1, 
				_("Name: "), "");
		val = ebox_new(WND_OBJ(d), 2, 2, WND_WIDTH(d) - 5, 1, -1, 
				_("Value: "), "");
		wnd_run(d);
		if (d->m_ok)
		{
			int was_len = cfg_list->m_num_vars;

			/* Set variable */
			cfg_set_var(cfg_list, EBOX_TEXT(name), EBOX_TEXT(val));

			/* Update main dialog items */
			if (was_len != cfg_list->m_num_vars)
			{
				lbox_add(lb, EBOX_TEXT(name));
			}
			else
			{
				if (lb->m_cursor >= 0 && 
						!strcmp(lb->m_list[lb->m_cursor].m_name, 
							EBOX_TEXT(name)))
					ebox_set_text(eb, EBOX_TEXT(val));
			}
		}
		wnd_destroy(d);
	}
} /* End of 'player_var_mngr_notify' function */
#endif

/* Save variables to main configuration file */
void player_save_cfg_vars( cfg_list_t *list, char *vars )
{
	char *name;
	cfg_node_t *tlist;
	int i, j;
	
	if (list == NULL)
		return;

	/* Initialize variables with initial values */
	tlist = cfg_new_list(NULL, "root", 0, 0);

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
	cfg_free_node(tlist);
} /* End of 'player_save_cfg_vars' function */

/* Save configuration list */
void player_save_cfg_list( cfg_list_t *list, char *fname )
{
#if 0
	FILE *fd;
	int i;

	if (list == NULL)
		return;

	/* Open file */
	fd = fopen(fname, "wt");
	if (fd == NULL)
		return;

	/* Write variables */
	for ( i = 0; i < list->m_num_vars; i ++ )
	{
	//	if (!(cfg_get_var_flags(list, list->m_vars[i].m_name) & CFG_RUNTIME))
			fprintf(fd, "%s=%s\n", list->m_vars[i].m_name, 
					list->m_vars[i].m_val);
	}

	/* Close file */
	fclose(fd);
#endif
} /* End of 'player_save_cfg_list' function */

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

#if 0
/* Execute a command */
void player_exec( void )
{
	editbox_t *ebox;

	/* Create edit box for command */
	ebox = ebox_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, 
			WND_WIDTH(wnd_root), 1, -1, _("Enter command: "), "");
	if (ebox != NULL)
	{
		ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_EXEC];
		
		/* Run message loop */
		wnd_run(ebox);

		/* Execute if enter was pressed */
		if (ebox->m_last_key == '\n')
		{
			wnd_close_curses();
			system(EBOX_TEXT(ebox));
			getchar();
			wnd_restore_curses();
		}

		/* Destroy edit box */
		wnd_destroy(ebox);
	}
} /* End of 'player_exec' function */
#endif

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

#if 0
/* Advanced search dialog */
void player_advanced_search_dialog( void )
{
	choice_ctrl_t *ch;
	char choice;
	int t;

	/* Get search criteria */
	ch = choice_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
		1, _("Search for: (T)itle, (N)ame, (A)rtist, A(l)bum, (Y)ear, "
			"(G)enre"), 
		"tnalyg");
	if (ch == NULL)
		return;
	wnd_run(ch);
	choice = ch->m_choice;
	wnd_destroy(ch);
	if (!CHOICE_VALID(choice))
		return;

	/* Search */
	switch (choice)
	{
	case 't':
		t = PLIST_SEARCH_TITLE;
		break;
	case 'n':
		t = PLIST_SEARCH_NAME;
		break;
	case 'a':
		t = PLIST_SEARCH_ARTIST;
		break;
	case 'l':
		t = PLIST_SEARCH_ALBUM;
		break;
	case 'y':
		t = PLIST_SEARCH_YEAR;
		break;
	case 'g':
		t = PLIST_SEARCH_GENRE;
		break;
	}
	player_search_dialog(t);
} /* End of 'player_advanced_search_dialog' function */
#endif

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

/* Return to the last time */
void player_time_back( void )
{
	player_play(player_last_song, player_last_song_time);
} /* End of 'player_time_back' function */

/* Message printer */
void player_print_msg( char *fmt, ... )
{
	int len = 256, n;
	va_list ap;

	if (player_msg != NULL)
		free(player_msg);
	player_msg = (char *)malloc(len);
	for ( ;; )
	{
		va_start(ap, fmt);
		n = vsnprintf(player_msg, len, fmt, ap);
		va_end(ap);
		if (n > -1 && n < len)
			break;
		else if (n > -1)
			len = n + 1;
		else
			len *= 2;
		player_msg = (char *)realloc(player_msg, len);
	}
	wnd_invalidate(player_wnd);
} /* End of 'player_print_msg' function */

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

#if 0
/* Info reload dialog */
void player_info_reload_dialog( void )
{
	choice_ctrl_t *ch;
	char choice;
	int t;
	bool_t g;

	/* Get reload globalness parameter */
	ch = choice_new(wnd_root, 0, WND_HEIGHT(wnd_root) - 1, WND_WIDTH(wnd_root),
			1, _("Reload info in the whole list? (Yes/No)"), "yn");
	if (ch == NULL)
		return;
	wnd_run(ch);
	choice = ch->m_choice;
	wnd_destroy(ch);
	if (!CHOICE_VALID(choice))
		return;
	g = (choice == 'y');
	
	/* Reload */
	plist_reload_info(player_plist, g);
} /* End of 'player_info_reload_dialog' function */

/* Notify function for info editor */
void player_info_notify( wnd_t *wnd, dword data )
{
	int i;
	int id = WND_NOTIFY_ID(data);

	/* Save info if need */
	if (inp_get_spec_flags(player_info_song->m_inp, id) &
			INP_SPEC_SAVE_INFO)
		player_save_info_dlg(wnd);

	/* Call special functions */
	if (player_info_local)
	{
		inp_spec_func(player_info_song->m_inp, id, 
				player_info_song->m_file_name);
		song_update_info(player_info_song);
	}
	else
	{
		in_plugin_t *inp = player_info_song->m_inp;
		
		for ( i = player_info_start; i <= player_info_end; i ++ )
		{
			if (player_plist->m_list[i]->m_info != NULL && 
					inp == player_plist->m_list[i]->m_inp)
			{
				inp_spec_func(inp, id, player_plist->m_list[i]->m_file_name);
				song_update_info(player_plist->m_list[i]);
			}
		}
	}

	/* Update dialog */
	player_update_info_dlg(wnd);
} /* End of 'player_info_notify' function */

/* Update currently opened info editor dialog */
void player_update_info_dlg( wnd_t *wnd )
{
	song_info_t *info;
	combobox_t *genre;
	
	if (wnd == NULL || player_info_song == NULL || 
			(info = player_info_song->m_info) == NULL)
		return;

	/* Update */
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_NAME), 
			info->m_name);
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_ARTIST), 
			info->m_artist);
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_ALBUM), 
			info->m_album);
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_YEAR), 
			info->m_year);
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_TRACK), 
			info->m_track);
	ebox_set_text((editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_COMMENTS), 
			info->m_comments);
	cbox_set_text((combobox_t *)wnd_find_child_by_id(wnd, 
				PLAYER_INFO_GENRE), info->m_genre);
	label_set_text((label_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_LABEL),
			info->m_own_data);
} /* End of 'player_update_info_dlg' function */

/* Save currently opened info editor dialog */
void player_save_info_dlg( wnd_t *wnd )
{
	song_t *s = player_info_song;
	editbox_t *name, *artist, *album, *track, *year, *comments;
	combobox_t *genre;
	label_t *own_data;
	int i;

	if (player_info_song == NULL || player_info_start < 0 || 
			player_info_end < 0)
		return;

	/* Get dialog items */
	name = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_NAME);
	artist = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_ARTIST);
	album = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_ALBUM);
	year = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_YEAR);
	track = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_TRACK);
	comments = (editbox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_COMMENTS);
	genre = (combobox_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_GENRE);
	own_data = (label_t *)wnd_find_child_by_id(wnd, PLAYER_INFO_LABEL);
	if (name == NULL || artist == NULL || album == NULL || year == NULL ||
			track == NULL || comments == NULL || own_data == NULL)
		return;

	/* Save info */
	for ( i = player_info_start; i <= player_info_end; i ++ )
	{
		song_info_t *info;
		
		if (!player_info_local)
		{
			s = player_plist->m_list[i];
			if (s->m_info == NULL)
				continue;
		}
		info = s->m_info;
		if (name->m_changed)
			si_set_name(info, EBOX_TEXT(name));
		if (artist->m_changed)
			si_set_artist(info, EBOX_TEXT(artist));
		if (album->m_changed)
			si_set_album(info, EBOX_TEXT(album));
		if (year->m_changed)
			si_set_year(info, EBOX_TEXT(year));
		if (comments->m_changed)
			si_set_comments(info, EBOX_TEXT(comments));
		if (track->m_changed)
			si_set_track(info, EBOX_TEXT(track));
		if (genre->m_changed)
			si_set_genre(info, CBOX_TEXT(genre));
	
		/* Save info */
		iwt_push(s);

		/* Update */
//		song_update_info(s);

		/* Break if local */
		if (player_info_local)
			break;
	}
} /* End of 'player_save_info_dlg' function */
#endif

/* Set a new search string */
void player_set_search_string( char *str )
{
	if (player_search_string != NULL)
		free(player_search_string);
	player_search_string = strdup(str);
} /* End of 'player_set_search_string' function */

/* Signal handler */
void player_handle_signal( int signum )
{
	if (signum == SIGINT || signum == SIGTERM)
		player_deinit();
} /* End of 'player_handle_signal' function */

/* Initialize configuration */
bool_t player_init_cfg( void )
{
	/* Initialize root configuration list and variables handlers */
	cfg_list = cfg_new_list(NULL, "", CFG_NODE_BIG_LIST, 0);
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

/* Play list window closing handler */
wnd_msg_retcode_t player_on_close( wnd_t *wnd )
{
	assert(wnd);
	wnd_close(wnd_root);
	return WND_MSG_RETCODE_OK;
} /* End of 'player_on_close' function */

/* End of 'player.c' file */

