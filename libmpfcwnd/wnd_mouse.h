/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for mouse functions. 
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
#include "wnd_types.h"

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
	wnd_t *m_root_wnd;

	/* Global data */
	wnd_global_data_t *m_global;
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
wnd_mouse_data_t *wnd_mouse_init( wnd_global_data_t *global );

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
wnd_mouse_driver_t wnd_mouse_get_driver( cfg_node_t *cfg );

/* GPM Mouse thread function */
void *wnd_mouse_thread( void *arg );

/* Get window under which mouse cursor is */
wnd_t *wnd_get_wnd_under_cursor( wnd_mouse_data_t *data, int x, int y );

/* Find a given window child that is under the cursor */
wnd_t *wnd_mouse_find_cursor_child( wnd_t *wnd, int x, int y );

/* Handle the mouse event (send respective message) */
void wnd_mouse_handle_event( wnd_mouse_data_t *data, 
		int x, int y, wnd_mouse_button_t btn, wnd_mouse_event_t type,
		void *addional );

#endif

/* End of 'wnd_mouse.h' file */

