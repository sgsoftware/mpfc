/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : window.c
 * PURPOSE     : SG Konsamp. Window functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.07.2003
 * NOTE        : Module prefix 'wnd'.
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

#include <curses.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "types.h"
#include "dlgbox.h"
#include "editbox.h"
#include "error.h"
#include "window.h"

/* Check that window object is valid */
#define WND_ASSERT(wnd) \
	if ((wnd) == NULL || ((wnd)->m_wnd) == NULL) return
#define WND_ASSERT_RET(wnd, ret) \
	if ((wnd) == NULL || ((wnd)->m_wnd) == NULL) return (ret)

/* Root window */
wnd_t *wnd_root = NULL;

/* Current focus window */
wnd_t *wnd_focus = NULL;

/* Keyboard thread ID */
pthread_t wnd_kbd_tid;

/* Keyboard thread termination flag */
bool wnd_end_kbd_thread = FALSE;

/* Create a new root window */
wnd_t *wnd_new_root( void )
{
	/* Allocate memory for window object */
	wnd_root = (wnd_t *)malloc(sizeof(wnd_t));
	if (wnd_root == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}
	
	/* Initialize window fields */
	if (!wnd_init(wnd_root, NULL, 0, 0, 0, 0))
	{
		free(wnd_root);
		return NULL;
	}

	/* Set no-delay 'getch' mode */
	nodelay(wnd_root->m_wnd, TRUE);

	/* Initialize mouse */
	mousemask(ALL_MOUSE_EVENTS, NULL);

	/* Initialize keyboard thread */
	pthread_create(&wnd_kbd_tid, NULL, wnd_kbd_thread, NULL);

	/* Return */
	return wnd_root;
} /* End of 'wnd_new_root' function */

/* Create a new child window */
wnd_t *wnd_new_child( wnd_t *parent, int x, int y, int w, int h	)
{
	wnd_t *wnd;

	/* Allocate memory for window object */
	wnd = (wnd_t *)malloc(sizeof(wnd_t));
	if (wnd == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}
	
	/* Initialize window fields */
	if (!wnd_init(wnd, parent, x, y, w, h))
	{
		free(wnd);
		return NULL;
	}
	return wnd;
} /* End of 'wnd_new_child' function */

/* Initialize window fields */
bool wnd_init( wnd_t *wnd, wnd_t *parent, int x, int y, int w, int h )
{
	int sx, sy;
	int i;

	/* Get real window position (on screen) */
	if (parent != NULL)
	{
		sx = x + parent->m_x;
		sy = y + parent->m_y;
	}
	else
	{
		sx = x;
		sy = y;
	}

	/* Initialize NCURSES library */
	if (parent == NULL)
	{
		wnd->m_wnd = initscr();
		if (wnd->m_wnd == NULL)
			return FALSE;
		cbreak();
		noecho();
		keypad(wnd->m_wnd, TRUE);
		w = COLS;
		h = LINES;
	}
	/* Create NCURSES window */
	else
	{
		wnd->m_wnd = newwin(h, w, sy, sx);
		if (wnd->m_wnd == NULL)
		{
			error_set_code(ERROR_CURSES);
			return FALSE;
		}
		keypad(wnd->m_wnd, TRUE);
	}

	/* Fill window fields */
	wnd->m_x = x;
	wnd->m_y = y;
	wnd->m_width = w;
	wnd->m_height = h;
	wnd->m_parent = parent;
	wnd->m_child = NULL;
	wnd->m_next = NULL;
	wnd->m_flags = 0;
	if (wnd->m_parent != NULL)
	{
		if (wnd->m_parent->m_child == NULL)
			wnd->m_parent->m_child = wnd;
		else
		{
			wnd_t *child = wnd->m_parent->m_child;
			
			while (child->m_next != NULL)
				child = child->m_next;
			child->m_next = wnd;
		}
	}
	for ( i = 0; i < WND_MSG_NUMBER; i ++ )
		wnd->m_msg_handlers[i] = NULL;
	wnd->m_msg_handlers[WND_MSG_CLOSE] = wnd_handle_close;
	wnd->m_msg_handlers[WND_MSG_CHANGE_FOCUS] = wnd_handle_ch_focus;
	wnd->m_wnd_destroy = wnd_destroy_func;
	wnd->m_msg_queue_head = wnd->m_msg_queue_tail = NULL;

	/* Create mutex for message queue synchronization */
	pthread_mutex_init(&wnd->m_mutex, NULL);

	return TRUE;
} /* End of 'wnd_init' function */

