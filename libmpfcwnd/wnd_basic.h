/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_basic.h
 * PURPOSE     : MPFC Window Library. Interface for 'basic' window
 *               class.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.08.2004
 * NOTE        : Module prefix 'wnd_msg'.
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

#ifndef __SG_MPFC_WND_COMMON_MSG_H__
#define __SG_MPFC_WND_COMMON_MSG_H__

#include "types.h"
#include "wnd_class.h"
#include "wnd_kbd.h"
#include "wnd_mouse.h"
#include "wnd_msg.h"
#include "wnd_types.h"

/* Initialize basic window class */
wnd_class_t *wnd_basic_class_init( wnd_global_data_t *global );

/* Get message handler and callback function */
wnd_msg_handler_t **wnd_basic_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/*
 * Messages without arguments 
 */

/* Create data for no-arguments message (base function) */
wnd_msg_data_t wnd_msg_noargs_new( void );

/* Aliases for this function (for using with specific messages) */
#define wnd_msg_display_new 	wnd_msg_noargs_new 
#define wnd_msg_close_new		wnd_msg_noargs_new 
#define wnd_msg_erase_back_new	wnd_msg_noargs_new 

/* Callback function for no-arguments messages */
wnd_msg_retcode_t wnd_basic_callback_noargs( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/*
 * Destructor
 */

/* Callback for destructor */
wnd_msg_retcode_t wnd_basic_callback_destructor( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/*
 * Keyboard messages 
 */

/* Message data */
typedef struct
{
	wnd_key_t m_keycode;
} wnd_msg_key_t;

/* Create data for key-related message */
wnd_msg_data_t wnd_msg_key_new( wnd_key_t *keycode );

/* Alias */
#define wnd_msg_keydown_new	wnd_msg_key_new 

/* Callback function for keyboard messages */
wnd_msg_retcode_t wnd_basic_callback_key( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/*
 * Parent reposition message
 */

/* Parent reposition message data */
typedef struct
{
	int m_prev_x, m_prev_y, m_prev_w, m_prev_h;
	int m_new_x, m_new_y, m_new_w, m_new_h;
} wnd_msg_parent_repos_t;

/* Create data for parent reposition message */
wnd_msg_data_t wnd_msg_parent_repos_new( int px, int py, int pw, int ph,
		int nx, int ny, int nw, int nh );

/* Callback function for parent reposition message */
wnd_msg_retcode_t wnd_basic_callback_parent_repos( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/*
 * Mouse messages
 */

/* Mouse message data */
typedef struct
{
	/* Cursor position */
	int m_x, m_y;

	/* Mouse event type */
	wnd_mouse_event_t m_type;

	/* Pressed button */
	wnd_mouse_button_t m_button;
} wnd_msg_mouse_t;

/* Create mouse message data */
wnd_msg_data_t wnd_msg_mouse_new( int x, int y, wnd_mouse_event_t type,
		wnd_mouse_button_t button );

/* Aliases */
#define wnd_msg_mouse_ldown_new		wnd_msg_mouse_new
#define wnd_msg_mouse_mdown_new		wnd_msg_mouse_new
#define wnd_msg_mouse_rdown_new		wnd_msg_mouse_new
#define wnd_msg_mouse_ldouble_new	wnd_msg_mouse_new
#define wnd_msg_mouse_mdouble_new	wnd_msg_mouse_new
#define wnd_msg_mouse_rdouble_new	wnd_msg_mouse_new

/* Callback function */
wnd_msg_retcode_t wnd_basic_callback_mouse( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

#endif

/* End of 'wnd_basic.h' file */

