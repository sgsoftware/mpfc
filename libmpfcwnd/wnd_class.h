/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for window classes handling 
 * functions.
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

#ifndef __SG_MPFC_WND_CLASS_H__
#define __SG_MPFC_WND_CLASS_H__

#include "types.h"
#include "cfg.h"
#include "wnd_class.h"
#include "wnd_types.h"

/* Callback function for a message (i.e. function that calls the handler
 * translating message data into specific handler's arguments */
typedef wnd_msg_retcode_t (*wnd_class_msg_callback_t)( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Get message handler and callback function */
typedef wnd_msg_handler_t **(*wnd_class_msg_get_info_t)( wnd_t *wnd, 
		char *msg_name, wnd_class_msg_callback_t *callback );

/* Free window's message handlers */
typedef void (*wnd_class_free_handlers_t)( wnd_t *wnd );

/* Window class data */
struct tag_wnd_class_t
{
	/* Class name */
	char *m_name;
	
	/* Parent class */
	wnd_class_t *m_parent;

	/* Callbacks */
	wnd_class_msg_get_info_t m_get_info;
	wnd_class_free_handlers_t m_free_handlers;

	/* Class configuration */
	cfg_node_t *m_cfg_list;

	/* Next class in the classes table */
	wnd_class_t *m_next;
};

/* Create a new window class */
wnd_class_t *wnd_class_new( wnd_global_data_t *global, char *name,
		wnd_class_t *parent, wnd_class_msg_get_info_t get_info_func,
		wnd_class_free_handlers_t free_handlers_func,
		cfg_set_default_values_t set_def_styles );

/* Free window class */
void wnd_class_free( wnd_class_t *klass );

/* Call 'get_msg_info' function */
wnd_msg_handler_t **wnd_class_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Call 'free_handlers' function */
void wnd_class_free_handlers( wnd_t *wnd );

#endif

/* End of 'wnd_class.h' file */

