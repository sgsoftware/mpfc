/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : editbox.c
 * PURPOSE     : SG MPFC. Edit box functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.08.2003
 * NOTE        : Module prefix 'ebox'.
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
#include "editbox.h"
#include "error.h"
#include "history.h"
#include "window.h"
#include "util.h"

/* Create a new edit box */
editbox_t *ebox_new( wnd_t *parent, int x, int y, int width, 
						int height,	int max_len, char *label, char *text )
{
	editbox_t *wnd;

	/* Allocate memory for edit box object */
	wnd = (editbox_t *)malloc(sizeof(editbox_t));
	if (wnd == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize edit box fields */
	if (!ebox_init(wnd, parent, x, y, width, height, max_len, label, text))
	{
		free(wnd);
		return NULL;
	}
	
	return wnd;
} /* End of 'ebox_new' function */

/* Initialize edit box */
bool_t ebox_init( editbox_t *wnd, wnd_t *parent, int x, int y, int width, 
					int height,	int max_len, char *label, char *text )
{
	/* Create common window part */
	if (!wnd_init(&wnd->m_wnd, parent, x, y, width, height))
	{
		return FALSE;
	}

	/* Register message handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, ebox_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, ebox_handle_key);

	/* Set editbox-specific fields */
	strcpy(wnd->m_label, label);
	strcpy(wnd->m_text, text);
	wnd->m_cursor = wnd->m_len = strlen(wnd->m_text);
	wnd->m_last_key = 0;
	wnd->m_max_len = max_len;
	wnd->m_scrolled = 0;
	wnd->m_hist_list = NULL;
	wnd->m_changed = FALSE;
	strcpy(wnd->m_text_before_hist, wnd->m_text);
	((wnd_t *)wnd)->m_wnd_destroy = ebox_destroy;
	WND_OBJ(wnd)->m_flags |= (WND_ITEM | WND_INITIALIZED);
	return TRUE;
} /* End of 'ebox_init' function */

/* Destroy edit box */
void ebox_destroy( wnd_t *wnd )
{
	/* Destroy window */
	wnd_destroy_func(wnd);
} /* End of 'ebox_destroy' function */

/* Edit box display function */
void ebox_display( wnd_t *wnd, dword data )
{
	editbox_t *box = (editbox_t *)wnd;

	/* Print label */
	wnd_move(wnd, 0, 0);
	col_set_color(wnd, COL_EL_DLG_ITEM_TITLE);
	wnd_printf(wnd, "%s", box->m_label);

	/* Print edit box text */
	col_set_color(wnd, COL_EL_DLG_ITEM_CONTENT);
	wnd_printf(wnd, "%s\n", &box->m_text[box->m_scrolled]);
	col_set_color(wnd, COL_EL_DEFAULT);

	/* Move cursor to respective place */
	wnd_move(wnd, strlen(box->m_label) + box->m_cursor - box->m_scrolled, 0);
} /* End of 'ebox_display' function */

/* Edit box key handler function */
void ebox_handle_key( wnd_t *wnd, dword data )
{
	editbox_t *box = (editbox_t *)wnd;
	int key = (int)data;
	
	/* Remember last pressed key */
	box->m_last_key = key;

	/* Add character to text if it is printable */
	if (key >= ' ' && key < 256)
		ebox_add(box, key);
	/* Move cursor */
	else if (key == KEY_RIGHT)
		ebox_move(box, TRUE, 1);
	else if (key == KEY_LEFT)
		ebox_move(box, TRUE, -1);
	else if (key == KEY_HOME)
		ebox_move(box, FALSE, 0);
	else if (key == KEY_END)
		ebox_move(box, FALSE, box->m_len);
	/* History stuff */
	else if (key == KEY_UP)
		ebox_hist_move(box, TRUE);
	else if (key == KEY_DOWN)
		ebox_hist_move(box, FALSE);
	/* Delete character */
	else if (key == KEY_BACKSPACE)
		ebox_del(box, box->m_cursor - 1);
	else if (key == KEY_DC)
		ebox_del(box, box->m_cursor);
	/* Common dialog box item actions */
	else DLG_ITEM_HANDLE_COMMON(wnd, key)
	else if (key == '\n' || key == 27)
	{
		/* Save content to history list */
		ebox_hist_save(box, key);
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
	}
} /* End of 'ebox_handle_key' function */

/* Add a character to edit box */
void ebox_add( editbox_t *box, char c )
{
	if (box->m_len >= box->m_max_len)
		return;
	
	memmove(&box->m_text[box->m_cursor + 1], 
			&box->m_text[box->m_cursor],
			box->m_len - box->m_cursor + 1);
	box->m_text[box->m_cursor] = c;
	box->m_len ++;
	box->m_changed = TRUE;
	ebox_set_cursor(box, box->m_cursor + 1);
} /* End of 'ebox_add' function */

/* Delete a character */
void ebox_del( editbox_t *box, int index )
{
	if (index >= 0 && index < box->m_len)
	{
		memmove(&box->m_text[index], &box->m_text[index + 1], 
				box->m_len - index);
		ebox_set_cursor(box, index);
		box->m_len --;
		box->m_changed = TRUE;
	}
} /* End of 'ebox_del' function */

/* Move cursor */
void ebox_move( editbox_t *box, bool_t rel, int offset )
{
	ebox_set_cursor(box, box->m_cursor * rel + offset);
} /* End of 'ebox_move' function */

/* Set new cursor position */
void ebox_set_cursor( editbox_t *box, int new_pos )
{
	int old_cur = box->m_cursor, page_size;
	
	/* Set cursor position */
	box->m_cursor = new_pos;
	if (box->m_cursor < 0)
		box->m_cursor = 0;
	else if (box->m_cursor > box->m_len)
		box->m_cursor = box->m_len;

	/* Handle scrolling */
	page_size = box->m_wnd.m_width - strlen(box->m_label);
	while (box->m_cursor < box->m_scrolled + 1)
		box->m_scrolled -= 5;
	while (box->m_cursor >= box->m_scrolled + page_size)
		box->m_scrolled ++;
	if (box->m_scrolled < 0)
		box->m_scrolled = 0;
	else if (box->m_scrolled >= box->m_len)
		box->m_scrolled = box->m_len - 1;
} /* End of 'ebox_set_cursor' function */

/* Handle history moving */
void ebox_hist_move( editbox_t *box, bool_t up )
{
	hist_list_t *l;
	
	if ((l = box->m_hist_list) != NULL && l->m_tail != NULL)
	{
		if (up)
		{
			if (l->m_cur == NULL)
			{
				l->m_cur = l->m_tail;
				strcpy(box->m_text_before_hist, box->m_text);
			}
			else if (l->m_cur->m_prev != NULL)
				l->m_cur = l->m_cur->m_prev;
			else
				return;
		}
		else
		{
			if (l->m_cur == NULL)
				return;
			else
				l->m_cur = l->m_cur->m_next;
		}
		if (l->m_cur != NULL)
			strcpy(box->m_text, l->m_cur->m_text);
		else if (!up)
			strcpy(box->m_text, box->m_text_before_hist);
		box->m_len = strlen(box->m_text);
		box->m_changed = FALSE;
		ebox_move(box, FALSE, strlen(box->m_text));
	}
} /* End of 'ebox_hist_move' function */

/* Save history information */
void ebox_hist_save( editbox_t *box, int key )
{
	if (key == '\n' && box->m_hist_list != NULL && box->m_changed &&
			strlen(box->m_text))
		hist_add_item(box->m_hist_list, box->m_text);
	if (box->m_hist_list != NULL)
		box->m_hist_list->m_cur = NULL;
} /* End of 'ebox_hist_save' function */

/* Set edit box text */
void ebox_set_text( editbox_t *box, char *text )
{
	if (box == NULL || text == NULL)
		return;

	strcpy(box->m_text, text);
	box->m_len = strlen(box->m_text);
	ebox_set_cursor(box, box->m_len);
	wnd_send_msg(box, WND_MSG_DISPLAY, 0);
} /* End of 'ebox_set_text' function */

/* End of 'editbox.c' file */

