/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.c
 * PURPOSE     : SG MPFC. Main player functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.09.2003
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
#include <unistd.h>
#include "types.h"
#include "button.h"
#include "cfg.h"
#include "choice_ctrl.h"
#include "colors.h"
#include "combobox.h"
#include "dlgbox.h"
#include "editbox.h"
#include "eqwnd.h"
#include "error.h"
#include "file_input.h"
#include "help_screen.h"
#include "history.h"
#include "key_bind.h"
#include "label.h"
#include "listbox.h"
#include "menu.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "sat.h"
#include "undo.h"
#include "util.h"

/* Files for player to play */
int player_num_files = 0;
char **player_files = NULL;

/* Play list */
plist_t *player_plist = NULL;

/* Command repeat value */
int player_repval = 0;

/* Search string and criteria */
char player_search_string[256] = "";
int player_search_criteria = PLIST_SEARCH_TITLE;

/* Message text */
char player_msg[80] = "";

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
char player_objects[10][256];
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

/* Initialize player */
bool_t player_init( int argc, char *argv[] )
{
	int i, l, r;

	/* Set signal handlers */
	/*signal(SIGINT, player_handle_signal);
	signal(SIGSTOP, player_handle_signal);
	signal(SIGTSTP, player_handle_signal);
	signal(SIGCONT, player_handle_signal);*/
	
	/* Initialize configuration */
	cfg_init();

	/* Parse command line */
	if (!player_parse_cmd_line(argc, argv))
		return FALSE;

	/* Create screen window */
	if (wnd_new_root() == NULL)
	{
		return FALSE;
	}
	wnd_register_handler(wnd_root, WND_MSG_DISPLAY, 
			player_display);
	wnd_register_handler(wnd_root, WND_MSG_KEYDOWN, 
			player_handle_key);
	wnd_register_handler(wnd_root, WND_MSG_MOUSE_LEFT_CLICK,
			player_handle_mouse_click);

	/* Initialize key bindings */
	kbind_init();

	/* Initialize colors */
	col_init();
	
	/* Initialize plugin manager */
	pmng_init();

	/* Initialize song adder thread */
	sat_init();

	/* Initialize undo list */
	player_ul = undo_new();

	/* Create a play list and add files to it */
	player_plist = plist_new(3, wnd_root->m_height - 5);
	if (player_plist == NULL)
	{
		return FALSE;
	}
	for ( i = 0; i < player_num_obj; i ++ )
		plist_add_obj(player_plist, player_objects[i]);
	for ( i = 0; i < player_num_files; i ++ )
		plist_add(player_plist, player_files[i]);

	/* Load saved play list if files list is empty */
	if (!player_num_files && !player_num_obj)
		plist_add(player_plist, "~/mpfc.m3u");

	/* Initialize history lists */
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
		player_hist_lists[i] = hist_list_new();

	/* Initialize playing thread */
	pthread_create(&player_tid, NULL, player_thread, NULL);

	/* Get volume */
	outp_get_volume(pmng_cur_out, &l, &r);
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
	if (cfg_get_var_int(cfg_list, "play_from_stop") && 
			!player_num_files && !player_num_obj)
	{
		player_status = cfg_get_var_int(cfg_list, "player_status");
		player_start = cfg_get_var_int(cfg_list, "player_start") - 1;
		player_end = cfg_get_var_int(cfg_list, "player_end") - 1;
		if (player_status != PLAYER_STATUS_STOPPED)
			player_play(cfg_get_var_int(cfg_list, "cur_song"),
					cfg_get_var_int(cfg_list, "cur_time"));
	}

	/* Exit */
	return TRUE;
} /* End of 'player_init' function */

