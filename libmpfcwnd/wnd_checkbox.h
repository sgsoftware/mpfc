/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_checkbox.h
 * PURPOSE     : MPFC Window Library. Interface for checkbox
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 18.08.2004
 * NOTE        : Module prefix 'checkbox'.
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

/* Check box type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;
	
	/* Check box state */
	bool_t m_checked;
} checkbox_t;

/* Convert a window object to check box type */
#define CHECKBOX_OBJ(wnd)	((checkbox_t *)wnd)

/* Create a new check box */
checkbox_t *checkbox_new( wnd_t *parent, char *title, char *id, 
		bool_t checked );

/* Check box constructor */
bool_t checkbox_construct( checkbox_t *cb, wnd_t *parent, char *title, 
		char *id, bool_t checked );

/* 'keydown' message handler */
wnd_msg_retcode_t checkbox_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'display' message handler */
wnd_msg_retcode_t checkbox_on_display( wnd_t *wnd );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t checkbox_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* Toggle checked state */
void checkbox_toggle( checkbox_t *cb );

/* Get size desired by check box */
void checkbox_get_desired_size( dlgitem_t *di, int *width, int *height );

#endif

/* End of 'wnd_checkbox.h' file */

