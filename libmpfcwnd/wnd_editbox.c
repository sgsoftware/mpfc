/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_editbox.c
 * PURPOSE     : MPFC Window Library. Edit box functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'editbox'.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mystring.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_editbox.h"

/* Create a new edit box */
editbox_t *editbox_new( wnd_t *parent, char *id, char *text, int width )
{
	editbox_t *eb;

	/* Allocate memory */
	eb = (editbox_t *)malloc(sizeof(*eb));
	if (eb == NULL)
		return NULL;
	memset(eb, 0, sizeof(*eb));
	WND_OBJ(eb)->m_class = wnd_basic_class_init(WND_GLOBAL(parent));

	/* Initialize edit box */
	if (!editbox_construct(eb, parent, id, text, width))
	{
		free(eb);
		return NULL;
	}
	WND_OBJ(eb)->m_flags |= WND_FLAG_INITIALIZED;
	return eb;
} /* End of 'editbox_new' function */

/* Edit box constructor */
bool_t editbox_construct( editbox_t *eb, wnd_t *parent, char *id, char *text,
		int width )
{
	wnd_t *wnd = WND_OBJ(eb);

	/* Initialize window part */
	if (!dlgitem_construct(DLGITEM_OBJ(eb), parent, "", id, 
				editbox_get_desired_size, NULL, 0))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "display", editbox_on_display);
	wnd_msg_add_handler(wnd, "keydown", editbox_on_keydown);
	wnd_msg_add_handler(wnd, "mouse_ldown", editbox_on_mouse);
	wnd_msg_add_handler(wnd, "destructor", editbox_destructor);

	/* Initialize edit box fields */
	eb->m_text = str_new(text);
	eb->m_cursor = 0;
	eb->m_scrolled = 0;
	eb->m_width = width;
	editbox_move(eb, STR_LEN(eb->m_text));
	return TRUE;
} /* End of 'editbox_construct' function */

/* Destructor */
void editbox_destructor( wnd_t *wnd )
{
	str_free(EDITBOX_OBJ(wnd)->m_text);
} /* End of 'editbox_destructor' function */

/* Get edit box desired size */
void editbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	editbox_t *eb = EDITBOX_OBJ(di);
	*width = eb->m_width;
	*height = 1;
} /* End of 'editbox_get_desired_size' function */

/* Set edit box text */
void editbox_set_text( editbox_t *eb, char *text )
{
	assert(eb);
	assert(text);
	str_copy_cptr(eb->m_text, text);
	editbox_move(eb, EDITBOX_LEN(eb));
	wnd_invalidate(WND_OBJ(eb));
} /* End of 'editbox_set_text' function */

/* Add character into the current cursor position */
void editbox_addch( editbox_t *eb, char ch )
{
	assert(eb);
	str_insert_char(eb->m_text, ch, eb->m_cursor);
	editbox_move(eb, eb->m_cursor + 1);
} /* End of 'editbox_addch' function */

/* Delete character from the current or previous cursor position */
void editbox_delch( editbox_t *eb, int pos )
{
	assert(eb);
	if (str_delete_char(eb->m_text, pos))
		editbox_move(eb, pos);
} /* End of 'editbox_delch' function */

/* Move cursor */
void editbox_move( editbox_t *eb, int new_pos )
{
	int old_cur = eb->m_cursor;
	int len = EDITBOX_LEN(eb);
	
	/* Set cursor position */
	eb->m_cursor = new_pos;
	if (eb->m_cursor < 0)
		eb->m_cursor = 0;
	else if (eb->m_cursor > len)
		eb->m_cursor = len;
	if (eb->m_cursor == 0)
	{
		eb->m_scrolled = 0;
		return;
	}

	/* Handle scrolling */
	while (eb->m_cursor < eb->m_scrolled + 1)
		eb->m_scrolled -= 5;
	while (eb->m_cursor >= eb->m_scrolled + WND_WIDTH(eb))
		eb->m_scrolled ++;
	if (eb->m_scrolled < 0)
		eb->m_scrolled = 0;
	else if (eb->m_scrolled >= EDITBOX_LEN(eb))
		eb->m_scrolled = EDITBOX_LEN(eb) - 1;
} /* End of 'editbox_move' function */

/* 'display' message handler */
wnd_msg_retcode_t editbox_on_display( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	assert(wnd);

	/* Print text */
	wnd_move(wnd, 0, 0, 0);
	wnd_printf(wnd, 0, 0, "%s", STR_TO_CPTR(eb->m_text) + eb->m_scrolled);

	/* Move cursor */
	wnd_move(wnd, 0, eb->m_cursor - eb->m_scrolled, 0);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_display' function */

/* 'keydown' message handler */
wnd_msg_retcode_t editbox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	/* Append char to the text */
	if (key >= ' ' && key <= 0xFF)
	{
		editbox_addch(eb, key);
	}
	/* Delete previous char */
	else if (key == KEY_BACKSPACE)
	{
		editbox_delch(eb, eb->m_cursor - 1);
	}
	/* Delete current character */
	else if (key == KEY_DC || key == KEY_CTRL_D)
	{
		editbox_delch(eb, eb->m_cursor);
	}
	/* Kill text from current position to the start */
	else if (key == KEY_CTRL_U)
	{
		while (eb->m_cursor != 0)
			editbox_delch(eb, eb->m_cursor - 1);
	}
	/* Kill text from current position to the end */
	else if (key == KEY_CTRL_K)
	{
		int count = EDITBOX_LEN(eb) - eb->m_cursor;
		int was_cursor = eb->m_cursor;
		for ( ; count > 0; count -- )
			editbox_delch(eb, was_cursor);
		editbox_move(eb, was_cursor);
	}
	/* Move cursor right */
	else if (key == KEY_RIGHT || key == KEY_CTRL_F)
	{
		editbox_move(eb, eb->m_cursor + 1);
	}
	/* Move cursor left */
	else if (key == KEY_LEFT || key == KEY_CTRL_B)
	{
		editbox_move(eb, eb->m_cursor - 1);
	}
	/* Move cursor to text start */
	else if (key == KEY_HOME || key == KEY_CTRL_A)
	{
		editbox_move(eb, 0);
	}
	/* Move cursor to text end */
	else if (key == KEY_END || key == KEY_CTRL_E)
	{
		editbox_move(eb, EDITBOX_LEN(eb));
	}
	else
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_keydown' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t editbox_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	/* Move cursor to the respective position */
	editbox_move(eb, x - eb->m_scrolled);
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_mouse' function */

/* End of 'wnd_editbox.c' file */

