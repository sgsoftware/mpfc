/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : window.h
 * PURPOSE     : SG MPFC. Interface for window functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 30.01.2004
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

#ifndef __SG_MPFC_WINDOW_H__
#define __SG_MPFC_WINDOW_H__

#include <curses.h>
#include <gpm.h>
#include <pthread.h>
#include "types.h"

/* Declare window type at first */
struct tag_wnd_t;

/* Window messages */
#define WND_MSG_DISPLAY					0
#define WND_MSG_KEYDOWN					1
#define WND_MSG_USER					2
#define WND_MSG_CLOSE					3
#define WND_MSG_CHANGE_FOCUS			4
#define WND_MSG_NOTIFY					5
#define WND_MSG_POSTPONED_NOTIFY		6
#define WND_MSG_MOUSE_LEFT_CLICK		7
#define WND_MSG_MOUSE_LEFT_DOUBLE		8
#define WND_MSG_MOUSE_MIDDLE_CLICK		9
#define WND_MSG_MOUSE_RIGHT_CLICK		10
#define WND_MSG_MOUSE_OUTSIDE_FOCUS		11
#define WND_MSG_NUMBER					12

/* Check if specified message is a mouse message */
#define WND_IS_MOUSE_MSG(id) ((id) >= WND_MSG_MOUSE_LEFT_CLICK && \
								(id) < WND_MSG_NUMBER)

/* Window flags */
#define WND_DIALOG						0x00000001
#define WND_ITEM						0x00000002
#define WND_NO_FOCUS					0x00000004
#define WND_INITIALIZED					0x00000008

/* Get macros */
#define WND_OBJ(wnd)		((wnd_t *)(wnd))
#define WND_FLAGS(wnd)		(((wnd_t *)(wnd))->m_flags)
#define WND_X(wnd)			(((wnd_t *)(wnd))->m_x)
#define WND_Y(wnd)			(((wnd_t *)(wnd))->m_y)
#define WND_WIDTH(wnd)		(((wnd_t *)(wnd))->m_width)
#define WND_HEIGHT(wnd)		(((wnd_t *)(wnd))->m_height)

/* Create data for notify message */
#define WND_NOTIFY_DATA(id, act) (((id) << 16) | (act))
#define WND_NOTIFY_ID(data) ((data) >> 16)
#define WND_NOTIFY_ACT(data) (short)(data)

/* Create and get data for mouse messages */
#define WND_MOUSE_DATA(x, y) (((x) << 16) | (y))
#define WND_MOUSE_X(data) ((data) >> 16)
#define WND_MOUSE_Y(data) (short)(data)

/* Window message handler function */
typedef void (*wnd_msg_handler)( struct tag_wnd_t *wnd, dword data );

/* Window type */
typedef struct tag_wnd_t
{
	/* NCURSES window handle */
	WINDOW *m_wnd;

	/* Window position and size */
	int m_x, m_y, m_width, m_height;

	/* Window screen position */
	int m_sx, m_sy;

	/* Window ID */
	short m_id;

	/* Window flags */
	dword m_flags;

	/* Window parent */
	struct tag_wnd_t *m_parent;

	/* Window first child and next sibling */
	struct tag_wnd_t *m_child, *m_next;

	/* Message handlers */
	wnd_msg_handler m_msg_handlers[WND_MSG_NUMBER];

	/* Virtual destructor */
	void (*m_wnd_destroy)( struct tag_wnd_t *wnd );

	/* Window messages queue */
	struct tag_msg_queue
	{
		dword m_id;
		dword m_data;
		struct tag_msg_queue *m_next;
	} *m_msg_queue_head, *m_msg_queue_tail;

	/* Mutex for queue access synchronization */
	pthread_mutex_t m_mutex;

#if 0
	/* Window contents after last redraw */
	chtype **m_save_contents;
#endif
} wnd_t;

/* Data type for 'WND_MSG_POSTPONED_NOTIFY' message */
typedef struct 
{
	wnd_t *m_whom;
	dword m_data;
} wnd_postponed_notify_data_t;

/* Root window */
extern wnd_t *wnd_root;

/* Current focus window */
extern wnd_t *wnd_focus;

/* Create a new root window */
wnd_t *wnd_new_root( void );

/* Create a new child window */
wnd_t *wnd_new_child( wnd_t *parent, int x, int y, int w, int h );

/* Initialize window fields */
bool_t wnd_init( wnd_t *wnd, wnd_t *parent, int x, int y, int w, int h );

/* Destroy window */
void wnd_destroy_func( wnd_t *wnd );

/* Destroy common window */
void wnd_destroy( void *wnd );

/* Register message handler */
void wnd_register_handler( void *wnd, dword msg_id, wnd_msg_handler handler );

/* Run window message loop */
int wnd_run( void *wnd );

/* Send message to window */
void wnd_send_msg( void *wnd, dword id, dword data );

/* Get message from queue */
bool_t wnd_get_msg( void *wnd, dword *id, dword *data );

/* Move cursor to a specified location */
void wnd_move( wnd_t *wnd, int x, int y );

/* Get cursor X coordinate */
int wnd_getx( wnd_t *wnd );

/* Get cursor Y coordinate */
int wnd_gety( wnd_t *wnd );

/* Set print attributes */
void wnd_set_attrib( wnd_t *wnd, int attr );

/* Print a formatted string */
void wnd_printf( wnd_t *wnd, char *format, ... );

/* Print a formatted string with bounds */
void wnd_printf_bound( wnd_t *wnd, int len, char *format, ... );

/* Print one character */
void wnd_print_char( wnd_t *wnd, int ch );

/* System display window */
void wnd_display( wnd_t *wnd );

/* Find window child that contains focus window */
wnd_t *wnd_find_focus_branch( wnd_t *wnd );

/* Keyboard thread function */
void *wnd_kbd_thread( void *arg );

/* Mouse thread function */
void *wnd_mouse_thread( void *arg );

/* Generic WND_MSG_CLOSE message handler */
void wnd_handle_close( wnd_t *wnd, dword data );

/* Generic WND_MSG_CHANGE_FOCUS message handler */
void wnd_handle_ch_focus( wnd_t *wnd, dword data );

/* Generic WND_MSG_POSTPONED_NOTIFY message handler */
void wnd_handle_pp_notify( wnd_t *wnd, dword data );

/* Clear the window */
void wnd_clear( wnd_t *wnd, bool_t start_from_cursor );

/* Totally redisplay window */
void wnd_redisplay( wnd_t *wnd );

/* Initialize color pair */
int wnd_init_pair( int fg, int bg );

/* Check that window is focused */
bool_t wnd_is_focused( void *wnd );

/* Temporarily exit curses */
void wnd_close_curses( void );

/* Restore curses */
void wnd_restore_curses( void );

/* Find a child by its ID */
wnd_t *wnd_find_child_by_id( wnd_t *parent, short id );

/* Gpm mouse handler */
int wnd_mouse_handler( Gpm_Event *event, void *data );

/* Get window under which mouse cursor is */
wnd_t *wnd_get_wnd_under_cursor( int x, int y );

/* Check if a point belongs to the window */
bool_t wnd_pt_belongs( wnd_t *wnd, int x, int y );

/* Reinitialize mouse */
void wnd_reinit_mouse( void );

/* Untouch lines touched by ncurses and touch lines that have been
 * really changed */
void wnd_touch( wnd_t *wnd );

#endif

/* End of 'window.h' file */

