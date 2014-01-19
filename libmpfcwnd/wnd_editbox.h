/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for edit box functions.
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

#ifndef __SG_MPFC_WND_EDITBOX_H__
#define __SG_MPFC_WND_EDITBOX_H__

#include "types.h"
#include "mystring.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Edit box history list type */
typedef struct 
{
	/* List */
	struct editbox_history_item_t
	{
		char *m_text;
		struct editbox_history_item_t *m_next, *m_prev;
	} *m_head, *m_tail, *m_cur;
} editbox_history_t;

/* Edit box type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_changed;

	/* Edit box text */
	str_t *m_text;

	/* Cursor position */
	int m_cursor;
	int m_cursor_byte;

	/* Scroll value */
	int m_scrolled;
	int m_scrolled_byte;

	/* The desired width */
	int m_width;

	/* Has the text been modified? */
	bool_t m_modified;

	/* The history list for this edit box */
	editbox_history_t *m_history;

	/* Edit box text before inserting text from history */
	str_t *m_text_before_hist;

	/* Should non-modified text be displayed as grayed? */
	bool_t m_gray_non_modified;

	/* Edit box state changed */
	bool_t m_state_changed;

	/* Is this edit box editable */
	bool_t m_editable;
} editbox_t;

/* Convert window object to edit box type */
#define EDITBOX_OBJ(wnd)	((editbox_t *)wnd)

/* Access edit box data */
#define EDITBOX_TEXT(wnd)	    (STR_TO_CPTR(EDITBOX_OBJ(wnd)->m_text))
#define EDITBOX_BYTE_LEN(wnd)	(STR_BYTE_LEN(EDITBOX_OBJ(wnd)->m_text))
#define EDITBOX_EMPTY(wnd)	    (*EDITBOX_TEXT(wnd) == 0)

/* Create a new edit box */
editbox_t *editbox_new( wnd_t *parent, char *id, char *text, char letter,
		int width );

/* Edit box constructor */
bool_t editbox_construct( editbox_t *eb, wnd_t *parent, char *id, char *text,
		char letter, int width );

/* Create an edit box with label */
editbox_t *editbox_new_with_label( wnd_t *parent, char *title, char *id,
		char *text, char letter, int width );

/* Destructor */
void editbox_destructor( wnd_t *wnd );

/* Get edit box desired size */
void editbox_get_desired_size( dlgitem_t *di, int *width, int *height );

/* Set edit box text */
void editbox_set_text( editbox_t *eb, const char *text );

/* Add character into the current cursor position */
void editbox_addch( editbox_t *eb, char ch );

/* Delete character from the current or previous cursor position */
void editbox_delch( editbox_t *eb, bool_t before_cursor );

/* Move cursor */
bool_t editbox_move( editbox_t *eb, int new_pos );

/* 'display' message handler */
wnd_msg_retcode_t editbox_on_display( wnd_t *wnd );

/* 'keydown' message handler */
wnd_msg_retcode_t editbox_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'action' message handler */
wnd_msg_retcode_t editbox_on_action( wnd_t *wnd, char *action );

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t editbox_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type );

/* Initialize history list */
editbox_history_t *editbox_history_new( void );

/* Free history list */
void editbox_history_free( editbox_history_t *l );

/* Add an item to history list */
void editbox_history_add( editbox_history_t *l, char *text );

/* Handle history moving */
void editbox_hist_move( editbox_t *box, bool_t up );

/*
 * Class functions
 */

/* Create edit box class */
wnd_class_t *editbox_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **editbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void editbox_free_handlers( wnd_t *wnd );

/* Set edit box class default styles */
void editbox_class_set_default_styles( cfg_node_t *list );

/* Aliases for creating message data */
#define editbox_changed_new		wnd_msg_noargs_new

#endif

/* End of 'wnd_editbox.h' file */

