/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : listbox.c
 * PURPOSE     : SG MPFC. List box functions implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "colors.h"
#include "dlgbox.h"
#include "error.h"
#include "listbox.h"
#include "window.h"

/* Create a new list box */
listbox_t *lbox_new( wnd_t *parent, int x, int y, int width, 
						int height,	char *label )
{
	listbox_t *wnd;

	/* Allocate memory for list box object */
	wnd = (listbox_t *)malloc(sizeof(listbox_t));
	if (wnd == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize list box fields */
	if (!lbox_init(wnd, parent, x, y, width, height, label))
	{
		free(wnd);
		return NULL;
	}
	
	return wnd;
} /* End of 'lbox_new' function */

/* Initialize list box */
bool_t lbox_init( listbox_t *wnd, wnd_t *parent, int x, int y, int width, 
					int height,	char *label )
{
	/* Create common window part */
	if (!wnd_init(&wnd->m_wnd, parent, x, y, width, height))
	{
		return FALSE;
	}

	/* Register message handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, lbox_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, lbox_handle_key);
	wnd_register_handler(wnd, WND_MSG_CHANGE_FOCUS, lbox_handle_focus);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_CLICK, lbox_handle_left_click);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_DOUBLE, lbox_handle_dbl_click);
	wnd_register_handler(wnd, WND_MSG_MOUSE_MIDDLE_CLICK, 
			lbox_handle_middle_click);

	/* Set listbox-specific fields */
	strcpy(wnd->m_label, label);
	wnd->m_list = NULL;
	wnd->m_size = 0;
	wnd->m_last_key = 0;
	wnd->m_cursor = -1;
	wnd->m_scrolled = 0;
	wnd->m_expanded = FALSE;
	wnd->m_minimalizing = TRUE;
	((wnd_t *)wnd)->m_wnd_destroy = lbox_destroy;
	WND_OBJ(wnd)->m_flags |= (WND_ITEM | WND_INITIALIZED);
	return TRUE;
} /* End of 'lbox_init' function */

/* Destroy list box */
void lbox_destroy( wnd_t *wnd )
{
	listbox_t *lbox = (listbox_t *)wnd;
	
	if (lbox != NULL)
	{
		if (lbox->m_list != NULL)
			free(lbox->m_list);
	}
	wnd_destroy_func(wnd);
} /* End of 'lbox_destroy' function */

/* List box display function */
void lbox_display( wnd_t *wnd, dword data )
{
	listbox_t *l = (listbox_t *)wnd;
	
	if (l == NULL)
		return;

	/* Print label */
	wnd_move(wnd, 0, 0);
	col_set_color(wnd, COL_EL_DLG_ITEM_TITLE);
	wnd_printf(wnd, "%s", l->m_label);

	/* Print text */
	if (l->m_expanded)
	{
		int i, y;

		for ( i = l->m_scrolled, y = 0; y < wnd->m_height && i < l->m_size;
		   		i ++, y ++ )
		{
			if (i == l->m_cursor)
				col_set_color(wnd, wnd_is_focused(wnd) ? 
						COL_EL_LBOX_CUR_FOCUSED : COL_EL_LBOX_CUR);
			else
				col_set_color(wnd, COL_EL_DLG_ITEM_CONTENT);
			wnd_move(wnd, strlen(l->m_label), y);
			wnd_printf(wnd, "%s\n", l->m_list[i].m_name);
		}
		wnd_move(wnd, strlen(l->m_label), l->m_cursor - l->m_scrolled);
	}
	else if (l->m_cursor >= 0)
	{
		col_set_color(wnd, COL_EL_DLG_ITEM_CONTENT);
		wnd_printf(wnd, "%s", l->m_list[l->m_cursor].m_name);
		wnd_move(wnd, strlen(l->m_label), 0);
	}
	col_set_color(wnd, COL_EL_DEFAULT);
} /* End of 'lbox_display' function */

/* List box key handler function */
void lbox_handle_key( wnd_t *wnd, dword data )
{
	listbox_t *l = (listbox_t *)wnd;
	int key = (int)data;
	
	if (key == 'j' || key == KEY_DOWN)
		lbox_move_cursor(l, TRUE, 1, TRUE);
	else if (key == 'k' || key == KEY_UP)
		lbox_move_cursor(l, TRUE, -1, TRUE);
	else if (key == 'u' || key == KEY_PPAGE)
		lbox_move_cursor(l, TRUE, -wnd->m_height - 1, TRUE);
	else if (key == 'd' || key == KEY_NPAGE)
		lbox_move_cursor(l, TRUE, wnd->m_height - 1, TRUE);
	else if (key == '0' || key == KEY_HOME)
		lbox_move_cursor(l, FALSE, 0, TRUE);
	else if (key == 'G' || key == KEY_END)
		lbox_move_cursor(l, FALSE, l->m_size - 1, TRUE);
	else DLG_ITEM_HANDLE_COMMON(wnd, key)
} /* End of 'lbox_handle_key' function */

/* Add item to list box */
void lbox_add( listbox_t *lbox, char *name )
{
	if (lbox == NULL)
		return;

	lbox->m_list = (struct tag_lbox_item_t *)realloc(lbox->m_list,
			sizeof(*(lbox->m_list)) * (lbox->m_size + 1));
	lbox->m_list[lbox->m_size ++].m_name = strdup(name);
} /* End of 'lbox_add' function */

/* Move cursor */
void lbox_move_cursor( listbox_t *lbox, bool_t rel, int pos, bool_t expand  )
{
	int new_pos;
	
	if (lbox == NULL)
		return;

	/* Guess when cursor is removed */
	if ((!rel && pos < 0) || (!lbox->m_size))
	{
		lbox->m_cursor = -1;
		lbox->m_scrolled = 0;
		return;
	}

	lbox->m_expanded = expand;
	if (expand && !lbox->m_expanded && lbox->m_minimalizing)
		return;

	/* Calculate new cursor position */
	new_pos = (rel ? lbox->m_cursor : 0) + pos;
	if (new_pos < 0)
		new_pos = 0;
	else if (new_pos >= lbox->m_size)
		new_pos = lbox->m_size - 1;

	/* Set new cursor position and update scrolling value */
	lbox->m_cursor = new_pos;
	if (lbox->m_cursor < lbox->m_scrolled)
		lbox->m_scrolled = lbox->m_cursor;
	else if (lbox->m_cursor >= lbox->m_scrolled + ((wnd_t *)lbox)->m_height)
		lbox->m_scrolled = lbox->m_cursor - ((wnd_t *)lbox)->m_height + 1;

	/* Send notify message to parent */
	wnd_send_msg(((wnd_t *)lbox)->m_parent, WND_MSG_NOTIFY, 
			WND_NOTIFY_DATA(((wnd_t *)lbox)->m_id, LBOX_MOVE));
} /* End of 'lbox_move_cursor' function */

/* Focus change handler */
void lbox_handle_focus( wnd_t *wnd, dword data )
{
	/* Unexpand box */
	if (((listbox_t *)wnd)->m_minimalizing)
		((listbox_t *)wnd)->m_expanded = FALSE;

	/* Call base handler */
	wnd_handle_ch_focus(wnd, data);
} /* End of 'lbox_handle_focus' function */

/* Mouse left button click handler */
void lbox_handle_left_click( wnd_t *wnd, dword data )
{
	listbox_t *box = (listbox_t *)wnd;
	
	if (wnd == NULL)
		return;

	/* Call base handler for a dialog item */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* If list box is not expanded - expand it */
	if (!box->m_expanded)
		box->m_expanded = TRUE;
	/* Else - set cursor to the respective position */
	else
		lbox_move_cursor(box, FALSE, WND_MOUSE_Y(data) + box->m_scrolled, TRUE);
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'lbox_handle_left_click' function */

/* Mouse middle button click handler */
void lbox_handle_middle_click( wnd_t *wnd, dword data )
{
	listbox_t *box = (listbox_t *)wnd;
	
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
		lbox_move_cursor(box, FALSE, WND_MOUSE_Y(data) + box->m_scrolled, 
				TRUE);
		box->m_scrolled = box->m_cursor - WND_HEIGHT(box) / 2;
		if (box->m_scrolled < 0)
			box->m_scrolled = 0;
		else if (box->m_scrolled + WND_HEIGHT(box) > box->m_size)
			box->m_scrolled = box->m_size - WND_HEIGHT(box);
	}
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'lbox_handle_middle_click' function */

/* Mouse left button double click handler */
void lbox_handle_dbl_click( wnd_t *wnd, dword data )
{
	listbox_t *box = (listbox_t *)wnd;
	
	if (wnd == NULL)
		return;

	/* Call base handler for a dialog item */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* If list box is not expanded - expand it */
	if (!box->m_expanded)
		box->m_expanded = TRUE;
	/* Else - set cursor to the respective position */
	else
	{
		lbox_move_cursor(box, FALSE, WND_MOUSE_Y(data) + box->m_scrolled, TRUE);
		if (box->m_minimalizing)
			box->m_expanded = FALSE;
	}
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'lbox_handle_dbl_click' function */

/* End of 'listbox.c' file */

