/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : window.c
 * PURPOSE     : SG MPFC. Window functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 1.01.2004
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
#include <sys/time.h>
#include "types.h"
#include "cfg.h"
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

/* Thread IDs */
pthread_t wnd_kbd_tid = -1, wnd_mouse_tid = -1;

/* Threads termination flag */
bool_t wnd_end_kbd_thread = FALSE;
bool_t wnd_end_mouse_thread = FALSE;

/* Number of initialized color pairs */
int wnd_num_pairs = 0;

/* Whether we have temporarily exited curses */
bool_t wnd_curses_closed = FALSE;

/* Mutex for synchronization displaying */
pthread_mutex_t wnd_display_mutex;

/* Mouse driver type */
#define WND_MOUSE_NONE  0
#define WND_MOUSE_GPM   1
#define WND_MOUSE_XTERM 2
int wnd_mouse_type = WND_MOUSE_NONE;

/* Create a new root window */
wnd_t *wnd_new_root( void )
{
	Gpm_Connect conn;

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

	/* Get mouse driver type */
	wnd_get_mouse_type();

	/* Initialize mouse */
	switch (wnd_mouse_type)
	{
	case WND_MOUSE_GPM:
		memset(&conn, 0, sizeof(conn));
		conn.eventMask = GPM_DOWN | GPM_DOUBLE | GPM_DRAG | GPM_UP;
		conn.defaultMask = ~GPM_HARD;
		conn.minMod = 0;
		conn.maxMod = 0;
		if (Gpm_Open(&conn, 0) == -1)
		{
			wnd_mouse_type = WND_MOUSE_NONE;
			break;
		}
		gpm_zerobased = TRUE;
		pthread_create(&wnd_mouse_tid, NULL, wnd_mouse_thread, NULL);
		break;
	case WND_MOUSE_XTERM:
		printf("\033[?9h");
		break;
	}

	/* Initialize keyboard thread */
	pthread_create(&wnd_kbd_tid, NULL, wnd_kbd_thread, NULL);
	wnd_root->m_flags |= WND_INITIALIZED;

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
bool_t wnd_init( wnd_t *wnd, wnd_t *parent, int x, int y, int w, int h )
{
	int i;

	/* Get real window position (on screen) */
	memset(wnd, 0, sizeof(wnd_t));
	if (parent != NULL)
	{
		wnd->m_sx = x + parent->m_sx;
		wnd->m_sy = y + parent->m_sy;
	}
	else
	{
		wnd->m_sx = x;
		wnd->m_sy = y;
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
		start_color();

		/* Create display mutex */
		pthread_mutex_init(&wnd_display_mutex, NULL);
	}
	/* Create NCURSES window */
	else
	{
		wnd->m_wnd = newwin(h, w, wnd->m_sy, wnd->m_sx);
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
	wnd->m_parent = parent;
	wnd->m_child = NULL;
	wnd->m_next = NULL;
	wnd->m_flags = 0;
	wnd->m_id = -1;
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
	wnd->m_msg_handlers[WND_MSG_POSTPONED_NOTIFY] = wnd_handle_pp_notify;
	wnd->m_wnd_destroy = wnd_destroy_func;
	wnd->m_msg_queue_head = wnd->m_msg_queue_tail = NULL;

#if 0
	/* Initialize window contents */
	wnd->m_save_contents = (chtype **)malloc(sizeof(chtype *) * wnd->m_height);
	for ( i = 0; i < wnd->m_height; i ++ ) 
	{
		wnd->m_save_contents[i] = (chtype *)malloc(sizeof(chtype) * 
				wnd->m_width);
		memset(wnd->m_save_contents[i], 0, wnd->m_width * sizeof(chtype));
	}
#endif

	/* Create mutex for message queue synchronization */
	pthread_mutex_init(&wnd->m_mutex, NULL);

	return TRUE;
} /* End of 'wnd_init' function */

/* Destroy window function */
void wnd_destroy_func( wnd_t *wnd )
{
	struct tag_msg_queue *t, *t1;
	int i;

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

#if 0
	/* Free memory */
	if (wnd->m_save_contents != NULL)
	{
		for ( i = 0; i < wnd->m_height; i ++ )
			free(wnd->m_save_contents[i]);
		free(wnd->m_save_contents);
		wnd->m_save_contents = NULL;
	}
#endif

	/* Unitialize NCURSES library if we destroy root window */
	if (wnd->m_parent == NULL)
	{
		/* Kill threads */
		wnd_end_kbd_thread = TRUE;
		wnd_end_mouse_thread = TRUE;
		if (wnd_kbd_tid >= 0)
		{
			pthread_join(wnd_kbd_tid, NULL);
			wnd_kbd_tid = -1;
		}
		if (wnd_mouse_tid >= 0)
		{
			pthread_join(wnd_mouse_tid, NULL);
			wnd_mouse_tid = -1;
		}

		/* Uninitialize mouse */
		if (wnd_mouse_type == WND_MOUSE_GPM)
			Gpm_Close();
		
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
	bool_t done = FALSE;
	wnd_t *wnd = (wnd_t *)obj;
	bool_t need_redraw = TRUE;

	WND_ASSERT_RET(wnd, 0);

	/* If it is dialog box - set focus to child */
	if ((wnd->m_flags & WND_DIALOG) && (wnd->m_child != NULL))
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
			bool_t end;

			do
			{
				/* Handle message */
				if (wnd->m_msg_handlers[id] != NULL)
					if (id != WND_MSG_DISPLAY)
						(wnd->m_msg_handlers[id])(wnd, data);

				/* Close window */
				if ((id == WND_MSG_CLOSE) || ((id == WND_MSG_CHANGE_FOCUS) &&
							(wnd->m_parent != NULL) && 
							(wnd->m_flags & WND_ITEM) &&
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

	/* Execute some messages */
	if (id == WND_MSG_NOTIFY || WND_IS_MOUSE_MSG(id))
	{
		if (wnd->m_msg_handlers[id] != NULL)
			(wnd->m_msg_handlers[id])(wnd, data);
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
bool_t wnd_get_msg( void *obj, dword *id, dword *data )
{
	wnd_t *wnd = (wnd_t *)obj;
	bool_t ret;

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
	int i;
	bool_t is_dialog;

	/* Don't display window if it is not initialized yet */
	if (!(wnd->m_flags & WND_INITIALIZED))
		return;

	/* Lock mutex for displaying */
	if (wnd == wnd_root)
		pthread_mutex_lock(&wnd_display_mutex);

	/* Call window-specific display function */
	if (display != NULL)
	{
		display(wnd, 0);
		wnoutrefresh(wnd->m_wnd);
	}

	/* Determine child branch that contains focus window (if any) */
	focus_child = wnd_find_focus_branch(wnd);

	/* Display window children */
	is_dialog = wnd->m_flags & WND_DIALOG;
	child = wnd->m_child;
	if (is_dialog)
		wnd_move(wnd, 1, 1);
	while (child != NULL)
	{
//		if (child != focus_child)
		if (is_dialog)
			wnd_advance(wnd, child->m_sx - wnd->m_sx, child->m_sy - wnd->m_sy);
		wnd_display(child);
		child = child->m_next;
	}
	if (is_dialog)
		wnd_advance(wnd, WND_WIDTH(wnd) - 2, WND_HEIGHT(wnd) - 2);
	if (focus_child != NULL)
		wnd_display(focus_child);

	/* Refresh screen in case of root window */
	if (wnd->m_parent == NULL && !wnd_curses_closed)
		doupdate();

	/* Unlock mutex for displaying */
	if (wnd == wnd_root)
		pthread_mutex_unlock(&wnd_display_mutex);
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
	struct timeval was_tv, now_tv;
	int was_btn;
	int was_width = COLS, was_height = LINES;

	gettimeofday(&was_tv, NULL);
	for ( ; !wnd_end_kbd_thread; )
	{
		int key;
		struct timespec tv;
		struct winsize winsz;

		/* Check whether terminal size was changed */
		winsz.ws_col = winsz.ws_row = 0;
		ioctl(0, TIOCGWINSZ, &winsz);
		if (winsz.ws_col != was_width || winsz.ws_row != was_height)
		{
			was_width = winsz.ws_col;
			was_height = winsz.ws_row;
			wnd_redisplay(wnd_root);
		}

		/* Read next character */
		if ((key = getch()) != ERR)
		{
			if (key == '\f')
				wnd_redisplay(wnd_root);
			else if (key == KEY_MOUSE && wnd_mouse_type == WND_MOUSE_XTERM)
			{
				Gpm_Event event;
				int btn, x, y;

				/* Get event parameters */
				btn = getch() - 040;
				x = getch() - 040 - 1;
				y = getch() - 040 - 1;
				
				memset(&event, 0, sizeof(event));
				event.type = GPM_DOWN;
				switch (btn)
				{
				case 0:
					event.buttons = GPM_B_LEFT;
					break;
				case 1:
					event.buttons = GPM_B_MIDDLE;
					break;
				case 2:
					event.buttons = GPM_B_RIGHT;
					break;
				}
				event.x = x;
				event.y = y;

				/* Check for double click */
				gettimeofday(&now_tv, NULL);
				if (((now_tv.tv_sec == was_tv.tv_sec && 
						now_tv.tv_usec - was_tv.tv_usec <= 200000) ||
						(now_tv.tv_sec == was_tv.tv_sec + 1 &&
						 now_tv.tv_usec + 1000000 - 
							 was_tv.tv_usec <= 200000)) && 
						btn == was_btn)
					event.type = GPM_DOUBLE;
				memcpy(&was_tv, &now_tv, sizeof(was_tv));
				was_btn = btn;
				
				wnd_mouse_handler(&event, NULL);
			}
/*			else if (key == 14)
				wnd_reinit_mouse();*/
			else 
				wnd_send_msg(wnd_focus, WND_MSG_KEYDOWN, (dword)key);
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
	if ((wnd->m_flags & WND_ITEM) && wnd->m_parent != NULL && 
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
		wnd_t *data_wnd = (wnd_t *)data;
		
		/* Search for next focusable dialog item */
		if (data_wnd == NULL || (WND_FLAGS(data_wnd) & WND_NO_FOCUS))
		{
			wnd_t *was = dlg->m_cur_focus;
			do
			{
				dlg_next_focus(dlg);
				if (dlg->m_cur_focus == was)
					break;
			} while ((dlg->m_cur_focus == NULL) ||
					(dlg->m_cur_focus->m_flags & WND_NO_FOCUS));
		}
		else
			dlg->m_cur_focus = data_wnd;
		if (dlg->m_cur_focus != NULL)
		{
			if (dlg->m_cur_focus->m_flags & WND_NO_FOCUS)
				dlg->m_cur_focus = NULL;
			else
				wnd_run(dlg->m_cur_focus);
		}
	}
	/* If we are item - close ourselves and inform parent to change focus */
	else if ((wnd->m_parent != NULL) && (wnd->m_flags & WND_ITEM) &&
			(wnd->m_parent->m_flags & WND_DIALOG))
	{
		wnd_send_msg(wnd->m_parent, WND_MSG_CHANGE_FOCUS, data);
	}
} /* End of 'wnd_handle_ch_focus' function */

/* Clear the window */
void wnd_clear( wnd_t *wnd, bool_t start_from_cursor )
{
	int i;

	if (!start_from_cursor)
		wnd_move(wnd, 0, 0);
	for ( i = (start_from_cursor ? wnd_gety(wnd) : 0); i < WND_HEIGHT(wnd); i ++ )
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

/* Initialize color pair */
int wnd_init_pair( int fg, int bg )
{
	int i;
	
	/* Search for existing color pair respecting to this color */
	for ( i = 1; i <= wnd_num_pairs; i ++ )
	{
		short f, b;

		pair_content(i, &f, &b);
		if (f == fg && b == bg)
			return i;
	}

	/* Pair not found - create new one */
	if (wnd_num_pairs < COLOR_PAIRS - 1)
	{
		init_pair(++wnd_num_pairs, fg, bg);
		return wnd_num_pairs;
	}
	return 0;
} /* End of 'wnd_init_pair' function */

/* Check that window is focused */
bool_t wnd_is_focused( void *wnd )
{
	return ((wnd_t *)wnd == wnd_focus);
} /* End of 'wnd_is_focused' function */

/* Temporarily exit curses */
void wnd_close_curses( void )
{
	wnd_curses_closed = TRUE;
	wnd_clear(wnd_root, FALSE);
	refresh();
	endwin();
} /* End of 'wnd_close_curses' function */

/* Restore curses */
void wnd_restore_curses( void )
{
	wnd_curses_closed = FALSE;
	refresh();
} /* End of 'wnd_restore_curses' function */

/* Find a child by its ID */
wnd_t *wnd_find_child_by_id( wnd_t *parent, short id )
{
	wnd_t *child;

	if (parent == NULL)
		return NULL;

	for ( child = parent->m_child; child != NULL; child = child->m_next )
		if (WND_OBJ(child)->m_id == id)
			return child;
	return NULL;
} /* End of 'wnd_find_child_by_id' function */

/* Gpm mouse handler */
int wnd_mouse_handler( Gpm_Event *event, void *data )
{
	wnd_t *wnd;
	int msg = -1, x, y;

	/* Determine window to which this event is addressed */
	x = event->x, y = event->y;
	wnd = wnd_get_wnd_under_cursor(x, y);
	if (wnd != NULL)
		x -= wnd->m_sx, y -= wnd->m_sy;

	/* Send respective message to it */
	if ((event->type & GPM_DOUBLE) && (event->buttons & GPM_B_LEFT))
		msg = WND_MSG_MOUSE_LEFT_DOUBLE;
	if ((event->type & GPM_DOWN) && (event->buttons & GPM_B_LEFT))
		msg = WND_MSG_MOUSE_LEFT_CLICK;
	if ((event->type & GPM_DOWN) && (event->buttons & GPM_B_MIDDLE))
		msg = WND_MSG_MOUSE_MIDDLE_CLICK;
	if ((event->type & GPM_DOWN) && (event->buttons & GPM_B_RIGHT))
		msg = WND_MSG_MOUSE_RIGHT_CLICK;
	if (msg >= 0)
		wnd_send_msg(wnd, msg, WND_MOUSE_DATA(x, y));
	if (wnd != wnd_focus && msg >= 0)
		wnd_send_msg(wnd_focus, WND_MSG_MOUSE_OUTSIDE_FOCUS, 0);

	/* Display cursor */
	if (wnd_mouse_type == WND_MOUSE_GPM)
		GPM_DRAWPOINTER(event);
} /* End of 'wnd_mouse_handler' function */

/* Get window under which mouse cursor is */
wnd_t *wnd_get_wnd_under_cursor( int x, int y )
{
	wnd_t *wnd, *w;

	for ( wnd = wnd_root; wnd != NULL; )
	{
		wnd_t *fw, *last_suitable = NULL;
		
		/* Find a child under cursor */
		w = fw = wnd_find_focus_branch(wnd);
		if (w == NULL || !wnd_pt_belongs(w, x, y))
		{
			for ( w = wnd->m_child; w != NULL; w = w->m_next )
			{
				if (w == fw)
					continue;
				if (wnd_pt_belongs(w, x, y))
					last_suitable = w;
			}
			w = last_suitable;
		}

		/* If not found - return current window */
		if (w == NULL)
			return wnd;

		/* Else - continue in this window */
		wnd = w;
	}
	return wnd;
} /* End of 'wnd_get_wnd_under_cursor' function */

/* Mouse thread function */
void *wnd_mouse_thread( void *arg )
{
	fd_set readset;
	struct timeval tv;

	/* Loop */
	for ( ; !wnd_end_mouse_thread; )
	{
		Gpm_Event event;
		int ret;
		
		if (gpm_fd >= 0)
		{
			/* Initialize stuff for select */
			FD_ZERO(&readset);
			FD_SET(gpm_fd, &readset);
			memset(&tv, 0, sizeof(tv));
			
			/* Check for events */
			if ((ret = select(gpm_fd + 1, &readset, NULL, NULL, &tv)) > 0)
			{
				if (Gpm_GetEvent(&event) > 0)
					wnd_mouse_handler(&event, NULL);
			}
		}

		/* Wait a little */
		util_delay(0, 100000);
	}
	return NULL;
} /* End of 'wnd_mouse_thread' function */

/* Check if a point belongs to the window */
bool_t wnd_pt_belongs( wnd_t *wnd, int x, int y )
{
	return (wnd != NULL && x >= wnd->m_sx && x < wnd->m_sx + WND_WIDTH(wnd) &&
				y >= wnd->m_sy && y < wnd->m_sy + WND_HEIGHT(wnd));
} /* End of 'wnd_pt_belongs' function */

/* Generic WND_MSG_POSTPONED_NOTIFY message handler */
void wnd_handle_pp_notify( wnd_t *wnd, dword data )
{
	/* Free memory */
	free((void *)data);
} /* End of 'wnd_handle_pp_notify' function */

#if 0
/* Reinitialize mouse */
void wnd_reinit_mouse( void )
{
	Gpm_Connect conn;

	if (gpm_fd >= 0)
		Gpm_Close();

	memset(&conn, 0, sizeof(conn));
	conn.eventMask = GPM_DOWN | GPM_DOUBLE | GPM_DRAG | GPM_UP;
	conn.defaultMask = ~GPM_HARD;
	conn.minMod = 0;
	conn.maxMod = 0;
	if (Gpm_Open(&conn, 0) == -2)
	{
		wnd_xterm_mouse = TRUE;
		printf("\033[?9h");
	}
	gpm_zerobased = TRUE;
} /* End of 'wnd_reinit_mouse' function */
#endif

#if 0
/* Untouch lines touched by ncurses and touch lines that have been
 * really changed */
void wnd_touch( wnd_t *wnd )
{
	WINDOW *w;
	int i, j, x, y;
	
	if (wnd == NULL || wnd->m_save_contents == NULL || 
			(w = wnd->m_wnd) == NULL)
		return;

	/* Save cursor position */
	x = wnd_getx(wnd);
	y = wnd_gety(wnd);

	/* Clean curses information about touched lines */
	untouchwin(w);

	/* Compare lines */
	for ( i = 0; i < wnd->m_height; i ++ )
	{
		bool_t equal = TRUE;

		for ( j = 0; j < wnd->m_width; j ++ )
		{
			chtype ch = mvwinch(w, i, j);
			if (ch != wnd->m_save_contents[i][j])
			{
				equal = FALSE;
				wnd->m_save_contents[i][j] = ch;
			}
		}

		/* Touch line if it is changed */
		if (!equal)
			touchline(w, i, 1);
	}

	/* Restore cursor */
	wnd_move(wnd, x, y);
} /* End of 'wnd_touch' function */
#endif

/* Print a formatted string with bounds */
void wnd_printf_bound( wnd_t *wnd, int len, dword flags, char *format, ... )
{
	va_list ap;
	char *str;
	int n;
	
	WND_ASSERT(wnd);

	if (len < 0)
		return;

	/* Print string */
	str = (char *)malloc(len + 1);
	va_start(ap, format);
	n = vsnprintf(str, len + 1, format, ap);
	if (n > len && (flags & WND_PRINT_ELLIPSES))
	{
		strcpy(&str[len - 3], "...");
	}
	waddstr(wnd->m_wnd, str);
	va_end(ap);
	free(str);
	while (n < len && (flags & WND_PRINT_FILL_REST))
	{
		waddch(wnd->m_wnd, ' ');
		n ++;
	}
} /* End of 'wnd_printf_bound' function */

/* Advance cursor (move it with clearing space) */
void wnd_advance( wnd_t *wnd, int x, int y )
{
	int cur_x, cur_y;
	bool_t till_end = FALSE;

	if (wnd == NULL)
		return;

	/* Fix position */
	if (x < 0)
		x = 0;
	else if (x >= WND_WIDTH(wnd))
	{
		x = WND_WIDTH(wnd) - 1;
		till_end = TRUE;
	}
	if (y < 0)
		y = 0;
	else if (y >= WND_HEIGHT(wnd))
		y = WND_HEIGHT(wnd) - 1;

	/* If position is before current - simply move without clearing anything */
	cur_x = wnd_getx(wnd);
	cur_y = wnd_gety(wnd);
	if (y < cur_y || (y == cur_y && x < cur_x))
		wnd_move(wnd, x, y);

	/* Print spaces until reaching specified position */
	while (wnd_gety(wnd) < y)
		waddch(wnd->m_wnd, '\n');
	while (wnd_getx(wnd) < x)
		waddch(wnd->m_wnd, ' ');
	if (till_end)
		waddch(wnd->m_wnd, ' ');
} /* End of 'wnd_advance' function */

/* Get mouse driver type */
void wnd_get_mouse_type( void )
{
	char *driver, *term;
	
	/* Check variable that may force mouse type */
	driver = cfg_get_var(cfg_list, "mouse");
	if (driver != NULL)
	{
		if (!strcmp(driver, "xterm"))
			wnd_mouse_type = WND_MOUSE_XTERM;
		else if (!strcmp(driver, "gpm"))
			wnd_mouse_type = WND_MOUSE_GPM;
		else
			wnd_mouse_type = WND_MOUSE_NONE;
		return;
	}

	/* Check TERM variable */
	term = getenv("TERM");
	if (term == NULL || !strcmp(term, ""))
		wnd_mouse_type = WND_MOUSE_NONE;
	else if (!strcmp(term, "xterm") || !strcmp(term, "rxvt") || 
				!strcmp(term, "Eterm"))
		wnd_mouse_type = WND_MOUSE_XTERM;
	else
		wnd_mouse_type = WND_MOUSE_GPM;
} /* End of 'wnd_get_mouse_type' function */

/* Get window width */
int WND_WIDTH( void *wnd )
{
	int w, h;
	getmaxyx(WND_OBJ(wnd)->m_wnd, h, w);
	return w;
} /* End of 'WND_WIDTH' function */

/* Get window height */
int WND_HEIGHT( void *wnd )
{
	int w, h;
	getmaxyx(WND_OBJ(wnd)->m_wnd, h, w);
	return h;
} /* End of 'WND_HEIGHT' function */

/* End of 'window.c' file */

