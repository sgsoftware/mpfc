/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_class.h
 * PURPOSE     : MPFC Window Library. Interface for window
 *               classes handling functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.08.2004
 * NOTE        : Module prefix 'wnd_class'.
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
#include "wnd_class.h"
#include "wnd_types.h"

/* Callback function for a message (i.e. function that calls the handler
 * translating message data into specific handler's arguments */
typedef wnd_msg_retcode_t (*wnd_class_msg_callback_t)( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Get message handler and callback function */
typedef wnd_msg_handler_t **(*wnd_class_msg_get_info_t)( wnd_t *wnd, 
		char *msg_name, wnd_class_msg_callback_t *callback );

/* Window class data */
struct tag_wnd_class_t
{
	/* Class name */
	char *m_name;
	
	/* Parent class */
	wnd_class_t *m_parent;

	/* Get specified message handler and callback function */
	wnd_class_msg_get_info_t m_get_info;

	/* Next class in the classes table */
	wnd_class_t *m_next;
};

/* Create a new window class */
wnd_class_t *wnd_class_new( wnd_global_data_t *global, char *name,
		wnd_class_t *parent, wnd_class_msg_get_info_t get_info_func );

/* Free window class */
void wnd_class_free( wnd_class_t *klass );

#endif

/* End of 'wnd_class.h' file */

