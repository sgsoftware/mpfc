/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_kbd.c
 * PURPOSE     : MPFC Window Library. Keyboard functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'wnd_kbd'.
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

#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <term.h>
#include "types.h"
#include "wnd.h"
#include "wnd_kbd.h"
#include "wnd_msg.h"
#include "util.h"

/* Initialize keyboard management system */
wnd_kbd_data_t *wnd_kbd_init( wnd_t *wnd_root )
{
	/* Create data */
	wnd_kbd_data_t *data = (wnd_kbd_data_t *)malloc(sizeof(wnd_kbd_data_t));
	data->m_end_thread = FALSE;
	data->m_wnd_root = wnd_root;

	/* Initialize escape sequences list */
	wnd_kbd_init_seq(data);
	
	/* Start thread */
	if (pthread_create(&data->m_tid, NULL, wnd_kbd_thread, data))
	{
		free(data);
		return NULL;
	}
	return data;
} /* End of 'wnd_kbd_init' function */

/* Initialize the escape sequences list */
void wnd_kbd_init_seq( wnd_kbd_data_t *data )
{
	int i;

	/* Initialize sequences */
	data->m_seq = NULL;
	data->m_last_seq = NULL;

	/* Add sequences */
	for ( i = 0; i <= 0xFF; i ++ )
	{
		char str[3] = {'\033', (char)i, 0};
		wnd_kbd_add_seq(data, str, WND_KEY_WITH_ALT(i));
	}
	wnd_kbd_add_seq(data, tgetstr("kr", NULL), KEY_RIGHT);
	wnd_kbd_add_seq(data, tgetstr("kl", NULL), KEY_LEFT);
	wnd_kbd_add_seq(data, tgetstr("ku", NULL), KEY_UP);
	wnd_kbd_add_seq(data, tgetstr("kd", NULL), KEY_DOWN);
	wnd_kbd_add_seq(data, tgetstr("kb", NULL), KEY_BACKSPACE);
	wnd_kbd_add_seq(data, tgetstr("kD", NULL), KEY_DC);
	wnd_kbd_add_seq(data, tgetstr("kh", NULL), KEY_HOME);
	wnd_kbd_add_seq(data, tgetstr("@7", NULL), KEY_END);
	wnd_kbd_add_seq(data, tgetstr("kI", NULL), KEY_IC);
	wnd_kbd_add_seq(data, tgetstr("kN", NULL), KEY_NPAGE);
	wnd_kbd_add_seq(data, tgetstr("kP", NULL), KEY_PPAGE);
	wnd_kbd_add_seq(data, tgetstr("k0", NULL), KEY_F0);
	wnd_kbd_add_seq(data, tgetstr("k1", NULL), KEY_F(1));
	wnd_kbd_add_seq(data, tgetstr("k2", NULL), KEY_F(2));
	wnd_kbd_add_seq(data, tgetstr("k3", NULL), KEY_F(3));
	wnd_kbd_add_seq(data, tgetstr("k4", NULL), KEY_F(4));
	wnd_kbd_add_seq(data, tgetstr("k5", NULL), KEY_F(5));
	wnd_kbd_add_seq(data, tgetstr("k6", NULL), KEY_F(6));
	wnd_kbd_add_seq(data, tgetstr("k7", NULL), KEY_F(7));
	wnd_kbd_add_seq(data, tgetstr("k8", NULL), KEY_F(8));
	wnd_kbd_add_seq(data, tgetstr("k9", NULL), KEY_F(9));
	wnd_kbd_add_seq(data, tgetstr("k;", NULL), KEY_F(10));
	wnd_kbd_add_seq(data, tgetstr("F1", NULL), KEY_F(11));
	wnd_kbd_add_seq(data, tgetstr("F2", NULL), KEY_F(12));
} /* End of 'wnd_kbd_init_seq' function */

/* Add a sequence to the list */
void wnd_kbd_add_seq( wnd_kbd_data_t *data, char *seq, int code )
{
	struct wnd_kbd_seq_t *item;

	/* Sequence is invalid */
	if (seq == NULL)
		return;

	/* Create the list item */
	item = (struct wnd_kbd_seq_t *)malloc(sizeof(*item));
	item->m_str = strdup(seq);
	item->m_code = code;
	item->m_next = NULL;

	/* Add it */
	if (data->m_seq == NULL)
		data->m_seq = item;
	else
		data->m_last_seq->m_next = item;
	data->m_last_seq = item;
} /* End of 'wnd_kbd_add_seq' function */

/* Uninitialize keyboard management system */
void wnd_kbd_free( wnd_kbd_data_t *data )
{
	struct wnd_kbd_seq_t *seq, *next;

	assert(data);

	/* Free memory */
	for ( seq = data->m_seq; seq != NULL; seq = next )
	{
		next = seq->m_next;
		free(seq->m_str);
		free(seq);
	}
	
	/* Stop keyboard thread */
	data->m_end_thread = TRUE;
	pthread_join(data->m_tid, NULL);
	free(data);
} /* End of 'wnd_kbd_free' function */