/* Unitialize player */
void player_deinit( void )
{
	int i;
	
	/* Save information about place in song where we stop */
	cfg_set_var_int(cfg_list, "cur_song", 
			player_plist->m_cur_song);
	cfg_set_var_int(cfg_list, "cur_time", player_cur_time);
	cfg_set_var_int(cfg_list, "player_status", player_status);
	cfg_set_var_int(cfg_list, "player_start", player_start + 1);
	cfg_set_var_int(cfg_list, "player_end", player_end + 1);
	player_save_cfg_vars(cfg_list, "cur_song;cur_time;player_status;"
			"player_start;player_end");
	
	/* End playing thread */
	sat_free();
	player_end_track = TRUE;
	player_end_thread = TRUE;
	pthread_join(player_tid, NULL);
	player_end_thread = FALSE;
	player_tid = 0;
	
	/* Uninitialize plugin manager */
	pmng_free();

	/* Uninitialize key bindings */
	kbind_free();

	/* Uninitialize history lists */
	for ( i = 0; i < PLAYER_NUM_HIST_LISTS; i ++ )
		hist_list_free(player_hist_lists[i]);
	
	/* Destroy screen window */
	wnd_destroy(wnd_root);
	
	/* Destroy all objects */
	if (player_plist != NULL)
	{
		/* Save play list */
		if (cfg_get_var_int(cfg_list, "save_playlist_on_exit"))
			plist_save(player_plist, "~/mpfc.m3u");
		
		plist_free(player_plist);
		player_plist = NULL;
	}
	undo_free(player_ul);

	/* Uninitialize configuration manager */
	cfg_free();
} /* End of 'player_deinit' function */

/* Run player */
bool_t player_run( void )
{
	/* Run window message loop */
	wnd_run(wnd_root);
	return TRUE;
} /* End of 'player_run' function */

/* Parse program command line */
bool_t player_parse_cmd_line( int argc, char *argv[] )
{
	int i;

	/* Get options */
	for ( i = 1; i < argc && argv[i][0] == '-'; i ++ )
	{
		char name[80], val[256];
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
		memcpy(name, &str[name_start], name_end - name_start + 1);
		name[name_end - name_start + 1] = 0;

		/* Load object */
		if (!strcmp(name, "obj"))
		{
			if (i < argc - 1 && player_num_obj < 10)
				strcpy(player_objects[player_num_obj ++], argv[++i]);
			continue;
		}

		/* We have no value - assume it "1" */
		if (name_end == strlen(str) - 1)
		{
			strcpy(val, "1");
		}
		/* Extract value */
		else
		{
			strcpy(val, &str[name_end + 2]);
		}
		
		/* Set respective variable */
		cfg_set_var(cfg_list, name, val);
	}

	/* Get file names from command line */
	player_num_files = argc - i;
	player_files = &argv[i];
	return TRUE;
} /* End of 'player_parse_cmd_line' function */

/* Handle key function */
void player_handle_key( wnd_t *wnd, dword data )
{
	kbind_key2buf((int)data);
} /* End of 'player_handle_key' function */

/* Handle mouse left button click */
void player_handle_mouse_click( wnd_t *wnd, dword data )
{
	strcpy(player_msg, "Mouse button clicked");
} /* End of 'player_handle_mouse_click' function */

