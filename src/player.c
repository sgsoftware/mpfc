/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.c
 * PURPOSE     : SG Konsamp. Main player functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.04.2003
 * NOTE        : None.
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
#include "cfg.h"
#include "choice_ctrl.h"
#include "dlgbox.h"
#include "editbox.h"
#include "error.h"
#include "file_input.h"
#include "help_screen.h"
#include "listbox.h"
#include "menu.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "util.h"

/* Files for player to play */
int player_num_files = 0;
char **player_files = NULL;

/* Play list */
plist_t *player_plist = NULL;

/* Command repeat value */
int player_repval = 0;

/* Search string and position */
char player_search_string[256] = "";

/* Message text */
char player_msg[80] = "";

/* Player thread ID */
pthread_t player_tid = 0;

/* Player termination flag */
bool player_end_thread = FALSE;

/* Timer thread ID */
pthread_t player_timer_tid = 0;

/* Timer termination flag */
bool player_end_timer = FALSE;
bool player_end_track = FALSE;

/* Current song playing time */
int player_cur_time = 0;

/* Player status */
int player_status = PLAYER_STATUS_STOPPED;

/* Initialize player */
bool player_init( int argc, char *argv[] )
{
	int i;

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
	
	/* Initialize plugin manager */
	pmng_init();

	/* Create a play list and add files to it */
	player_plist = plist_new(3, wnd_root->m_height - 6);
	if (player_plist == NULL)
	{
		return FALSE;
	}
	for ( i = 0; i < player_num_files; i ++ )
		plist_add(player_plist, player_files[i]);

	/* Initialize playing thread */
	pthread_create(&player_tid, NULL, player_thread, NULL);

	/* Exit */
	return TRUE;
} /* End of 'player_init' function */

/* Unitialize player */
void player_deinit( void )
{
	/* End playing thread */
	player_end_track = TRUE;
	player_end_thread = TRUE;
	pthread_join(player_tid, NULL);
	player_end_thread = FALSE;
	player_tid = 0;
	
	/* Uninitialize plugin manager */
	pmng_free();
	
	/* Destroy screen window */
	wnd_destroy(wnd_root);
	
	/* Destroy all objects */
	if (player_plist != NULL)
	{
		plist_free(player_plist);
		player_plist = NULL;
	}
} /* End of 'player_deinit' function */

/* Run player */
bool player_run( void )
{
	/* Run window message loop */
	wnd_run(wnd_root);
	return TRUE;
} /* End of 'player_run' function */

/* Parse program command line */
bool player_parse_cmd_line( int argc, char *argv[] )
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
		for ( name_end = name_start; str[name_end] && str[name_end] != '='; 
				name_end ++ );
		name_end --;

		/* Extract variable name */
		memcpy(name, &str[name_start], name_end - name_start + 1);
		name[name_end - name_start + 1] = 0;

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
		cfg_set_var(name, val);
	}

	/* Get file names from command line */
	player_num_files = argc - i;
	player_files = &argv[i];
	return TRUE;
} /* End of 'player_parse_cmd_line' function */