/* Destroy window function */
void wnd_destroy_func( wnd_t *wnd )
{
	struct tag_msg_queue *t, *t1;

	WND_ASSERT(wnd);

	/* Destroy queue and mutex */
	pthread_mutex_lock(&wnd->m_mutex);
	t = wnd->m_msg_queue_head;
	while (t != NULL)
	{
		t1 = t->m_next;
		free(t);
		t = t1;
	}
	pthread_mutex_destroy(&wnd->m_mutex);

	/* Unitialize NCURSES library if we destroy root window */
	if (wnd->m_parent == NULL)
	{
		/* Kill keyboard thread */
		wnd_end_kbd_thread = TRUE;
		pthread_join(wnd_kbd_tid, NULL);
		
		endwin();
	}
	else
	{
		delwin(wnd->m_wnd);
	}
} /* End of 'wnd_destroy_func' function */

/* Destroy common window */
void wnd_destroy( void *obj )
{
	wnd_t *wnd = (wnd_t *)obj;
	wnd_t *child;
	
	WND_ASSERT(wnd);

	/* Remove window from children list */
	if (wnd->m_parent != NULL)
	{
		child = wnd->m_parent->m_child;
		
		if (child == wnd)
		{
			wnd->m_parent->m_child = wnd->m_next;
		}
		else
		{
			while (child != NULL && child->m_next != wnd)
				child = child->m_next;
			if (child != NULL)
			{
				child->m_next = wnd->m_next;
			}
		}
	}

	/* Destroy window children */
	child = wnd->m_child;
	while (child != NULL)
	{
		wnd_t *next = child->m_next;
		
		wnd_destroy(child);
		child = next;
	}

	/* Call window virtual destructor */
	wnd->m_wnd_destroy(wnd);

	/* Free memory */
	free(wnd);
} /* End of 'wnd_destroy' function */

/* Register message handler */
void wnd_register_handler( void *obj, dword msg_id, wnd_msg_handler handler )
{
	wnd_t *wnd = (wnd_t *)obj;
	
	WND_ASSERT(wnd);

	if (msg_id < WND_MSG_NUMBER)
		wnd->m_msg_handlers[msg_id] = handler;
} /* End of 'wnd_register_handler' function */

/* Run window message loop */
int wnd_run( void *obj )
{
	bool done = FALSE;
	wnd_t *wnd = (wnd_t *)obj;
	bool need_redraw = TRUE;

	WND_ASSERT_RET(wnd, 0);

	/* If it is dialog box - set focus to child */
	if (wnd->m_flags & WND_DIALOG && wnd->m_child != NULL)
		wnd_send_msg(wnd, WND_MSG_CHANGE_FOCUS, 0);

	/* Message loop */
	while (!done)
	{
		dword id, data;
		struct timespec tv;

		/* Set current focus window */
		wnd_focus = wnd;
		
		/* Display root window */
		if (need_redraw)
		{
			wnd_display(wnd_root);
			need_redraw = FALSE;
		}

		/* Get new messages from queue */
		if (wnd_get_msg(wnd, &id, &data))
		{
			bool end;

			do
			{
				/* Handle message */
				if (wnd->m_msg_handlers[id] != NULL)
					if (id != WND_MSG_DISPLAY)
						(wnd->m_msg_handlers[id])(wnd, data);

				/* Close window */
				if ((id == WND_MSG_CLOSE) || ((id == WND_MSG_CHANGE_FOCUS) &&
							(wnd->m_parent != NULL) && 
							(wnd->m_parent->m_flags & WND_DIALOG)))
				{
					done = TRUE;
					break;
				}

				/* Extract next message */
				end = !wnd_get_msg(wnd, &id, &data);
			} while (!end);

			/* We need to redraw now */
			need_redraw = TRUE;
		}

		/* Wait a little */
		tv.tv_sec = 0;
		tv.tv_nsec = 100000L;
		nanosleep(&tv, NULL);
	}

	wnd_focus = NULL;
} /* End of 'wnd_run' function */