/* Display player function */
void player_display( wnd_t *wnd, dword data )
{
	int i;
	song_t *s = NULL;

	/* Clear the screen */
	wnd_clear(wnd, FALSE);
	
	/* Display head */
	wnd_move(wnd, 0, 0);
	if (player_plist->m_cur_song == -1)
	{
		col_set_color(wnd, COL_EL_ABOUT);
		wnd_printf(wnd, _("SG Software Media Player For Console\n"
				"version 1.0alpha\n"));
		col_set_color(wnd, COL_EL_DEFAULT);
	}
	else
	{
		char title[80];
		int t;
		bool_t show_rem;
		
		s = player_plist->m_list[player_plist->m_cur_song];
		if (strlen(s->m_title) >= wnd->m_width - 1)
		{
			memcpy(title, s->m_title, wnd->m_width - 4);
			strcpy(&title[wnd->m_width - 4], "...");
		}
		else
			strcpy(title, s->m_title);
		col_set_color(wnd, COL_EL_CUR_TITLE);
		wnd_printf(wnd, "%s\n", title);
		col_set_color(wnd, COL_EL_CUR_TIME);
		t = (show_rem = cfg_get_var_int(cfg_list, "show_time_remaining")) ? 
			s->m_len - player_cur_time : player_cur_time;
		wnd_printf(wnd, "%s%i:%02i/%i:%02i\n", 
				show_rem ? "-" : "", t / 60, t % 60,
				s->m_len / 60, s->m_len % 60);
		col_set_color(wnd, COL_EL_DEFAULT);
	}

	/* Display play modes */
	col_set_color(wnd, COL_EL_PLAY_MODES);
	if (cfg_get_var_int(cfg_list, "shuffle_play"))
	{
		wnd_move(wnd, wnd->m_width - 13, 0);
		wnd_printf(wnd, "Shuffle");
	}
	if (cfg_get_var_int(cfg_list, "loop_play"))
	{
		wnd_move(wnd, wnd->m_width - 5, 0);
		wnd_printf(wnd, "Loop");
	}
	col_set_color(wnd, COL_EL_DEFAULT);

	/* Display different slidebars */
	player_display_slider(wnd, 0, 2, wnd->m_width - 24, 
			player_cur_time, (s == NULL) ? 0 : s->m_len);
	player_display_slider(wnd, wnd->m_width - 22, 1, 20, 
			player_balance, 100.);
	player_display_slider(wnd, wnd->m_width - 22, 2, 20, 
			player_volume, 100.);
	
	/* Display play list */
	plist_display(player_plist, wnd);

	/* Print message */
	col_set_color(wnd, COL_EL_STATUS);
	wnd_move(wnd, 0, wnd->m_height - 2);
	wnd_printf(wnd, "%s", player_msg);
	col_set_color(wnd, COL_EL_DEFAULT);

	/* Hide cursor */
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'player_display' function */

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
		ebox_move(box, FALSE, box->m_len);
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

	inp_seek(s->m_inp, new_time);
	player_cur_time = new_time;
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'player_seek' function */

/* Play song */
void player_play( int song, int start_time )
{
	song_t *s;
	
	/* Check that we have anything to play */
	if (song < 0 || song >= player_plist->m_len ||
			(s = player_plist->m_list[song]) == NULL)
	{
		player_plist->m_cur_song = -1;
		return;
	}

	/* End current playing */
	player_end_play();

	/* Start new playing thread */
	cfg_set_var(cfg_list, "cur_song_name", 
			util_get_file_short_name(s->m_file_name));
	player_plist->m_cur_song = song;
	player_cur_time = start_time;
//	player_status = PLAYER_STATUS_PLAYING;
} /* End of 'player_play' function */

/* End playing song */
void player_end_play( void )
{
	player_plist->m_cur_song = -1;
	player_end_track = TRUE;
//	player_status = PLAYER_STATUS_STOPPED;
	while (player_timer_tid)
		util_delay(0, 100000);
	cfg_set_var(cfg_list, "cur_song_name", "");
} /* End of 'player_end_play' function */

/* Player thread function */
void *player_thread( void *arg )
{
	bool_t no_outp = FALSE;
	
	/* Main loop */
	while (!player_end_thread)
	{
		song_t *s;
		int ch, freq;
		dword fmt;
		song_info_t si;

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
	
		/* Get song length and information at first */
		if (cfg_get_var_int(cfg_list, "update_song_len_on_play"))
			s->m_len = inp_get_len(s->m_inp, s->m_file_name);
		song_update_info(s);

		/* Start playing */
		if (!inp_start(s->m_inp, s->m_file_name))
		{
			player_next_track();
			error_set_code(ERROR_UNKNOWN_FILE_TYPE);
			strcpy(player_msg, error_text);
			wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
			continue;
		}
		if (player_cur_time > 0)
			inp_seek(s->m_inp, player_cur_time);
		no_outp = inp_get_flags(s->m_inp) & INP_NO_OUTP;

		/* Start output plugin */
		if (!no_outp && (pmng_cur_out == NULL || 
				(!cfg_get_var_int(cfg_list, "silent_mode") && 
					!outp_start(pmng_cur_out))))
		{
			strcpy(player_msg, _("Unable to initialize output plugin"));
//			wnd_send_msg(wnd_root, WND_MSG_USER, PLAYER_MSG_END_TRACK);
			inp_end(s->m_inp);
			player_status = PLAYER_STATUS_STOPPED;
			wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
			continue;
		}

		/* Set audio parameters */
		if (!no_outp)
		{
			inp_get_audio_params(s->m_inp, &ch, &freq, &fmt);
			outp_set_channels(pmng_cur_out, 2);
			outp_set_freq(pmng_cur_out, freq);
			outp_set_fmt(pmng_cur_out, fmt);
		}

		/* Save current input plugin */
		player_inp = s->m_inp;

		/* Set equalizer */
		inp_set_eq(s->m_inp);

		/* Start timer thread */
		pthread_create(&player_timer_tid, NULL, player_timer_func, 0);
		//wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
	
		/* Play */
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
					inp_set_eq(s->m_inp);
				}
				
				/* Get stream from input plugin */
				if (size = inp_get_stream(s->m_inp, buf, size))
				{
					int new_ch, new_freq;
					dword new_fmt;
					
					/* Update audio parameters if they have changed */
					if (!no_outp)
					{
						inp_get_audio_params(s->m_inp, &new_ch, &new_freq, 
								&new_fmt);
						if (ch != new_ch || freq != new_freq || fmt != new_fmt)
						{
							ch = new_ch;
							freq = new_freq;
							fmt = new_fmt;
						
							outp_flush(pmng_cur_out);
							outp_set_channels(pmng_cur_out, 2);
							outp_set_freq(pmng_cur_out, freq);
							outp_set_fmt(pmng_cur_out, fmt);
						}

						/* Apply effects */
						size = pmng_apply_effects(buf, size, fmt, freq, 2);
					
						/* Send to output plugin */
						outp_play(pmng_cur_out, buf, size);
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
			outp_flush(pmng_cur_out);

		/* Stop timer thread */
		player_stop_timer();

		/* End playing */
		inp_end(s->m_inp);
		player_inp = NULL;

		/* End output plugin */
		if (!no_outp)
			outp_end(pmng_cur_out);

		/* Send message about track end */
		if (!player_end_track)
			player_next_track();

		/* Update screen */
		wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
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
					wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
				}
			}
			else if (tm - player_cur_time)
			{
				player_cur_time = tm;
				wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
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

/* Process add file dialog */
void player_add_dialog( void )
{
	file_input_box_t *fin;

	/* Create edit box for path input */
	fin = fin_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, _("Enter file (or directory) name: "));
	if (fin != NULL)
	{
		((editbox_t *)fin)->m_hist_list = 
			player_hist_lists[PLAYER_HIST_LIST_ADD];

		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
			plist_add(player_plist, fin->m_box.m_text);

		/* Destroy edit box */
		wnd_destroy(fin);
	}
} /* End of 'player_add_dialog' function */

/* Process save play list dialog */
void player_save_dialog( void )
{
	file_input_box_t *fin;

	/* Create edit box for path input */
	fin = fin_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width, 
			_("Enter file name: "));
	if (fin != NULL)
	{
		((editbox_t *)fin)->m_hist_list = 
			player_hist_lists[PLAYER_HIST_LIST_SAVE];

		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
		{
			bool_t res = plist_save(player_plist, fin->m_box.m_text);
			strcpy(player_msg, (res) ? _("Play list saved") : 
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
	sprintf(player_msg, _("Removed %i songs"), was - player_plist->m_len);
} /* End of 'player_rem_dialog' function */

/* Process sort play list dialog */
void player_sort_dialog( void )
{
	choice_ctrl_t *ch;
	char choice;
	int t;
	bool_t g;

	/* Get sort globalness parameter */
	ch = choice_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width,
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
	ch = choice_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width,
		1, _("Sort by: (T)itle, (F)ile name, (P)ath and file name"), "tfp");
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
	}
	plist_sort(player_plist, g, t);
} /* End of 'player_sort_dialog' function */

/* Process search play list dialog */
void player_search_dialog( int criteria )
{
	editbox_t *ebox;

	/* Display edit box for entering search text */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width,
			1, 256, _("Enter search string: "), "");
	ebox->m_hist_list = 
		player_hist_lists[PLAYER_HIST_LIST_SEARCH];
	if (ebox != NULL)
	{
		wnd_run(ebox);

		/* Save search parameters and search */
		if (ebox->m_last_key == '\n')
		{
			strcpy(player_search_string, ebox->m_text);
			player_search_criteria = criteria;
			if (!plist_search(player_plist, player_search_string, 1, 
						criteria))
				strcpy(player_msg, _("String not found"));
			else
				strcpy(player_msg, _("String found"));
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
	int i, y = 1;

	/* Get song object */
	if (player_plist->m_sel_end < 0 || !player_plist->m_len)
		return;
	else
		s = player_plist->m_list[player_plist->m_sel_end];

	/* Update song information */
	song_update_info(s);

	/* Check if there exists song information */
	if (s->m_info == NULL || 
			(!s->m_info->m_not_own_present && !(*(s->m_info->m_own_data))))
		return;

	/* Create info dialog */
	dlg = dlg_new(wnd_root, 2, 2, wnd_root->m_width - 4, 21, 
			cfg_get_var_int(cfg_list, "info_editor_show_full_name") ? 
			s->m_file_name : util_get_file_short_name(s->m_file_name));
	if (!s->m_info->m_only_own)
	{
		name = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Song name: "), s->m_info->m_name);
		artist = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Artist name: "), s->m_info->m_artist);
		album = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Album name: "), s->m_info->m_album);
		year = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Year: "), s->m_info->m_year);
		track = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Track No: "), s->m_info->m_track);
		comments = ebox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 1, 
				256, _("Comments: "), s->m_info->m_comments);
		genre = cbox_new((wnd_t *)dlg, 2, y ++, WND_WIDTH(dlg) - 6, 12,
				_("Genre: "));
		glist = inp_get_glist(s->m_inp);
		for ( i = 0; glist != NULL && i < glist->m_size; i ++ )
			cbox_list_add(genre, glist->m_list[i].m_name);
		cbox_move_list_cursor(genre, FALSE, 
				(s->m_info->m_genre == GENRE_ID_UNKNOWN ||
				 s->m_info->m_genre == GENRE_ID_OWN_STRING) ? -1 : 
				s->m_info->m_genre, FALSE, TRUE);
		if (s->m_info->m_genre == GENRE_ID_OWN_STRING)
			cbox_set_text(genre, s->m_info->m_genre_data.m_text);
	}

	/* Display own data */
	label_new(WND_OBJ(dlg), 2, y + 1, WND_WIDTH(dlg) - 6, 
			WND_HEIGHT(dlg) - y - 1, s->m_info->m_own_data);

	/* Display dialog */
	wnd_run(dlg);

	/* Save */
	if (dlg->m_ok)
	{
		/* Remember information */
		strcpy(s->m_info->m_name, name->m_text);
		strcpy(s->m_info->m_artist, artist->m_text);
		strcpy(s->m_info->m_album, album->m_text);
		strcpy(s->m_info->m_year, year->m_text);
		strcpy(s->m_info->m_comments, comments->m_text);
		strcpy(s->m_info->m_track, track->m_text);
		if (genre->m_list_cursor < 0)
		{
			s->m_info->m_genre = GENRE_ID_OWN_STRING;
			strcpy(s->m_info->m_genre_data.m_text, genre->m_text);
		}
		else
			s->m_info->m_genre = genre->m_list_cursor;
		s->m_info->m_not_own_present = TRUE;
	
		/* Get song length and information at first */
		inp_save_info(s->m_inp, s->m_file_name, s->m_info);

		/* Update */
		song_update_info(s);
	}
	
	wnd_destroy(dlg);
} /* End of 'player_info_dialog' function */

/* Show help screen */
void player_help( void )
{
	help_screen_t *h;

	h = help_new(wnd_root, 0, 0, wnd_root->m_width, wnd_root->m_height);
	wnd_run(h);
	wnd_destroy(h);
} /* End of 'player_help' function */

/* Start next track */
void player_next_track( void )
{
	player_skip_songs(1);
} /* End of 'player_next_track' function */

/* Handle non-digit key (place it to buffer) */
void player_handle_non_digit( int key )
{
} /* End of 'player_handle_non_digit' function */

/* Execute key action */
void player_exec_key_action( void )
{
} /* End of 'player_exec_key_action' function */

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
	
	col_set_color(wnd, COL_EL_SLIDER);
	wnd_move(wnd, x, y);
	slider_pos = (range) ? (pos * width / range) : 0;
	for ( i = 0; i <= width; i ++ )
	{
		if (i == slider_pos)
			wnd_printf(wnd, "O");
		else 
			wnd_printf(wnd, "=");
	}
	col_set_color(wnd, COL_EL_DEFAULT);
} /* End of 'player_display_slider' function */

/* Process equalizer dialog */
void player_eq_dialog( void )
{
	eq_wnd_t *wnd;

	wnd = eqwnd_new(wnd_root, 0, 0, wnd_root->m_width, wnd_root->m_height);
	wnd_run(wnd);
	wnd_destroy(wnd);
} /* End of 'player_eq_dialog' function */

/* Skip some songs */
void player_skip_songs( int num )
{
	int len, base, song;
	
	if (player_plist == NULL || !player_plist->m_len)
		return;
	
	/* Change current song */
	song = player_plist->m_cur_song;
	len = (player_start < 0) ? player_plist->m_len : 
		(player_end - player_start + 1);
	base = (player_start < 0) ? 0 : player_start;
	if (cfg_get_var_int(cfg_list, "shuffle_play"))
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
		if (cfg_get_var_int(cfg_list, "loop_play"))
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
	if (song == -1)
		player_end_play();
	else
		player_play(song, 0);
} /* End of 'player_skip_songs' function */

/* Launch variables manager */
void player_var_manager( void )
{
	dlgbox_t *dlg;
	listbox_t *var_lb;
	editbox_t *val_eb;
	button_t *btn;
	int i;

	/* Initialize dialog */
	dlg = dlg_new(wnd_root, 2, 2, wnd_root->m_width - 4, 20, 
			_("Variables manager"));
	wnd_register_handler(dlg, WND_MSG_NOTIFY, player_var_mngr_notify);
	var_lb = lbox_new(WND_OBJ(dlg), 2, 2, WND_WIDTH(dlg) - 4, 
			WND_HEIGHT(dlg) - 5, "");
	var_lb->m_minimalizing = FALSE;
	WND_OBJ(var_lb)->m_id = PLAYER_VAR_MNGR_VARS;
	val_eb = ebox_new(WND_OBJ(dlg), 2, WND_HEIGHT(dlg) - 3,
			WND_WIDTH(dlg) - 4, 1, 256, _("Value: "), 
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
				val_eb->m_text);
	}

	/* Free memory */
	wnd_destroy(dlg);
} /* End of 'player_var_manager' function */

