/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_mouse.h
 * PURPOSE     : MPFC Window Library. Interface for mouse functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.08.2004
 * NOTE        : Module prefix 'wnd_mouse'.
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

#ifndef __MPFC_WND_MOUSE_H__
#define __MPFC_WND_MOUSE_H__

#include <pthread.h>
#include "types.h"

/* Further declaration */
struct tag_wnd_t;

/* Mouse driver types */
typedef enum
{
	WND_MOUSE_NONE,
	WND_MOUSE_GPM,
	WND_MOUSE_XTERM
} wnd_mouse_driver_t;

/* Mouse data */
typedef struct
{
	/* Mouse driver */
	wnd_mouse_driver_t m_driver;

	/* Mouse thread data */
	pthread_t m_tid;
	bool_t m_end_thread;

	/* The root window */
	struct tag_wnd_t *m_root_wnd;
} wnd_mouse_data_t;

/* Mouse buttons */
typedef enum
{
	WND_MOUSE_LEFT,
	WND_MOUSE_RIGHT,
	WND_MOUSE_MIDDLE
} wnd_mouse_button_t;

/* Mouse event type */
typedef enum
{
	WND_MOUSE_DOWN,
	WND_MOUSE_DOUBLE
} wnd_mouse_event_t;

/* Initialize mouse */
bool_t wnd_mouse_init( wnd_mouse_data_t *data );

/* Initialize mouse with GPM driver */
bool_t wnd_mouse_init_gpm( wnd_mouse_data_t *data );

/* Initialize mouse with xterm driver */
bool_t wnd_mouse_init_xterm( wnd_mouse_data_t *data );

/* Free mouse-related stuff */
void wnd_mouse_free( wnd_mouse_data_t *data );

/* Free mouse in GPM mode */
void wnd_mouse_free_gpm( wnd_mouse_data_t *data );

/* Free mouse in xterm mode */
void wnd_mouse_free_xterm( wnd_mouse_data_t *data );

/* Determine the mouse driver type */
wnd_mouse_driver_t wnd_get_mouse_type( cfg_node_t *cfg );

/* GPM Mouse thread function */
void *wnd_mouse_thread( void *arg );

/* Get window under which mouse cursor is */
struct tag_wnd_t *wnd_get_wnd_under_cursor( wnd_mouse_data_t *data, 
		int x, int y );

/* Find a given window child that is under the cursor */
struct tag_wnd_t *wnd_mouse_find_cursor_child( struct tag_wnd_t *wnd,
		int x, int y );

/* Handle the mouse event (send respective message) */
void wnd_mouse_handle_event( wnd_mouse_data_t *data, 
		int x, int y, wnd_mouse_button_t btn, wnd_mouse_event_t type,
		void *addional );

#endif

/* End of 'wnd_mouse.h' file */