/* Handle key function */
void player_handle_key( wnd_t *wnd, dword data )
{
	char str[10];
	editbox_t *ebox;
	bool dont_change_repval = FALSE;
	int key = (int)data;

	/* Clear message string */
	strcpy(player_msg, "");

	switch (key)
	{
	/* Exit Konsamp */
	case 'q':
	case 'Q':
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;

	/* Show help screen */
	case '?':
		player_help();
		break;

	/* Move cursor */
	case 'j':
	case KEY_DOWN:
		plist_move(player_plist, (player_repval == 0) ? 1 : player_repval, 
				TRUE);
		break;
	case 'k':
	case KEY_UP:
		plist_move(player_plist, (player_repval == 0) ? -1 : -player_repval, 
				TRUE);
		break;
	case 'd':
	case KEY_NPAGE:
		plist_move(player_plist, (player_repval == 0) ? 
				player_plist->m_height : 
				player_plist->m_height * player_repval, TRUE);
		break;
	case 'u':
	case KEY_PPAGE:
		plist_move(player_plist, (player_repval == 0) ? 
				-player_plist->m_height : 
				-player_plist->m_height * player_repval, TRUE);
		break;
	case 'G':
		plist_move(player_plist, (player_repval == 0) ? 
				player_plist->m_len - 1 : player_repval - 1, FALSE);
		break;

	/* Seek song */
	case 'l':
		player_seek((player_repval == 0) ? 10 : 10 * player_repval, TRUE);
		break;
	case 'h':
		player_seek((player_repval == 0) ? -10 : -10 * player_repval, TRUE);
		break;
	case 'g':
		player_seek((player_repval == 0) ? 0 : player_repval, FALSE);
		break;

	/* Centrize view */
	case 'C':
		plist_centrize(player_plist);
		break;

	/* Enter visual mode */
	case 'V':
		player_plist->m_visual = !player_plist->m_visual;
		break;

	/* Resume playing */
	case 'x':
		if (player_status == PLAYER_STATUS_PAUSED)
			player_status = PLAYER_STATUS_PLAYING;
		else
			player_play();
		break;

	/* Pause */
	case 'c':
		if (player_status == PLAYER_STATUS_PLAYING)
			player_status = PLAYER_STATUS_PAUSED;
		else if (player_status == PLAYER_STATUS_PAUSED)
			player_status = PLAYER_STATUS_PLAYING;
		break;

	/* Stop */
	case 'v':
		player_end_play();
		break;

	/* Play song */
	case '\n':
		player_plist->m_cur_song = player_plist->m_sel_end;
		player_play();
		break;

	/* Go to next song */
	case 'b':
		player_plist->m_cur_song += ((player_repval) ? player_repval : 1);
		if (player_plist->m_cur_song >= player_plist->m_len)
		{
			player_plist->m_cur_song = -1;
			player_end_play();
		}
		else
		{
			player_play();
		}
		break;

	/* Go to previous song */
	case 'z':
		player_plist->m_cur_song -= ((player_repval) ? player_repval : 1);
		if (player_plist->m_cur_song < 0)
		{
			player_plist->m_cur_song = -1;
			player_end_play();
		}
		else
		{
			player_play();
		}
		break;

	/* Add a file */
	case 'a':
		player_add_dialog();
		break;

	/* Save play list */
	case 's':
		player_save_dialog();
		break;

	/* Remove song(s) */
	case 'r':
		player_rem_dialog();
		break;

	/* Sort play list */
	case 'S':
		player_sort_dialog();
		break;

	/* Song info dialog */
	case 'i':
		player_info_dialog();
		break;

	/* Search */
	case '/':
		player_search_dialog();
		break;

	/* Find next/previous search match */
	case 'n':
	case 'N':
		if (!plist_search(player_plist, player_search_string, 
					(key == 'n') ? 1 : -1))
			strcpy(player_msg, _("String not found"));
		else
			strcpy(player_msg, _("String found"));
		break;
		
	/* Digit means command repeation value edit */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		/* Create edit box */
		str[0] = key;
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
				
				if (ebox->m_last_key >= 'A' && ebox->m_last_key <= 'z')
					player_handle_key(wnd, ebox->m_last_key);
			}
			
			/* Destroy edit box */
			wnd_destroy(ebox);
		}
		break;
	}

	/* Flush repeat value */
	if (!dont_change_repval)
		player_repval = 0;
} /* End of 'player_handle_key' function */

/* Handle mouse left button click */
void player_handle_mouse_click( wnd_t *wnd, dword data )
{
	strcpy(player_msg, "Mouse button clicked");
} /* End of 'player_handle_mouse_click' function */