/* Launch add object dialog */
void player_add_obj_dialog( void )
{
	editbox_t *ebox;

	/* Create edit box for path input */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, 1, 256, _("Enter object name: "), "");
	if (ebox != NULL)
	{
		ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_ADD_OBJ];
		
		/* Run message loop */
		wnd_run(ebox);

		/* Add object if enter was pressed */
		if (ebox->m_last_key == '\n')
			plist_add_obj(player_plist, ebox->m_text);

		/* Destroy edit box */
		wnd_destroy(ebox);
	}
} /* End of 'player_add_obj_dialog' function */

/* Handle action */
void player_handle_action( int action )
{
	char str[10];
	editbox_t *ebox;
	bool_t dont_change_repval = FALSE;
	int was_pos;
	int was_song, was_time;

	/* Clear message string */
	strcpy(player_msg, "");

	was_pos = player_plist->m_sel_end;
	was_song = player_plist->m_cur_song;
	was_time = player_cur_time;
	switch (action)
	{
	/* Exit MPFC */
	case KBIND_QUIT:
		wnd_send_msg(wnd_root, WND_MSG_CLOSE, 0);
		break;

	/* Show help screen */
	case KBIND_HELP:
		player_help();
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
				player_plist->m_height : 
				player_plist->m_height * player_repval, TRUE);
		break;
	case KBIND_SCREEN_UP:
		plist_move(player_plist, (player_repval == 0) ? 
				-player_plist->m_height : 
				-player_plist->m_height * player_repval, TRUE);
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
		plist_centrize(player_plist);
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
		player_end_play();
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
		player_skip_songs((player_repval) ? player_repval : 1);
		break;

	/* Go to previous song */
	case KBIND_PREV:
		player_skip_songs(-((player_repval) ? player_repval : 1));
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
		player_search_dialog(PLIST_SEARCH_TITLE);
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
			strcpy(player_msg, _("String not found"));
		else
			strcpy(player_msg, _("String found"));
		break;

	/* Show equalizer dialog */
	case KBIND_EQUALIZER:
		player_eq_dialog();
		break;

	/* Set/unset shuffle mode */
	case KBIND_SHUFFLE:
		cfg_set_var_int(cfg_list, "shuffle_play",
				!cfg_get_var_int(cfg_list, "shuffle_play"));
		break;
		
	/* Set/unset loop mode */
	case KBIND_LOOP:
		cfg_set_var_int(cfg_list, "loop_play",
				!cfg_get_var_int(cfg_list, "loop_play"));
		break;

	/* Variables manager */
	case KBIND_VAR_MANAGER:
		player_var_manager();
		break;
	case KBIND_VAR_MINI_MANAGER:
		player_var_mini_mngr();
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
		plist_reload_info(player_plist);
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
		player_exec();
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
		ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, 
				wnd_root->m_width, 1, 5, 
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
				player_repval = atoi(ebox->m_text);
				dont_change_repval = TRUE;
				player_handle_key(wnd_root, ebox->m_last_key);
			}
			
			/* Destroy edit box */
			wnd_destroy(ebox);
		}
		break;
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

