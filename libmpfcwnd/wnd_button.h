/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_button.h
 * PURPOSE     : MPFC Window Library. Interface for button
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.08.2004
 * NOTE        : Module prefix 'button'.
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

#ifndef __SG_MPFC_WND_BUTTON_H__
#define __SG_MPFC_WND_BUTTON_H__

#include "types.h"
#include "wnd.h"

/* Button window type */
typedef struct 
{
	/* Common window part */
	wnd_t m_wnd;

	/* Messages */
	wnd_msg_handler_t *m_on_clicked;
} button_t;

/* Convert a window object to button type */
#define BUTTON_OBJ(wnd)	((button_t *)wnd)

/* Create a new button */
button_t *button_new( char *title, wnd_t *parent, int x, int y, int width, 
		int height );

/* Button initialization function */
bool_t button_construct( button_t *btn, char *title, wnd_t *parent, int x,
		int y, int width, int height );

/* 
 * Message handlers
 */

/* 'display' message handler */
wnd_msg_retcode_t button_on_display( wnd_t *wnd );

/* 'keydown' message handler */
wnd_msg_retcode_t button_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t button_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* 
 * Class functions 
 */

/* Initialize button class */
wnd_class_t *button_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **button_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Aliases for message data creating */
#define button_msg_clicked_new	wnd_msg_noargs_new

#endif

/* End of 'wnd_button.h' file */

