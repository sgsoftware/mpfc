/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_combobox.h
 * PURPOSE     : MPFC Window Library. Interface for combo box
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 20.08.2004
 * NOTE        : Module prefix 'combo'.
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

#ifndef __SG_MPFC_WND_COMBOBOX_H__
#define __SG_MPFC_WND_COMBOBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_editbox.h"

/* Combo box type */
typedef struct
{
	/* Edit box part */
	editbox_t m_wnd;

	/* Items list */
	char **m_list;
	int m_list_size;

	/* Currently selected item */
	int m_cursor;
	int m_scrolled;

	/* Whether combo box is expanded? */
	bool_t m_expanded;

	/* The height of list in expanded state */
	int m_height;
} combo_t;

/* Convert a window object t combo box type */
#define COMBO_OBJ(wnd)	((combo_t *)wnd)

/* Create a new combo box */
combo_t *combo_new( wnd_t *parent, char *id, char *text, char letter, 
		int width, int height );

/* Create a new combo box with a label */
combo_t *combo_new_with_label( wnd_t *parent, char *title, 
		char *id, char *text, char letter, int width, int height );

/* Combo box constructor */
bool_t combo_construct( combo_t *combo, wnd_t *parent, char *id, 
		char *text, char letter, int width, int height );

/* Destructor */
void combo_destructor( wnd_t *wnd );

/* Add an item to the list */
void combo_add_item( combo_t *combo, char *item );

/* Set new cursor position in the list */
void combo_move_cursor( combo_t *combo, int pos, bool_t synch_text );

/* Expand combo box */
void combo_expand( combo_t *combo );

/* Unexpand combo box */
void combo_unexpand( combo_t *combo );

/* Synchronize list cursor with text */
void combo_synch_list( combo_t *combo );

/* 'keydown' message handler */
wnd_msg_retcode_t combo_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'display' message handler */
wnd_msg_retcode_t combo_on_display( wnd_t *wnd );

/* 'changed' message handler */
wnd_msg_retcode_t combo_on_changed( wnd_t *wnd );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t combo_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* 'loose_focus' message handler */
wnd_msg_retcode_t combo_on_loose_focus( wnd_t *wnd );

#endif

/* End of 'wnd_combobox.h' file */

