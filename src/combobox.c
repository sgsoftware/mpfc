/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : combobox.c
 * PURPOSE     : SG MPFC. Combo box functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.11.2003
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "colors.h"
#include "combobox.h"
#include "dlgbox.h"
#include "error.h"
#include "window.h"

/* Create a new combo box */
combobox_t *cbox_new( wnd_t *parent, int x, int y, int width, 
						int height,	char *label )
{
	combobox_t *wnd;

	/* Allocate memory for edit box object */
	wnd = (combobox_t *)malloc(sizeof(combobox_t));
	if (wnd == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize combo box fields */
	if (!cbox_init(wnd, parent, x, y, width, height, label))
	{
		free(wnd);
		return NULL;
	}
	return wnd;
} /* End of 'cbox_new' function */

/* Initialize combo box */
bool_t cbox_init( combobox_t *wnd, wnd_t *parent, int x, int y, int width, 
					int height,	char *label )
{
	/* Create common window part */
	if (!wnd_init(&wnd->m_wnd, parent, x, y, width, height))
	{
		return FALSE;
	}

	/* Register message handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, cbox_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, cbox_handle_key);
	wnd_register_handler(wnd, WND_MSG_CHANGE_FOCUS, cbox_handle_focus);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_CLICK, cbox_handle_left_click);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_DOUBLE, 
			cbox_handle_dbl_click);
	wnd_register_handler(wnd, WND_MSG_MOUSE_MIDDLE_CLICK, 
			cbox_handle_middle_click);
	wnd_register_handler(wnd, WND_MSG_MOUSE_OUTSIDE_FOCUS, 
			cbox_handle_mouse_outside);
	WND_OBJ(wnd)->m_wnd_destroy = cbox_destroy;
	WND_OBJ(wnd)->m_flags = WND_ITEM;

	/* Set combobox-specific fields */
	wnd->m_label = strdup(label);
	wnd->m_label_len = strlen(label);
	strcpy(wnd->m_text, "");
	wnd->m_text_len = 0;
	wnd->m_edit_cursor = 0;
	wnd->m_list = NULL;
	wnd->m_list_size = 0;
	wnd->m_list_cursor = -1;
	wnd->m_list_height = height - 1;
	wnd->m_scrolled = 0;
	wnd->m_expanded = FALSE;
	wnd->m_changed = FALSE;
	wnd->m_grayed = FALSE;
	WND_OBJ(wnd)->m_flags |= (WND_ITEM | WND_INITIALIZED);
	return TRUE;
} /* End of 'cbox_init' function */

/* Destroy combo box */
void cbox_destroy( wnd_t *wnd )
{
	combobox_t *cb = (combobox_t *)wnd;
	int i;
	
	if (wnd == NULL)
		return;
	
	if (cb->m_label != NULL)
		free(cb->m_label);
	for ( i = 0; i < cb->m_list_size; i ++ )
		free(cb->m_list[i]);
	if (cb->m_list != NULL)
		free(cb->m_list);
	wnd_destroy_func(wnd);
} /* End of 'cbox_destroy' function */

/* Combo box display function */
void cbox_display( wnd_t *wnd, dword data )
{
	combobox_t *cb = (combobox_t *)wnd;
	int i, y;
	
	if (wnd == NULL)
		return;

	/* Display label */
	wnd_move(wnd, 0, 0);
	col_set_color(wnd, COL_EL_DLG_ITEM_TITLE);
	wnd_printf(wnd, "%s", cb->m_label);

	/* Display edit box */
	col_set_color(wnd, (!cb->m_changed && cb->m_grayed) ?
			COL_EL_DLG_ITEM_GRAYED : COL_EL_DLG_ITEM_CONTENT);
	wnd_printf(wnd, "%s", cb->m_text);
	col_set_color(wnd, COL_EL_DEFAULT);

	/* Display list box */
	if (cb->m_expanded)
	{
		for ( i = cb->m_scrolled, y = 1; 
				y <= cb->m_list_height && i < cb->m_list_size; i ++, y ++ )
		{
			wnd_move(wnd, cb->m_label_len, y);
			if (i == cb->m_list_cursor)
			{
				col_set_color(wnd, wnd_is_focused(wnd) ? 
						COL_EL_LBOX_CUR_FOCUSED : COL_EL_LBOX_CUR);
			}
			else
				col_set_color(wnd, COL_EL_DLG_ITEM_CONTENT);
			wnd_printf(wnd, "%s\n", cb->m_list[i]);
		}
		col_set_color(wnd, COL_EL_DEFAULT);
	}

	/* Place cursor to edit box */
	if (cb->m_edit_cursor >= 0)
		wnd_move(wnd, cb->m_label_len + cb->m_edit_cursor, 0);
} /* End of 'cbox_display' function */

/* Combo box key handler function */
void cbox_handle_key( wnd_t *wnd, dword data )
{
	combobox_t *cb = (combobox_t *)wnd;
	int key = (int)data;

	if (wnd == NULL)
		return;

	/* List box cursor movement */
	if (key == KEY_DOWN)
	{
		if (cb->m_list_cursor < 0)
			cbox_move_list_cursor(cb, FALSE, 0, TRUE, TRUE);
		else
			cbox_move_list_cursor(cb, TRUE, 1, TRUE, TRUE);
	}
	else if (key == KEY_UP)
	{
		if (cb->m_list_cursor < 0)
			cbox_move_list_cursor(cb, FALSE, 0, TRUE, TRUE);
		else
			cbox_move_list_cursor(cb, TRUE, -1, TRUE, TRUE);
	}
	else if (key == KEY_NPAGE)
	{
		if (cb->m_list_cursor < 0)
			cbox_move_list_cursor(cb, FALSE, 0, TRUE, TRUE);
		else
			cbox_move_list_cursor(cb, TRUE, cb->m_list_height, TRUE, TRUE);
	}
	else if (key == KEY_PPAGE)
	{
		if (cb->m_list_cursor < 0)
			cbox_move_list_cursor(cb, FALSE, 0, TRUE, TRUE);
		else
			cbox_move_list_cursor(cb, TRUE, -cb->m_list_height, TRUE, TRUE);
	}
	/* Unexpand list */
	else if (key == '\n' && cb->m_expanded)
		cb->m_expanded = FALSE;
	/* Add character to edit box */
	else if (key >= ' ' && key <= 0xFF)
		cbox_add_ch(cb, key);
	/* Delete character */
	else if (key == KEY_BACKSPACE)
		cbox_del_ch(cb, TRUE);
	else if (key == KEY_DC)
		cbox_del_ch(cb, FALSE);
	/* Move edit box cursor */
	else if (key == KEY_RIGHT)
		cbox_move_edit_cursor(cb, TRUE, 1);
	else if (key == KEY_LEFT)
		cbox_move_edit_cursor(cb, TRUE, -1);
	else if (key == KEY_HOME)
		cbox_move_edit_cursor(cb, FALSE, 0);
	else if (key == KEY_END)
		cbox_move_edit_cursor(cb, FALSE, cb->m_text_len);
	/* Common dialog box item actions */
	else DLG_ITEM_HANDLE_COMMON(wnd, key);
} /* End of 'cbox_handle_key' function */

/* Set edit box text */
void cbox_set_text( combobox_t *cb, char *text )
{
	if (cb == NULL)
		return;

	/* Update text */
	strcpy(cb->m_text, text);
	cb->m_text_len = strlen(text);
	cb->m_edit_cursor = cb->m_text_len;

	/* Update list box */
	if (*text)
		cbox_edit2list(cb);
} /* End of 'cbox_set_text' function */

/* Move edit box cursor */
void cbox_move_edit_cursor( combobox_t *cb, bool_t rel, int pos )
{
	if (cb == NULL || !cb->m_text_len)
		return;

	/* Move cursor */
	if (rel)
		cb->m_edit_cursor += pos;
	else
		cb->m_edit_cursor = pos;
	if (cb->m_edit_cursor < 0)
		cb->m_edit_cursor = 0;
	else if (cb->m_edit_cursor > cb->m_text_len)
		cb->m_edit_cursor = cb->m_text_len;
} /* End of 'cbox_move_edit_cursor' function */

/* Add charater to edit box */
void cbox_add_ch( combobox_t *cb, char ch )
{
	if (cb == NULL)
		return;

	/* Add character */
	memmove(&cb->m_text[cb->m_edit_cursor + 1], &cb->m_text[cb->m_edit_cursor],
			(cb->m_text_len - cb->m_edit_cursor + 1));
	cb->m_text[cb->m_edit_cursor] = ch;
	cb->m_edit_cursor ++;
	cb->m_text_len ++;
	cb->m_changed = TRUE;

	/* Update list box */
	cbox_edit2list(cb);
} /* End of 'cbox_add_ch' function */

/* Delete character form edit box */
void cbox_del_ch( combobox_t *cb, bool_t before_cursor )
{
	int i;
	
	if (cb == NULL)
		return;

	/* Delete character */
	i = before_cursor ? cb->m_edit_cursor - 1 : cb->m_edit_cursor;
	if (i < 0 || i >= cb->m_text_len)
		return;
	memmove(&cb->m_text[i], &cb->m_text[i + 1], cb->m_text_len - i);
	cb->m_text_len --;
	cb->m_changed = TRUE;

	/* Move cursor */
	if (before_cursor)
		cb->m_edit_cursor --;

	/* Update list box */
	cbox_edit2list(cb);
} /* End of 'cbox_del_ch' function */

/* Add string to list */
void cbox_list_add( combobox_t *cb, char *str )
{
	if (cb == NULL)
		return;

	if (cb->m_list == NULL)
		cb->m_list = (char **)malloc(sizeof(char *));
	else
		cb->m_list = (char **)realloc(cb->m_list, 
				sizeof(char *) * (cb->m_list_size + 1));
	cb->m_list[cb->m_list_size ++] = strdup(str);
} /* End of 'cbox_list_add' function */

/* Move list box cursor */
void cbox_move_list_cursor( combobox_t *cb, bool_t rel, int pos, bool_t expand,
	   							bool_t change_edit )
{
	if (cb == NULL || !cb->m_list_size)
		return;

	/* Expand */
	if (expand && !cb->m_expanded)
		cb->m_expanded = TRUE;

	/* Remove cursor */
	if (pos == -1 && !rel)
	{
		cb->m_list_cursor = -1;
		cb->m_scrolled = 0;
		return;
	}

	/* Update cursor position */
	if (rel)
		cb->m_list_cursor += pos;
	else 
		cb->m_list_cursor = pos;
	if (cb->m_list_cursor < 0)
		cb->m_list_cursor = 0;
	else if (cb->m_list_cursor >= cb->m_list_size)
		cb->m_list_cursor = cb->m_list_size - 1;

	/* Update scrolling */
	if (cb->m_list_cursor < cb->m_scrolled)
		cb->m_scrolled = cb->m_list_cursor;
	else if (cb->m_list_cursor >= cb->m_scrolled + cb->m_list_height)
		cb->m_scrolled = (rel) ? cb->m_list_cursor - cb->m_list_height + 1:
			cb->m_list_cursor;
	cb->m_changed = TRUE;

	/* Update edit box */
	if (change_edit)
	{
		strcpy(cb->m_text, cb->m_list[cb->m_list_cursor]);
		cb->m_text_len = strlen(cb->m_text);
		cb->m_edit_cursor = cb->m_text_len;
	}
} /* End of 'cbox_move_list_cursor' function */

/* Update list box using text from edit box */
void cbox_edit2list( combobox_t *cb )
{
	int i;

	if (cb == NULL)
		return;

	/* If there is no text make no selection */
	if (!(*(cb->m_text)))
		i = -1;
	else
	{
		/* Search list box for item with this text */
		for ( i = 0; i < cb->m_list_size; i ++ )
			if (!strncmp(cb->m_list[i], cb->m_text, cb->m_text_len))
				break;
	}

	/* Select this item */
	cbox_move_list_cursor(cb, FALSE, (i < cb->m_list_size) ? i : -1, FALSE, 
			FALSE);
} /* End of 'cbox_edit2list' function */

/* Mouse left button click handler */
void cbox_handle_left_click( wnd_t *wnd, dword data )
{
	combobox_t *box = (combobox_t *)wnd;
	
	if (wnd == NULL)
		return;

	/* Call base handler for a dialog item */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* If combo box is not expanded - expand it */
	if (!box->m_expanded)
		box->m_expanded = TRUE;
	/* Else - set cursor to the respective position */
	else
		cbox_move_list_cursor(box, FALSE, 
				WND_MOUSE_Y(data) + box->m_scrolled - 1, TRUE, TRUE);
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'cbox_handle_left_click' function */

/* Mouse middle button click handler */
void cbox_handle_middle_click( wnd_t *wnd, dword data )
{
	combobox_t *box = (combobox_t *)wnd;
	
	if (wnd == NULL)
		return;

	/* Call base handler for a dialog item */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* If list box is not expanded - expand it */
	if (!box->m_expanded)
		box->m_expanded = TRUE;
	/* Else - set cursor to the respective position and centrize view */
	else
	{
		cbox_move_list_cursor(box, FALSE, WND_MOUSE_Y(data) + 
				box->m_scrolled - 1, TRUE, TRUE);
		box->m_scrolled = box->m_list_cursor - box->m_list_height / 2;
		if (box->m_scrolled < 0)
			box->m_scrolled = 0;
		else if (box->m_scrolled + box->m_list_height > box->m_list_size)
			box->m_scrolled = box->m_list_size - box->m_list_height;
	}
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'cbox_handle_middle_click' function */

/* Focus change handler */
void cbox_handle_focus( wnd_t *wnd, dword data )
{
	/* Unexpand box */
	((combobox_t *)wnd)->m_expanded = FALSE;

	/* Call base handler */
	wnd_handle_ch_focus(wnd, data);
} /* End of 'cbox_handle_focus' function */

/* 'WND_MSG_MOUSE_OUTSIDE_FOCUS' message handler */
void cbox_handle_mouse_outside( wnd_t *wnd, dword data )
{
	/* Unexpand */
	((combobox_t *)wnd)->m_expanded = FALSE;
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'cbox_handle_mouse_outside' function */

/* Mouse left button double click handler */
void cbox_handle_dbl_click( wnd_t *wnd, dword data )
{
	combobox_t *box = (combobox_t *)wnd;
	
	if (wnd == NULL)
		return;

	/* Call base handler for a dialog item */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* If combo box is not expanded - expand it */
	if (!box->m_expanded)
		box->m_expanded = TRUE;
	/* Else - set cursor to the respective position */
	else
	{
		cbox_move_list_cursor(box, FALSE, 
				WND_MOUSE_Y(data) + box->m_scrolled - 1, TRUE, TRUE);
		box->m_expanded = FALSE;
	}
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'cbox_handle_dbl_click' function */

/* End of 'combobox.c' file */