/* Send message to window */
void wnd_send_msg( void *obj, dword id, dword data )
{
	wnd_t *wnd = (wnd_t *)obj;
	struct tag_msg_queue *msg;

	WND_ASSERT(wnd);
	
	/* Display window immediately */
	if (id == WND_MSG_DISPLAY)
	{
		wnd_display(wnd_root);
		return;
	}

	/* Get access to queue */
	pthread_mutex_lock(&wnd->m_mutex);

	/* Add message to queue */
	if (wnd->m_msg_queue_tail == NULL)
		msg = wnd->m_msg_queue_head = wnd->m_msg_queue_tail = 
			(struct tag_msg_queue *)malloc(sizeof(struct tag_msg_queue));
	else
		msg = wnd->m_msg_queue_tail->m_next = 
			(struct tag_msg_queue *)malloc(sizeof(struct tag_msg_queue));
	msg->m_id = id;
	msg->m_data = data;
	msg->m_next = NULL;

	/* Release queue */
	pthread_mutex_unlock(&wnd->m_mutex);
} /* End of 'wnd_send_msg' function */

/* Get message from queue */
bool wnd_get_msg( void *obj, dword *id, dword *data )
{
	wnd_t *wnd = (wnd_t *)obj;
	bool ret;

	WND_ASSERT_RET(wnd, FALSE);

	/* Get access to queue */
	pthread_mutex_lock(&wnd->m_mutex);

	/* Check that queue is not empty */
	if (wnd->m_msg_queue_head == NULL)
		ret = FALSE;
	/* Extract message */
	else
	{
		*id = wnd->m_msg_queue_head->m_id;
		*data = wnd->m_msg_queue_head->m_data;
		wnd->m_msg_queue_head = wnd->m_msg_queue_head->m_next;
		if (wnd->m_msg_queue_head == NULL)
			wnd->m_msg_queue_tail = NULL;
		ret = TRUE;
	}

	/* Release queue */
	pthread_mutex_unlock(&wnd->m_mutex);
	return ret;
} /* End of 'wnd_get_msg' function */

/* Move cursor to a specified location */
void wnd_move( wnd_t *wnd, int x, int y )
{
	WND_ASSERT(wnd);

	/* Move cursor */
	wmove(wnd->m_wnd, y, x);
} /* End of 'wnd_move' function */

/* Get cursor X coordinate */
int wnd_getx( wnd_t *wnd )
{
	int x, y;
	
	WND_ASSERT_RET(wnd, 0);
	
	getyx(wnd->m_wnd, y, x);
	return x;
} /* End of 'wnd_getx' function */

/* Get cursor Y coordinate */
int wnd_gety( wnd_t *wnd )
{
	int x, y;
	
	WND_ASSERT_RET(wnd, 0);
	
	getyx(wnd->m_wnd, y, x);
	return y;
} /* End of 'wnd_gety' function */

/* Set print attributes */
void wnd_set_attrib( wnd_t *wnd, int attr )
{
	WND_ASSERT(wnd);

	/* Set attribute */
	wattrset(wnd->m_wnd, attr);
} /* End of 'wnd_set_attrib' function */

/* Print a formatted string */
void wnd_printf( wnd_t *wnd, char *format, ... )
{
	va_list ap;
	
	WND_ASSERT(wnd);

	/* Print string */
	va_start(ap, format);
	vwprintw(wnd->m_wnd, format, ap);
	va_end(ap);
} /* End of 'wnd_printf' function */

/* Print one character */
void wnd_print_char( wnd_t *wnd, int ch )
{
	WND_ASSERT(wnd);

	/* Print character */
	waddch(wnd->m_wnd, ch);
} /* End of 'wnd_print_char' function */

/* System display window */
void wnd_display( wnd_t *wnd )
{
	wnd_t *child, *focus_child;
	wnd_msg_handler display = wnd->m_msg_handlers[WND_MSG_DISPLAY];

	/* Call window-specific display function */
	if (display != NULL)
	{
		display(wnd, 0);
		wnoutrefresh(wnd->m_wnd);
	}

	/* Determine child branch that contains focus window (if any) */
	focus_child = wnd_find_focus_branch(wnd);

	/* Display window children */
	child = wnd->m_child;
	while (child != NULL)
	{
		if (child != focus_child)
			wnd_display(child);
		child = child->m_next;
	}
	if (focus_child != NULL)
		wnd_display(focus_child);

	/* Refresh screen in case of root window */
	if (wnd->m_parent == NULL)
		doupdate();
} /* End of 'wnd_display' function */