/* Launch variables mini-manager */
void player_var_mini_mngr( void )
{
	editbox_t *ebox;
	char name[256], val[256];

	/* Create edit box for variable name input */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, 1, 256, _("Enter variable name: "), "");
	if (ebox == NULL)
		return;
	ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_VAR_NAME];

	/* Run message loop */
	wnd_run(ebox);

	/* Check input */
	if (ebox->m_last_key != '\n' || !ebox->m_len)
	{
		wnd_destroy(ebox);
		return;
	}
	strcpy(name, ebox->m_text);
	wnd_destroy(ebox);

	/* Get value */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, 1, 256, _("Enter variable value: "), "");
	if (ebox == NULL)
		return;
	ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_VAR_VAL];
	wnd_run(ebox);
	if (ebox->m_last_key != '\n')
	{
		wnd_destroy(ebox);
		return;
	}
	strcpy(val, ebox->m_len ? ebox->m_text : "1");
	wnd_destroy(ebox);

	/* Set variable value */
	cfg_set_var(cfg_list, name, val);
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
					lb->m_list[player_var_mngr_pos].m_name, eb->m_text);
			
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
		name = ebox_new(WND_OBJ(d), 4, 3, WND_WIDTH(d) - 5, 1, 256, 
				_("Name: "), "");
		val = ebox_new(WND_OBJ(d), 4, 4, WND_WIDTH(d) - 5, 1, 256, 
				_("Value: "), "");
		wnd_run(d);
		if (d->m_ok)
		{
			int was_len = cfg_list->m_num_vars;

			/* Set variable */
			cfg_set_var(cfg_list, name->m_text, val->m_text);

			/* Update main dialog items */
			if (was_len != cfg_list->m_num_vars)
			{
				lbox_add(lb, name->m_text);
			}
			else
			{
				if (lb->m_cursor >= 0 && 
						!strcmp(lb->m_list[lb->m_cursor].m_name, name->m_text))
					ebox_set_text(eb, val->m_text);
			}
		}
		wnd_destroy(d);
	}
} /* End of 'player_var_mngr_notify' function */

