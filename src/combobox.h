/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : combobox.h
 * PURPOSE     : SG MPFC. Interface for combo box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2003
 * NOTE        : Module prefix 'cbox'.
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

#ifndef __SG_MPFC_COMBO_BOX_H__
#define __SG_MPFC_COMBO_BOX_H__

#include "types.h"
#include "window.h"

/* Combo box type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Label */
	char *m_label;
	int m_label_len;

	/* Edit mode text */
	char m_text[256];
	int m_text_len;
	int m_edit_cursor;

	/* List */
	char **m_list;
	int m_list_size;
	int m_list_cursor;
	int m_list_height;
	int m_scrolled;

	/* Is list expanded? */
	bool m_expanded;
} combobox_t;

/* Create a new combo box */
combobox_t *cbox_new( wnd_t *parent, int x, int y, int width, 
						int height, char *label );

/* Initialize combo box */
bool cbox_init( combobox_t *cbox, wnd_t *parent, int x, int y, int width, 
					int height, char *label );

/* Destroy combo box */
void cbox_destroy( wnd_t *wnd );

/* Combo box display function */
void cbox_display( wnd_t *wnd, dword data );

/* Combo box key handler function */
void cbox_handle_key( wnd_t *wnd, dword data );

/* Set edit box text */
void cbox_set_text( combobox_t *cb, char *text );

/* Move edit box cursor */
void cbox_move_edit_cursor( combobox_t *cb, bool rel, int pos );

/* Add charater to edit box */
void cbox_add_ch( combobox_t *cb, char ch );

/* Delete character form edit box */
void cbox_del_ch( combobox_t *cb, bool before_cursor );

/* Add string to list */
void cbox_list_add( combobox_t *cb, char *str );

/* Move list box cursor */
void cbox_move_list_cursor( combobox_t *cb, bool rel, int pos, bool expand,
								bool change_edit );

/* Update list box using text from edit box */
void cbox_edit2list( combobox_t *cb );

#endif

/* End of 'combobox.h' file */

