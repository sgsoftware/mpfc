/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_listbox.h
 * PURPOSE     : MPFC Window Library. Interface for list box
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.10.2004
 * NOTE        : Module prefix 'listbox'.
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

#ifndef __SG_MPFC_WND_LISTBOX_H__
#define __SG_MPFC_WND_LISTBOX_H__

#include "types.h"
#include "wnd_dlgitem.h"

/* List box flags */
typedef enum
{
	LISTBOX_SELECTABLE = 1 << 0
} listbox_flags_t;

/* List box type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_changed,
					  *m_on_selection_changed;

	/* The list */
	struct listbox_item_t
	{
		char *m_name;
		void *m_data;
	} *m_list;
	int m_list_size;

	/* List box flags */
	listbox_flags_t m_flags;

	/* Cursor position */
	int m_cursor;

	/* Scrolling value */
	int m_scroll;

	/* Window size */
	int m_width, m_height;

	/* Selected item */
	int m_selected;
} listbox_t;

/* Convert window object to list box type */
#define LISTBOX_OBJ(wnd)	((listbox_t *)wnd)

/* Create a new list box */
listbox_t *listbox_new( wnd_t *parent, char *title, char *id, char letter, 
		listbox_flags_t flags, int width, int height );

/* List box constructor */
bool_t listbox_construct( listbox_t *lb, wnd_t *parent, char *title, char *id, 
		char letter, listbox_flags_t flags, int width, int height );

/* List box destructor */
void listbox_destructor( wnd_t *wnd );

/* Add an item */
int listbox_add( listbox_t *lb, char *item, void *data );

/* Move cursor */
void listbox_move( listbox_t *lb, int pos, bool_t rel );

/* Select an item */
void listbox_sel_item( listbox_t *lb, int pos );

/* 'display' message handler */
wnd_msg_retcode_t listbox_on_display( wnd_t *wnd );

/* 'action' message handler */
wnd_msg_retcode_t listbox_on_action( wnd_t *wnd, char *action );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t listbox_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* Get list box desired size */
void listbox_get_desired_size( dlgitem_t *di, int *width, int *height );

/*
 * Class functions
 */

/* Create list box class */
wnd_class_t *listbox_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **listbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void listbox_free_handlers( wnd_t *wnd );

/* Set list box class default styles */
void listbox_class_set_default_styles( cfg_node_t *list );

/*
 * Item selection message 
 */

/* Item selection message data */
typedef struct
{
	int m_item;
} listbox_msg_changed_t;

/* Create item selection message data */
wnd_msg_data_t listbox_msg_changed_new( int item );

/* Callback function */
wnd_msg_retcode_t listbox_callback_changed( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define LISTBOX_MSG_CHANGED_HANDLER(h) \
	((wnd_msg_retcode_t (*)(wnd_t *, int))(h->m_func))

/* Aliases */
#define listbox_msg_sel_changed_new listbox_msg_changed_new

#endif

/* End of 'wnd_listbox.h' file */

