/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for checkbox functions.
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

#ifndef __SG_MPFC_WND_CHECKBOX_H__
#define __SG_MPFC_WND_CHECKBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_label.h"

/* Check box type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Label */
	label_text_t m_text;

	/* Message handlers */
	wnd_msg_handler_t *m_on_clicked;
	
	/* Check box state */
	bool_t m_checked;
} checkbox_t;

/* Convert a window object to check box type */
#define CHECKBOX_OBJ(wnd)	((checkbox_t *)wnd)

/* Create a new check box */
checkbox_t *checkbox_new( wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked );

/* Check box constructor */
bool_t checkbox_construct( checkbox_t *cb, wnd_t *parent, char *title, 
		char *id, char letter, bool_t checked );

/* 'action' message handler */
wnd_msg_retcode_t checkbox_on_action( wnd_t *wnd, char *action );

/* 'display' message handler */
wnd_msg_retcode_t checkbox_on_display( wnd_t *wnd );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t checkbox_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* 'quick_change_focus' message handler */
wnd_msg_retcode_t checkbox_on_quick_change_focus( wnd_t *wnd );

/* Toggle checked state */
void checkbox_toggle( checkbox_t *cb );

/* Get size desired by check box */
void checkbox_get_desired_size( dlgitem_t *di, int *width, int *height );

/* 
 * Class functions 
 */

/* Initialize checkbox class */
wnd_class_t *checkbox_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **checkbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void checkbox_free_handlers( wnd_t *wnd );

/* Set check box class default styles */
void checkbox_class_set_default_styles( cfg_node_t *list );

/* Aliases for message data creating */
#define checkbox_msg_clicked_new	wnd_msg_noargs_new

#endif

/* End of 'wnd_checkbox.h' file */