/* Save variables to main configuration file */
void player_save_cfg_vars( cfg_list_t *list, char *vars )
{
	char fname[256], name[80];
	cfg_list_t *tlist;
	int i, j;
	
	if (list == NULL)
		return;

	/* Initialize variables with initial values */
	tlist = (cfg_list_t *)malloc(sizeof(cfg_list_t));
	tlist->m_vars = NULL;
	tlist->m_num_vars = 0;
	tlist->m_db = NULL;

	/* Read rc file */
	sprintf(fname, "%s/.mpfcrc", getenv("HOME"));
	cfg_read_rcfile(tlist, fname);

	/* Update variables */
	for ( i = 0, j = 0;; i ++ )
	{
		/* End of variable name */
		if (vars[i] == ';' || vars[i] == '\0')
		{
			name[j] = 0;
			j = 0;
			cfg_set_var(tlist, name, cfg_get_var(list, name));
			if (!vars[i])
				break;
		}
		else
			name[j ++] = vars[i];
	}

	/* Save temporary list to configuration file */
	player_save_cfg_list(tlist, fname);

	/* Free temporary list */
	cfg_free_list(tlist);
} /* End of 'player_save_cfg_vars' function */

/* Save configuration list */
void player_save_cfg_list( cfg_list_t *list, char *fname )
{
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
	outp_set_volume(pmng_cur_out, l, r);
} /* End of 'player_update_vol' function */

