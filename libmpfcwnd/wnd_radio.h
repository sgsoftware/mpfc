/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for radio button functions.
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

#ifndef __SG_MPFC_WND_RADIO_H__
#define __SG_MPFC_WND_RADIO_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_label.h"

/* Radio button type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	label_text_t m_text;

	/* If dialog item is checked */
	bool_t m_checked;
} radio_t;

/* Convert a window object to radio button type */
#define RADIO_OBJ(wnd)	((radio_t *)wnd)

/* Create a new radio button */
radio_t *radio_new( wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked );

/* Radio button constructor */
bool_t radio_construct( radio_t *r, wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked );

/* 'action' message handler */
wnd_msg_retcode_t radio_on_action( wnd_t *wnd, char *action );

/* 'display' message handler */
wnd_msg_retcode_t radio_on_display( wnd_t *wnd );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t radio_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* 'quick_change_focus' message handler */
wnd_msg_retcode_t radio_on_quick_change_focus( wnd_t *wnd );

/* Set checked state */
void radio_check( radio_t *r );

/* Get size desired by check box */
void radio_get_desired_size( dlgitem_t *di, int *width, int *height );

/*
 * Class functions
 */

/* Initialize radio button class */
wnd_class_t *radio_class_init( wnd_global_data_t *global );

/* Set radio button class default styles */
void radio_class_set_default_styles( cfg_node_t *list );

#endif

/* End of 'wnd_radio.h' file */

