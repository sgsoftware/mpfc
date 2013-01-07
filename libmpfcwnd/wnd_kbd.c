/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Keyboard functions implementation.
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

#include <assert.h>
#include <ncursesw/curses.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
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
	data->m_global = WND_GLOBAL(data->m_wnd_root);

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
	cfg_list_t *list = WND_ROOT_CFG(data->m_wnd_root);
	char *kb_val;

	/* Initialize sequences */
	data->m_seq = NULL;
	data->m_last_seq = NULL;

	/* Add sequences */
	for ( i = 0; i <= 0xFF; i ++ )
	{
		char str[3] = {'\033', (char)i, 0};
		wnd_kbd_add_seq(data, str, WND_KEY_WITH_ALT(i));
	}
	kb_val = wnd_kbd_ti_val(list, "kb");
	wnd_kbd_add_seq(data, kb_val, KEY_BACKSPACE);
	if (kb_val == NULL || !(kb_val[0] == '\x7F' && kb_val[1] == '\0'))
		wnd_kbd_add_seq(data, "\x7F", KEY_BACKSPACE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "K1"), KEY_A1);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "K3"), KEY_A3);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "K2"), KEY_B2);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@1"), KEY_BEG);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kB"), KEY_BTAB);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "K4"), KEY_C1);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "K5"), KEY_C3);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@2"), KEY_CANCEL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "ka"), KEY_CATAB);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kC"), KEY_CLEAR);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@3"), KEY_CLOSE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@4"), KEY_COMMAND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@5"), KEY_COPY);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@6"), KEY_CREATE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kt"), KEY_CTAB);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kD"), KEY_DC);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kL"), KEY_DL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kd"), KEY_DOWN);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kM"), KEY_EIC);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@7"), KEY_END);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@8"), KEY_ENTER);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kE"), KEY_EOL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kS"), KEY_EOS);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@9"), KEY_EXIT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k0"), KEY_F0);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k1"), KEY_F(1));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k;"), KEY_F(10));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F1"), KEY_F(11));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F2"), KEY_F(12));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F3"), KEY_F(13));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F4"), KEY_F(14));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F5"), KEY_F(15));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F6"), KEY_F(16));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F7"), KEY_F(17));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F8"), KEY_F(18));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "F9"), KEY_F(19));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k2"), KEY_F(2));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FA"), KEY_F(20));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FB"), KEY_F(21));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FC"), KEY_F(22));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FD"), KEY_F(23));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FE"), KEY_F(24));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FF"), KEY_F(25));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FG"), KEY_F(26));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FH"), KEY_F(27));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FI"), KEY_F(28));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FJ"), KEY_F(29));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k3"), KEY_F(3));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FK"), KEY_F(30));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FL"), KEY_F(31));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FM"), KEY_F(32));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FN"), KEY_F(33));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FO"), KEY_F(34));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FP"), KEY_F(35));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FQ"), KEY_F(36));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FR"), KEY_F(37));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FS"), KEY_F(38));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FT"), KEY_F(39));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k4"), KEY_F(4));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FU"), KEY_F(40));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FV"), KEY_F(41));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FW"), KEY_F(42));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FX"), KEY_F(43));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FY"), KEY_F(44));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "FZ"), KEY_F(45));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fa"), KEY_F(46));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fb"), KEY_F(47));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fc"), KEY_F(48));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fd"), KEY_F(49));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k5"), KEY_F(5));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fe"), KEY_F(50));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Ff"), KEY_F(51));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fg"), KEY_F(52));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fh"), KEY_F(53));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fi"), KEY_F(54));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fj"), KEY_F(55));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fk"), KEY_F(56));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fl"), KEY_F(57));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fm"), KEY_F(58));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fn"), KEY_F(59));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k6"), KEY_F(6));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fo"), KEY_F(60));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fp"), KEY_F(61));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fq"), KEY_F(62));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "Fr"), KEY_F(63));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k7"), KEY_F(7));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k8"), KEY_F(8));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "k9"), KEY_F(9));
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "@0"), KEY_FIND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%1"), KEY_HELP);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kh"), KEY_HOME);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kI"), KEY_IC);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kA"), KEY_IL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kl"), KEY_LEFT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kH"), KEY_LL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%2"), KEY_MARK);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%3"), KEY_MESSAGE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%4"), KEY_MOVE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%5"), KEY_NEXT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kN"), KEY_NPAGE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%6"), KEY_OPEN);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%7"), KEY_OPTIONS);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kP"), KEY_PPAGE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%8"), KEY_PREVIOUS);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%9"), KEY_PRINT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%0"), KEY_REDO);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&1"), KEY_REFERENCE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&2"), KEY_REFRESH);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&3"), KEY_REPLACE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&4"), KEY_RESTART);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&5"), KEY_RESUME);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kr"), KEY_RIGHT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&6"), KEY_SAVE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&9"), KEY_SBEG);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&0"), KEY_SCANCEL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*1"), KEY_SCOMMAND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*2"), KEY_SCOPY);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*3"), KEY_SCREATE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*4"), KEY_SDC);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*5"), KEY_SDL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*6"), KEY_SELECT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*7"), KEY_SEND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*8"), KEY_SEOL);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*9"), KEY_SEXIT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kF"), KEY_SF);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "*0"), KEY_SFIND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "#1"), KEY_SHELP);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "#2"), KEY_SHOME);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "#3"), KEY_SIC);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "#4"), KEY_SLEFT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%a"), KEY_SMESSAGE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%b"), KEY_SMOVE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%c"), KEY_SNEXT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%d"), KEY_SOPTIONS);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%e"), KEY_SPREVIOUS);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%f"), KEY_SPRINT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kR"), KEY_SR);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%g"), KEY_SREDO);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%h"), KEY_SREPLACE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%i"), KEY_SRIGHT);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "%j"), KEY_SRSUME);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "!1"), KEY_SSAVE);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "!2"), KEY_SSUSPEND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "kT"), KEY_STAB);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "!3"), KEY_SUNDO);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&7"), KEY_SUSPEND);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "&8"), KEY_UNDO);
	wnd_kbd_add_seq(data, wnd_kbd_ti_val(list, "ku"), KEY_UP);
	wnd_kbd_add_seq(data, /*wnd_kbd_ti_val(list, "kM")*/"\033[M", KEY_MOUSE);

	/* Some terminals (e.g. xterm) don't honour terminfo.
	 * Add some common escape sequences by hand */
	wnd_kbd_add_seq(data, "\033[A", KEY_UP);
	wnd_kbd_add_seq(data, "\033OA", KEY_UP);
	wnd_kbd_add_seq(data, "\033[B", KEY_DOWN);
	wnd_kbd_add_seq(data, "\033OB", KEY_DOWN);
	wnd_kbd_add_seq(data, "\033[C", KEY_RIGHT);
	wnd_kbd_add_seq(data, "\033OC", KEY_RIGHT);
	wnd_kbd_add_seq(data, "\033[D", KEY_LEFT);
	wnd_kbd_add_seq(data, "\033OD", KEY_LEFT);
	wnd_kbd_add_seq(data, "\033[H", KEY_HOME);
	wnd_kbd_add_seq(data, "\033OH", KEY_HOME);
	wnd_kbd_add_seq(data, "\033[F", KEY_END);
	wnd_kbd_add_seq(data, "\033OF", KEY_END);
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

	if (data == NULL)
		return;

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
	logger_debug(data->m_global->m_log, "keyboard thread terminated");
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
	int key;
	wnd_global_data_t *global = data->m_global;

	gettimeofday(&was_tv, NULL);
	for ( ; !data->m_end_thread; ) 
	{
		/* Extract keycode */
		if (wnd_kbd_extract_code(data, &keycode, buf, &buf_ptr))
		{
			/* Handle mouse events */
			if (keycode == KEY_MOUSE)
			{
				//if (WND_MOUSE_DATA(wnd_root)->m_driver == WND_MOUSE_XTERM)
				//{
					int x, y;
					wnd_mouse_event_t type;
					wnd_mouse_button_t btn;

					/* Get event parameters */
					pthread_mutex_lock(&global->m_curses_mutex);
					btn = getch() - 040;
					x = getch() - 040 - 1;
					y = getch() - 040 - 1;
					pthread_mutex_unlock(&global->m_curses_mutex);
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
					wnd_mouse_handle_event(global->m_mouse_data,
							x, y, btn, type, NULL);
					continue;
				//}
			}

			/* Send message */
			wnd_t *focus = global->m_focus;
			if (focus != NULL)
			{
				wnd_msg_send(focus, "keydown", wnd_msg_key_new(keycode));
			}
		}

		/* Read key into buffer */
		pthread_mutex_lock(&global->m_curses_mutex);
		key = getch();
		pthread_mutex_unlock(&global->m_curses_mutex);
		if (key == ERR)
		{
			util_wait();
			continue;
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
			{
				(*code) = s->m_code;
			}
		}
	}
	if (matches > 0)
		return matches;

	/* Test common keys */
	if ((*seq) != 27)
	{
		(*code) = (byte)(*seq);
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

/* Get terminfo entry value */
char *wnd_kbd_ti_val( cfg_node_t *list, char *name )
{
	char *var_name;

	/* Look up configuration for this entry first */
	var_name = util_strcat("ti.", name, NULL);
	if (var_name != NULL)
	{
		char *val = cfg_get_var(list, var_name);
		free(var_name);
		if (val != NULL)
			return val;
	}

	/* Look up terminfo */
	return tgetstr(name, NULL);
} /* End of 'wnd_kbd_ti_val' function */

/* End of 'wnd_kbd.c' file */