/* Display player function */
void player_display( wnd_t *wnd, dword data )
{
	int i, song_time_len = wnd_root->m_width - 15, slider_pos;
	song_t *s = NULL;
	
	/* Clear the screen */
	wnd_clear(wnd, FALSE);
	
	/* Display head */
	wnd_move(wnd, 0, 0);
	if (player_plist->m_cur_song == -1)
		wnd_printf(wnd, "SG Software Media Player For Console\n"
				"version 0.1\n");
	else
	{
		s = player_plist->m_list[player_plist->m_cur_song];
		wnd_printf(wnd, "%s\n%i:%02i/%i:%02i\n", s->m_title, 
				player_cur_time / 60, player_cur_time % 60,
				s->m_len / 60, s->m_len % 60);
	}

	/* Display different slidebars */
	wnd_move(wnd, 0, 2);
	slider_pos = (s == NULL || !s->m_len) ? 0 : 
		(player_cur_time * song_time_len / s->m_len);
	for ( i = 0; i < song_time_len; i ++ )
	{
		if (i == slider_pos)
			wnd_printf(wnd, "O");
		else
			wnd_printf(wnd, "=");
	}
	wnd_printf(wnd, "\n");
	
	/* Display play list */
	plist_display(player_plist, wnd);

	/* Print message */
	wnd_move(wnd, 0, wnd->m_height - 2);
	wnd_printf(wnd, "%s\n", player_msg);

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
void player_seek( int sec, bool rel )
{
	song_t *s;
	int new_time;
	
	if (player_plist->m_cur_song == -1)
		return;

	s = player_plist->m_list[player_plist->m_cur_song];

	(rel) ? (new_time = player_cur_time + sec) : (new_time = sec);
	if (new_time < 0)
	{
		sec = ((rel) ? -player_cur_time : 0);
		new_time = 0;
	}
	else if (new_time > s->m_len)
	{
		sec = ((rel) ? s->m_len - player_cur_time : s->m_len);
		new_time = s->m_len;
	}
	
	s->m_inp->m_fl.m_seek(new_time - player_cur_time);
	player_cur_time = new_time;
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'player_seek' function */

/* Play song */
void player_play( void )
{
	song_t *s;
	
	/* Check that we have anything to play */
	if (player_plist->m_cur_song == -1 || 
			(s = player_plist->m_list[player_plist->m_cur_song]) == NULL)
		return;

	/* End current playing */
	player_end_play();

	/* Start new playing thread */
	player_status = PLAYER_STATUS_PLAYING;
} /* End of 'player_play' function */

/* End playing song */
void player_end_play( void )
{
	player_end_track = TRUE;
	player_status = PLAYER_STATUS_STOPPED;
} /* End of 'player_end_play' function */

/* Player thread function */
void *player_thread( void *arg )
{
	/* Main loop */
	while (!player_end_thread)
	{
		song_t *s;
		int ch, freq, bits;
		inp_func_list_t ifl;
		outp_func_list_t ofl;
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
		player_status = PLAYER_STATUS_PLAYING;
		player_end_track = FALSE;
		wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
	
		/* Get song length and information at first */
		ifl = s->m_inp->m_fl;
		s->m_len = ifl.m_get_len(s->m_file_name);
		song_update_info(s);

		/* Start playing */
		if (!ifl.m_start(s->m_file_name))
		{
			player_next_track();
			continue;
		}

		/* Start output plugin */
		if (pmng_cur_out == NULL || (!cfg_get_var_int("silent_mode") && 
					!pmng_cur_out->m_fl.m_start()))
		{
			strcpy(player_msg, "Unable to initialize output plugin");
//			wnd_send_msg(wnd_root, WND_MSG_USER, PLAYER_MSG_END_TRACK);
			ifl.m_end();
			player_status = PLAYER_STATUS_STOPPED;
			wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
			continue;
		}
		ofl = pmng_cur_out->m_fl;

		/* Set audio parameters */
		ifl.m_get_audio_params(&ch, &freq, &bits);
		ofl.m_set_channels(2);
		ofl.m_set_freq(freq);
		ofl.m_set_bits(bits);

		/* Start timer thread */
		pthread_create(&player_timer_tid, NULL, player_timer_func, 0);
	
		/* Play */
		while (!player_end_track)
		{
			byte buf[8192];
			int size = 8192;
			struct timespec tv;

			if (player_status == PLAYER_STATUS_PLAYING)
			{
				if (size = ifl.m_get_stream(buf, size))
				{
					ofl.m_play(buf, size);
				}
				else
				{
					break;
				}
			}
		
			/* Sleep a little */
			util_delay(0, 100000L);
		}

		/* Stop timer thread */
		player_stop_timer();

		/* End playing */
		ifl.m_end();

		/* End output plugin */
		ofl.m_end();

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

		/* Update timer */
		if (player_status == PLAYER_STATUS_PAUSED)
		{
			t = new_t;
		}
		else if (new_t > t)
		{
			player_cur_time += (new_t - t);
			t = new_t;
			wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
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
		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
		{
			int was = player_plist->m_len;
			
			if (plist_add(player_plist, fin->m_box.m_text))
			{
				sprintf(player_msg, _("Added %i songs"), 
						player_plist->m_len - was);
			}
		}

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
		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
		{
			bool res = plist_save(player_plist, fin->m_box.m_text);
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
	bool g;

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
		t = PLIST_SORT_BY_FNAME;
		break;
	case 'p':
		t = PLIST_SORT_BY_PATH;
		break;
	}
	plist_sort(player_plist, g, t);
} /* End of 'player_sort_dialog' function */

/* Process search play list dialog */
void player_search_dialog( void )
{
	editbox_t *ebox;

	/* Display edit box for entering search text */
	ebox = ebox_new(wnd_root, 0, wnd_root->m_height - 1, wnd_root->m_width,
			1, 256, _("Enter search string: "), "");
	if (ebox != NULL)
	{
		wnd_run(ebox);

		/* Save search parameters and search */
		if (ebox->m_last_key == '\n')
		{
			strcpy(player_search_string, ebox->m_text);
			if (!plist_search(player_plist, player_search_string, 1))
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
	editbox_t *name, *album, *artist, *year, *comments;
	listbox_t *genre;
	genre_list_t *glist;
	int i;

	/* Get song object */
	if (player_plist->m_sel_end < 0)
		return;
	else
		s = player_plist->m_list[player_plist->m_sel_end];

	/* Update song information */
	song_update_info(s);

	/* Check if there exists song information */
	if (s->m_info == NULL)
		return;

	/* Create info dialog */
	dlg = dlg_new(wnd_root, 2, 2, wnd_root->m_width - 4, 20, s->m_file_name);
	name = ebox_new((wnd_t *)dlg, 2, 1, wnd_root->m_width - 10, 1, 
			30, "Song name: ", s->m_info->m_name);
	artist = ebox_new((wnd_t *)dlg, 2, 2, wnd_root->m_width - 10, 1, 
			30, "Artist name: ", s->m_info->m_artist);
	album = ebox_new((wnd_t *)dlg, 2, 3, wnd_root->m_width - 10, 1, 
			30, "Album name: ", s->m_info->m_album);
	year = ebox_new((wnd_t *)dlg, 2, 4, wnd_root->m_width - 10, 1, 
			4, "Year: ", s->m_info->m_year);
	comments = ebox_new((wnd_t *)dlg, 2, 5, wnd_root->m_width - 10, 1, 
			4, "Comments: ", s->m_info->m_comments);
	genre = lbox_new((wnd_t *)dlg, 2, 6, wnd_root->m_width - 10, 13,
			"Genre: ");
	glist = s->m_inp->m_fl.m_glist;
	for ( i = 0; i < glist->m_size; i ++ )
		lbox_add(genre, glist->m_list[i].m_name);
	lbox_move_cursor(genre, FALSE, 
			(s->m_info->m_genre == GENRE_ID_UNKNOWN) ? -1 : 
			s->m_info->m_genre, FALSE);
	wnd_run(dlg);

	/* Save */
	if (dlg->m_ok)
	{
		inp_func_list_t ifl;

		/* Remember information */
		strcpy(s->m_info->m_name, name->m_text);
		strcpy(s->m_info->m_artist, artist->m_text);
		strcpy(s->m_info->m_album, album->m_text);
		strcpy(s->m_info->m_year, year->m_text);
		strcpy(s->m_info->m_comments, comments->m_text);
		s->m_info->m_genre = genre->m_cursor;
	
		/* Get song length and information at first */
		ifl = s->m_inp->m_fl;
		ifl.m_save_info(s->m_file_name, s->m_info);

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
	player_plist->m_cur_song ++;
	if (player_plist->m_cur_song >= player_plist->m_len)
		player_plist->m_cur_song = -1;
	else
		player_play();
} /* End of 'player_next_track' function */

/* End of 'player.c' file */

