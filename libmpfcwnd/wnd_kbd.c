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
#include "types.h"
#include "wnd.h"
#include "wnd_kbd.h"
#include "wnd_msg.h"
#include "util.h"

/* Initialize keyboard management system */
wnd_kbd_data_t *wnd_kbd_init( wnd_t *wnd_root )
{
	/* Start keyboard thread */
	wnd_kbd_data_t *data = (wnd_kbd_data_t *)malloc(sizeof(wnd_kbd_data_t));
	data->m_end_thread = FALSE;
	data->m_wnd_root = wnd_root;
	if (pthread_create(&data->m_tid, NULL, wnd_kbd_thread, data))
	{
		free(data);
		return NULL;
	}
	return data;
} /* End of 'wnd_kbd_init' function */

/* Uninitialize keyboard management system */
void wnd_kbd_free( wnd_kbd_data_t *data )
{
	assert(data);
	
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
	int buf[10];
	int buf_ptr = 0;
	wnd_key_t keycode;
	struct timeval was_tv, now_tv;
	int was_btn;

	gettimeofday(&was_tv, NULL);
	for ( ; !data->m_end_thread; ) 
	{
		/* Extract keycode */
		if (wnd_kbd_extract_code(&keycode, buf, &buf_ptr))
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
			buf[buf_ptr ++] = key;
		}
	}
	return NULL;
} /* End of 'wnd_kbd_thread' function */

/* Extract real key code from the keys buffer */
bool_t wnd_kbd_extract_code( wnd_key_t *code, int *buf, int *len )
{
	if (*len == 0)
		return FALSE;

	/* Escape sequence */
	if (buf[0] == KEY_ESCAPE)
	{
		if (*len == 1)
			return FALSE;
		if (buf[1] != KEY_ESCAPE)
			(*code) = WND_KEY_WITH_ALT(buf[1]);
		else
			(*code) = KEY_ESCAPE;
		memmove(buf, &buf[2], (*len) - 2);
		(*len) -= 2;
	}
	/* Normal key */
	else
	{
		(*code) = buf[0];
		memmove(buf, &buf[1], (*len) - 1);
		(*len) --;
	}
	return TRUE;
} /* End of 'wnd_kbd_extract_code' function */

/* End of 'wnd_kbd.c' file */

