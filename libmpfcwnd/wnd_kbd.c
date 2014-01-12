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
#include <curses.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
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

	/* Start thread */
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
	wnd_key_t keycode;
	struct timeval was_tv, now_tv;
	int was_btn;
	int key;
	wnd_global_data_t *global = data->m_global;

	gettimeofday(&was_tv, NULL);
	for ( ; !data->m_end_thread; ) 
	{
		pthread_mutex_lock(&global->m_curses_mutex);
		key = getch();
		pthread_mutex_unlock(&global->m_curses_mutex);
		if (key == ERR)
		{
			util_wait();
			continue;
		}

		keycode = key;

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

		if (keycode == 27)
		{
			pthread_mutex_lock(&global->m_curses_mutex);
			int n = getch();
			pthread_mutex_unlock(&global->m_curses_mutex);
			keycode = (n != ERR) ? WND_KEY_WITH_ALT(n) : KEY_ESCAPE;
		}

		/* Send message */
		wnd_t *focus = global->m_focus;
		if (focus != NULL)
		{
			wnd_msg_send(focus, "keydown", wnd_msg_key_new(keycode));
		}
	}
	return NULL;
} /* End of 'wnd_kbd_thread' function */

/* End of 'wnd_kbd.c' file */