/* Find window child that contains focus window */
wnd_t *wnd_find_focus_branch( wnd_t *wnd )
{
	wnd_t *child;
	
	WND_ASSERT_RET(wnd, NULL);

	child = wnd->m_child;
	while (child != NULL)
	{
		if (child == wnd_focus || wnd_find_focus_branch(child) != NULL)
			return child;
		child = child->m_next;
	}
	return NULL;
} /* End of 'wnd_find_focus_branch' function */

/* Keyboard thread function */
void *wnd_kbd_thread( void *arg )
{
	for ( ; !wnd_end_kbd_thread; )
	{
		int key;
		struct timespec tv;

		/* Read next character */
		if ((key = getch()) != ERR)
		{
			/* Check that it was mouse event */
			if (key == KEY_MOUSE)
			{
				MEVENT me;

				getmouse(&me);
				if (me.bstate & BUTTON1_CLICKED)
				{
					wnd_send_msg(wnd_focus, WND_MSG_MOUSE_LEFT_CLICK,
							(((dword)(word)me.x) << 16) | (word)me.y);
				}
			}
			/* Send message about this key to current focus window */
			else
			{
				if (key == KEY_REFRESH)
				{
					util_log("Here\n");
					wnd_redisplay(wnd_root);
				}
				wnd_send_msg(wnd_focus, WND_MSG_KEYDOWN, (dword)key);
			}
		}

		/* Wait a little */
		tv.tv_sec = 0;
		tv.tv_nsec = 100000L;
		nanosleep(&tv, NULL);
	}
	return NULL;
} /* End of 'wnd_kbd_thread' function */

/* Generic WND_MSG_CLOSE message handler */
void wnd_handle_close( wnd_t *wnd, dword data )
{
	WND_ASSERT(wnd);

	/* Close parent window if it is dialog box */
	if (wnd->m_parent != NULL && 
			(wnd->m_parent->m_flags & WND_DIALOG))
		wnd_send_msg(wnd->m_parent, WND_MSG_CLOSE, 0);
} /* End of 'wnd_handle_close' function */

/* Generic WND_MSG_CHANGE_FOCUS message handler */
void wnd_handle_ch_focus( wnd_t *wnd, dword data )
{
	/* If we are dialog - run next window */
	if (wnd->m_flags & WND_DIALOG)
	{
		dlgbox_t *dlg = (dlgbox_t *)wnd;
		
		if (wnd->m_child != NULL)
		{
			if (dlg->m_cur_focus == NULL)
				dlg->m_cur_focus = wnd->m_child;
			else
				dlg->m_cur_focus = 
					((dlg->m_cur_focus->m_next != NULL) ? 
					 	dlg->m_cur_focus->m_next : wnd->m_child);
			wnd_run(dlg->m_cur_focus);
		}
	}
	/* If we are item - close ourselves and inform parent to change focus */
	else if ((wnd->m_parent != NULL) && (wnd->m_parent->m_flags & WND_DIALOG))
	{
		wnd_send_msg(wnd->m_parent, WND_MSG_CHANGE_FOCUS, 0);
	}
} /* End of 'wnd_handle_ch_focus' function */

/* Clear the window */
void wnd_clear( wnd_t *wnd, bool start_from_cursor )
{
	int i;

	if (!start_from_cursor)
		wnd_move(wnd, 0, 0);
	for ( i = (start_from_cursor ? wnd_gety(wnd) : 0); i < wnd->m_height; i ++ )
		wnd_printf(wnd, "\n");
} /* End of 'wnd_clear' function */

/* Totally redisplay window */
void wnd_redisplay( wnd_t *wnd )
{
	wnd_clear(wnd, FALSE);
	wrefresh(wnd->m_wnd);
	wnd_display(wnd);
/*	touchwin(wnd->m_wnd);
	refresh();*/
} /* End of 'wnd_redisplay' function */

/* End of 'window.c' file */