/* Execute a command */
void player_exec( void )
{
	editbox_t *ebox;

	/* Create edit box for command */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, 1, 256, _("Enter command: "), "");
	if (ebox != NULL)
	{
		ebox->m_hist_list = player_hist_lists[PLAYER_HIST_LIST_EXEC];
		
		/* Run message loop */
		wnd_run(ebox);

		/* Execute if enter was pressed */
		if (ebox->m_last_key == '\n')
		{
			wnd_close_curses();
			system(ebox->m_text);
			getchar();
			wnd_restore_curses();
		}

		/* Destroy edit box */
		wnd_destroy(ebox);
	}
} /* End of 'player_exec' function */

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

/* Advanced search dialog */
void player_advanced_search_dialog( void )
{
	choice_ctrl_t *ch;
	char choice;
	int t;

	/* Get search criteria */
	ch = choice_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width,
		1, _("Search for: (T)itle, (N)ame, (A)rtist, A(l)bum, (Y)ear, "
			"(G)enre, (O)wn data"), 
		"tnalygo");
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
	case 'o':
		t = PLIST_SEARCH_OWN;
		break;
	}
	player_search_dialog(t);
} /* End of 'player_advanced_search_dialog' function */

/* Handle 'title_format' variable setting */
void player_handle_var_title_format( char *name )
{
	int i;

	for ( i = 0; i < player_plist->m_len; i ++ )
		song_get_title_from_info(player_plist->m_list[i]);
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'player_handle_var_title_format' function */

/* Handle 'output_plugin' variable setting */
void player_handle_var_outp( char *name )
{
	/* Choose new output plugin */
	int i;
	for ( i = 0; i < pmng_num_outp; i ++ )
		if (!strcmp(pmng_outp[i]->m_name, 
					cfg_get_var(cfg_list, "output_plugin")))
		{
			pmng_cur_out = pmng_outp[i];
			break;
		}
} /* End of 'player_handle_var_outp' function */

/* Return to the last time */
void player_time_back( void )
{
	player_play(player_last_song, player_last_song_time);
} /* End of 'player_time_back' function */

/* End of 'player.c' file */

