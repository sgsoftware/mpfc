/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_common_msg.h
 * PURPOSE     : MPFC Window Library. Interface for common 
 *               messages.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.07.2004
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
#include "wnd_kbd.h"
#include "wnd_mouse.h"
#include "wnd_msg.h"

/* Types */
#define WND_MSG_DISPLAY			0
#define WND_MSG_KEYDOWN			1
#define WND_MSG_CLOSE			2
#define WND_MSG_ERASE_BACK		3
#define WND_MSG_UPDATE_SCREEN	4
#define WND_MSG_PARENT_REPOS	5
#define WND_MSG_MOUSE_LDOWN		6
#define WND_MSG_MOUSE_RDOWN		7
#define WND_MSG_MOUSE_MDOWN		8
#define WND_MSG_MOUSE_LDOUBLE	9
#define WND_MSG_MOUSE_RDOUBLE	10
#define WND_MSG_MOUSE_MDOUBLE	11

/********** Messages data ***********/

/* Key-related message data type */
typedef struct 
{
	/* Key code */
	wnd_key_t m_keycode;
} wnd_msg_data_key_t;

/* Parent reposition notification message data */
typedef struct 
{
	/* Previous position */
	int m_prev_x, m_prev_y, m_prev_width, m_prev_height;

	/* New position */
	int m_new_x, m_new_y, m_new_width, m_new_height;
} wnd_msg_data_parent_repos_t;

/* Mouse message data */
typedef struct
{
	/* Cursor position */
	int m_x, m_y;

	/* Mouse event type */
	wnd_mouse_event_t m_type;

	/* Pressed button */
	wnd_mouse_button_t m_button;
} wnd_msg_data_mouse_t;

/********** Constructors/destructors/callbacks *********/

/* Construct a display message data */
wnd_msg_data_t wnd_msg_data_display_new( void );

/* Callback for WND_MSG_DISPLAY */
wnd_msg_retcode_t wnd_callback_display( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a key-related message data */
wnd_msg_data_t wnd_msg_data_key_new( wnd_key_t *keycode );

/* Callback for WND_MSG_KEYDOWN */
wnd_msg_retcode_t wnd_callback_keydown( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a close message data */
wnd_msg_data_t wnd_msg_data_close_new( void );

/* Callback for WND_MSG_CLOSE */
wnd_msg_retcode_t wnd_callback_close( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a erase background message data */
wnd_msg_data_t wnd_msg_data_erase_back_new( void );

/* Callback for WND_MSG_ERASE_BACK */
wnd_msg_retcode_t wnd_callback_erase_back( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Callback for WND_MSG_ERASE_BACK */
wnd_msg_retcode_t wnd_callback_erase_back( struct tag_wnd_t *wnd,
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a update screen message data */
wnd_msg_data_t wnd_msg_data_update_screen_new( void );

/* Callback for WND_MSG_UPDATE_SCREEN */
wnd_msg_retcode_t wnd_callback_update_screen( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a parent reposition message data */
wnd_msg_data_t wnd_msg_data_parent_repos_new( int px, int py, int pw, int ph,
		int nx, int ny, int nw, int nh );

/* Callback for WND_MSG_PARENT_REPOS */
wnd_msg_retcode_t wnd_callback_parent_repos( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Construct a mouse message data */
wnd_msg_data_t wnd_msg_data_mouse_new( int x, int y, wnd_mouse_button_t btn,
		wnd_mouse_event_t type );

/* Callback for mouse messages */
wnd_msg_retcode_t wnd_callback_mouse( struct tag_wnd_t *wnd,
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

#endif

/* End of 'wnd_common_msg.h' file */