/* Keyboard thread function */
void *wnd_kbd_thread( void *arg )
{
	wnd_kbd_data_t *data = (wnd_kbd_data_t *)arg;
	wnd_t *wnd_root = data->m_wnd_root;
	char buf[32];
	int buf_ptr = 0;
	wnd_key_t keycode;
	struct timeval was_tv, now_tv;
	int was_btn;

	gettimeofday(&was_tv, NULL);
	for ( ; !data->m_end_thread; ) 
	{
		/* Extract keycode */
		if (wnd_kbd_extract_code(data, &keycode, buf, &buf_ptr))
		{
			/* Send message */
			wnd_t *focus = WND_FOCUS(wnd_root);
			if (focus != NULL)
			{
				wnd_msg_send(focus, "keydown", wnd_msg_key_new(keycode));
			}
		}

		/* Read key into buffer */
		int key = getch();
		if (key == ERR)
		{
			util_wait();
			continue;
		}
		/* Handle mouse events */
		else if (key == KEY_MOUSE)
		{
			if (WND_MOUSE_DATA(wnd_root)->m_driver == WND_MOUSE_XTERM)
			{
				int x, y;
				wnd_mouse_event_t type;
				wnd_mouse_button_t btn;

				/* Get event parameters */
				btn = getch() - 040;
				x = getch() - 040 - 1;
				y = getch() - 040 - 1;
				type = WND_MOUSE_DOWN;
				switch (btn)
				{
				case 0:
					btn = WND_MOUSE_LEFT;
					break;
				case 1:
					btn = WND_MOUSE_MIDDLE;
					break;
				case 2:
					btn = WND_MOUSE_RIGHT;
					break;
				}

				/* Check for double click */
				gettimeofday(&now_tv, NULL);
				if (((now_tv.tv_sec == was_tv.tv_sec && 
						now_tv.tv_usec - was_tv.tv_usec <= 200000) ||
						(now_tv.tv_sec == was_tv.tv_sec + 1 &&
						 now_tv.tv_usec + 1000000 - 
							 was_tv.tv_usec <= 200000)) && 
						btn == was_btn)
					type = WND_MOUSE_DOUBLE;
				memcpy(&was_tv, &now_tv, sizeof(was_tv));
				was_btn = btn;
				
				/* Handle mouse */
				wnd_mouse_handle_event(WND_MOUSE_DATA(wnd_root), 
						x, y, btn, type, NULL);
			}
		}
		else
		{
			buf[buf_ptr ++] = (char)key;
		}
	}
	return NULL;
} /* End of 'wnd_kbd_thread' function */

/* Extract real key code from the keys buffer */
bool_t wnd_kbd_extract_code( wnd_kbd_data_t *data, wnd_key_t *code, 
		char *buf, int *len )
{
	int pos, matched_pos = 0;

	/* Buffer is empty */
	if ((*len) == 0)
		return FALSE;

	/* Test increasing sequences from the buffer */
	for ( pos = 1; pos <= (*len); pos ++ )
	{
		int num;
		wnd_key_t ret;
		num = wnd_kbd_test_seq(data, buf, pos, &ret);

		/* No matches : return the last matched sequence */
		if (num == 0)
		{
			wnd_kbd_rem_from_buf(buf, matched_pos, len);
			return TRUE;
		}
		/* There are matches and one match is exact, so save it */
		else if (ret != WND_KEY_UNDEF)
		{
			(*code) = ret;
			matched_pos = pos;

			/* Return this match if it is the only match */
			if (num == 1)
			{
				wnd_kbd_rem_from_buf(buf, matched_pos, len);
				return TRUE;
			}
		}
	}
	
	/* Sequence is not complete, so wait for it */
	return FALSE;
} /* End of 'wnd_kbd_extract_code' function */

/* Test sequence for matches in the list */
int wnd_kbd_test_seq( wnd_kbd_data_t *data, char *seq, int len, 
		wnd_key_t *code )
{
	int matches = 0;
	struct wnd_kbd_seq_t *s;

	assert(len > 0);
	
	/* Test escape sequences */
	(*code) = WND_KEY_UNDEF;
	for ( s = data->m_seq; s != NULL; s = s->m_next )
	{
		int sl = strlen(s->m_str);
		if (!strncmp(s->m_str, seq, len))
		{
			matches ++;
			if (sl == len)
				(*code) = s->m_code;
		}
	}
	if (matches > 0)
		return matches;

	/* Test common keys */
	if ((*seq) != 27)
	{
		(*code) = (*seq);
		return 1;
	}
	return 0;
} /* End of 'wnd_kbd_test_seq' function */

/* Remove a sequence from the buffer */
void wnd_kbd_rem_from_buf( char *buf, int pos, int *len )
{
	memmove(buf, &buf[pos], (*len) - pos);
	(*len) -= pos;
} /* End of 'wnd_kbd_rem_from_buf' function */

/* End of 'wnd_kbd.c' file */

