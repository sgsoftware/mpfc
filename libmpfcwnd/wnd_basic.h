/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for 'basic' window class.
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

/* Set the default styles */
void wnd_basic_class_set_default_styles( cfg_node_t *list );

/* Free message handlers */
void wnd_basic_free_handlers( wnd_t *wnd );

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
#define wnd_msg_loose_focus_new	wnd_msg_noargs_new
#define wnd_msg_get_focus_new	wnd_msg_noargs_new

/* Callback function for no-arguments messages */
wnd_msg_retcode_t wnd_basic_callback_noargs( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define WND_MSG_NOARGS_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *))(h->m_func))

/*
 * Destructor
 */

/* Callback for destructor */
wnd_msg_retcode_t wnd_basic_callback_destructor( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define WND_MSG_DESTRUCTOR_HANDLER(h)	\
	((void (*)(wnd_t *))(h->m_func))

/*
 * Keyboard messages 
 */

/* Message data */
typedef struct
{
	wnd_key_t m_key;
} wnd_msg_key_t;

/* Create data for key-related message */
wnd_msg_data_t wnd_msg_key_new( wnd_key_t key );

/* Alias */
#define wnd_msg_keydown_new	wnd_msg_key_new 

/* Callback function for keyboard messages */
wnd_msg_retcode_t wnd_basic_callback_key( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define WND_MSG_KEY_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, wnd_key_t))(h->m_func))

/*
 * Action message
 */

/* Message data */
typedef struct
{
	char *m_action;
	int m_repval;
} wnd_msg_action_t;

/* Create data for action message */
wnd_msg_data_t wnd_msg_action_new( char *action, int repval );

/* Action message data destructor */
void wnd_msg_action_free( void *data );

/* Callback function for action message */
wnd_msg_retcode_t wnd_basic_callback_action( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define WND_MSG_ACTION_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, char *, int))(h->m_func))

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

/* Convert handler pointer to the proper type */
#define WND_MSG_PARENT_REPOS_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, int, int, int, int, \
							int, int, int, int))(h->m_func))

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

/* Convert handler pointer to the proper type */
#define WND_MSG_MOUSE_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, int, int, wnd_mouse_button_t, \
							wnd_mouse_event_t))(h->m_func))

/*
 * User message
 */

/* User message data */
typedef struct
{
	/* User message ID */
	int m_id;

	/* Additional data */
	void *m_data;
} wnd_msg_user_t;

/* Create user message data */
wnd_msg_data_t wnd_msg_user_new( int id, void *data );

/* Callback function */
wnd_msg_retcode_t wnd_basic_callback_user( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define WND_MSG_USER_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, int, void *))(h->m_func))

#endif

/* End of 'wnd_basic.h' file */

