/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : listbox.h
 * PURPOSE     : SG MPFC. Interface for list box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.02.2004
 * NOTE        : Module prefix 'lbox'.
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

#ifndef __SG_MPFC_LIST_BOX_H__
#define __SG_MPFC_LIST_BOX_H__

#include "types.h"
#include "window.h"

/* List box type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* List box label */
	char *m_label;

	/* List */
	struct tag_lbox_item_t
	{
		char *m_name;
	} *m_list;
	int m_size;

	/* Last pressed key */
	int m_last_key;

	/* Cursor position */
	int m_cursor;

	/* Scroll value */
	int m_scrolled;

	/* Is list box minimalizing? */
	bool_t m_minimalizing;

	/* Is list box expanded? */
	bool_t m_expanded;
} listbox_t;

/* List box notify messages */
#define LBOX_MOVE 0

/* Create a new list box */
listbox_t *lbox_new( wnd_t *parent, int x, int y, int width, 
						int height,	char *label );

/* Initialize list box */
bool_t lbox_init( listbox_t *ebox, wnd_t *parent, int x, int y, int width, 
					int height,	char *label );

/* Destroy list box */
void lbox_destroy( wnd_t *wnd );

/* List box display function */
void lbox_display( wnd_t *wnd, dword data );

/* List box key handler function */
void lbox_handle_key( wnd_t *wnd, dword data );

/* Add item to list box */
void lbox_add( listbox_t *lbox, char *name );

/* Move cursor */
void lbox_move_cursor( listbox_t *lbox, bool_t rel, int pos, bool_t expand );

/* Focus change handler */
void lbox_handle_focus( wnd_t *wnd, dword data );

/* Mouse left button click handler */
void lbox_handle_left_click( wnd_t *wnd, dword data );

/* Mouse left button double click handler */
void lbox_handle_dbl_click( wnd_t *wnd, dword data );

/* Mouse middle button click handler */
void lbox_handle_middle_click( wnd_t *wnd, dword data );

#endif

/* End of 'listbox.h' file */

