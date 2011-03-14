/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. List box functions implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "wnd_dlgitem.h"
#include "wnd_listbox.h"

/* Create a new list box */
listbox_t *listbox_new( wnd_t *parent, char *title, char *id, char letter, 
		listbox_flags_t flags, int width, int height )
{
	listbox_t *lb;

	/* Allocate memory */
	lb = (listbox_t *)malloc(sizeof(*lb));
	if (lb == NULL)
		return NULL;
	memset(lb, 0, sizeof(*lb));
	WND_OBJ(lb)->m_class = listbox_class_init(WND_GLOBAL(parent));

	/* Construct list box */
	if (!listbox_construct(lb, parent, title, id, letter, flags, width, height))
	{
		free(lb);
		return NULL;
	}
	WND_FLAGS(lb) |= WND_FLAG_INITIALIZED;
	return lb;
} /* End of 'listbox_new' function */

/* List box constructor */
bool_t listbox_construct( listbox_t *lb, wnd_t *parent, char *title, char *id, 
		char letter, listbox_flags_t flags, int width, int height )
{
	/* Construct dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(lb), parent, title, id,
				listbox_get_desired_size, NULL, letter, DLGITEM_BORDER))
		return FALSE;

	/* Set message handlers */
	wnd_msg_add_handler(WND_OBJ(lb), "display", listbox_on_display);
	wnd_msg_add_handler(WND_OBJ(lb), "action", listbox_on_action);
	wnd_msg_add_handler(WND_OBJ(lb), "mouse_ldown", listbox_on_mouse);
	wnd_msg_add_handler(WND_OBJ(lb), "destructor", listbox_destructor);

	/* Set list box data */
	lb->m_flags = flags;
	lb->m_width = width;
	lb->m_height = height;
	WND_OBJ(lb)->m_cursor_hidden = TRUE;
	return TRUE;
} /* End of 'listbox_construct' function */

/* List box destructor */
void listbox_destructor( wnd_t *wnd )
{
	listbox_t *lb = LISTBOX_OBJ(wnd);
	int i;

	/* Free items list */
	if (lb->m_list != NULL)
	{
		for ( i = 0; i < lb->m_list_size; i ++ )
			free(lb->m_list[i].m_name);
		free(lb->m_list);
	}
} /* End of 'listbox_destructor' function */

/* Add an item */
int listbox_add( listbox_t *lb, char *item, void *data )
{
	int pos;
	assert(lb);
	lb->m_list = (struct listbox_item_t *)realloc(lb->m_list, 
			sizeof(struct listbox_item_t) * (lb->m_list_size + 1));
	if (lb->m_list == NULL)
		return -1;
	pos = lb->m_list_size;
	lb->m_list[pos].m_name = strdup(item);
	lb->m_list[pos].m_data = data;
	lb->m_list_size ++;
	wnd_invalidate(WND_OBJ(lb));
	return pos;
} /* End of 'listbox_add' function */

/* Move cursor */
void listbox_move( listbox_t *lb, int pos, bool_t rel )
{
	assert(lb);

	/* Set cursor */
	lb->m_cursor = (rel ? lb->m_cursor + pos : pos);
	if (lb->m_cursor < 0)
		lb->m_cursor = 0;
	else if (lb->m_cursor >= lb->m_list_size)
		lb->m_cursor = lb->m_list_size - 1;

	/* Scroll */
	if (lb->m_cursor < lb->m_scroll)
		lb->m_scroll = lb->m_cursor;
	else if (lb->m_cursor >= lb->m_scroll + lb->m_height)
		lb->m_scroll = lb->m_cursor - lb->m_height + 1;
	if (lb->m_scroll >= lb->m_list_size - lb->m_height)
		lb->m_scroll = lb->m_list_size - lb->m_height - 1;
	if (lb->m_scroll < 0)
		lb->m_scroll = 0;
	wnd_invalidate(WND_OBJ(lb));

	/* Send 'changed' message */
	wnd_msg_send(WND_OBJ(lb), "changed",
			listbox_msg_changed_new(lb->m_cursor));
} /* End of 'listbox_move' function */

/* Select an item */
void listbox_sel_item( listbox_t *lb, int pos )
{
	if (!(lb->m_flags & LISTBOX_SELECTABLE))
		return;
	lb->m_selected = pos;
	wnd_invalidate(WND_OBJ(lb));
	wnd_msg_send(WND_OBJ(lb), "selection_changed", 
			listbox_msg_sel_changed_new(pos));
} /* End of 'listbox_sel_item' function */

/* 'display' message handler */
wnd_msg_retcode_t listbox_on_display( wnd_t *wnd )
{
	int i, j;
	listbox_t *lb = LISTBOX_OBJ(wnd);
	assert(lb);

	for ( i = lb->m_scroll, j = 0; i < lb->m_list_size && j < lb->m_height; 
			i ++, j ++ )
	{
		wnd_move(wnd, 0, 0, j);
		wnd_push_state(wnd, WND_STATE_COLOR);
		if (i == lb->m_cursor)
		{
			wnd_apply_style(wnd, WND_FOCUS(wnd) == wnd ? 
					"focus-list-style" : "list-style");
		}
		if (lb->m_flags & LISTBOX_SELECTABLE)
			wnd_printf(wnd, 0, 0, "[%c] ", lb->m_selected == i ? 'X' : ' ');
		wnd_putstring(wnd, 0, 0, lb->m_list[i].m_name);
		wnd_pop_state(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'listbox_on_display' function */

/* 'action' message handler */
wnd_msg_retcode_t listbox_on_action( wnd_t *wnd, char *action )
{
	listbox_t *lb = LISTBOX_OBJ(wnd);
	assert(lb);

	if (!strcasecmp(action, "move_down"))
	{
		listbox_move(lb, 1, TRUE);
	}
	else if (!strcasecmp(action, "move_up"))
	{
		listbox_move(lb, -1, TRUE);
	}
	else if (!strcasecmp(action, "screen_down"))
	{
		listbox_move(lb, lb->m_height, TRUE);
	}
	else if (!strcasecmp(action, "screen_up"))
	{
		listbox_move(lb, -lb->m_height, TRUE);
	}
	else if (!strcasecmp(action, "move_to_start"))
	{
		listbox_move(lb, 0, FALSE);
	}
	else if (!strcasecmp(action, "move_to_end"))
	{
		listbox_move(lb, lb->m_list_size - 1, FALSE);
	}
	else if (!strcasecmp(action, "select_item"))
	{
		listbox_sel_item(lb, lb->m_selected == lb->m_cursor ? -1 :
				lb->m_cursor);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'listbox_on_action' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t listbox_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	listbox_t *lb = LISTBOX_OBJ(wnd);

	/* Select item */
	listbox_move(lb, lb->m_scroll + y, FALSE);
	return WND_MSG_RETCODE_OK;
} /* End of 'listbox_on_mouse' function */

/* Get list box desired size */
void listbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	listbox_t *lb = LISTBOX_OBJ(di);
	assert(lb);
	(*width) = lb->m_width + 2;
	(*height) = lb->m_height + 2;
} /* End of 'listbox_get_desired_size' function */

/* Create list box class */
wnd_class_t *listbox_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "listbox", dlgitem_class_init(global), 
			listbox_get_msg_info, listbox_free_handlers,
			listbox_class_set_default_styles);
} /* End of 'listbox_class_init' function */

/* Get message information */
wnd_msg_handler_t **listbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "changed"))
	{
		if (callback != NULL)
			(*callback) = listbox_callback_changed;
		return &LISTBOX_OBJ(wnd)->m_on_changed;
	}
	else if (!strcmp(msg_name, "selection_changed"))
	{
		if (callback != NULL)
			(*callback) = listbox_callback_changed;
		return &LISTBOX_OBJ(wnd)->m_on_selection_changed;
	}
	return NULL;
} /* End of 'listbox_get_msg_info' function */

/* Free message handlers */
void listbox_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(LISTBOX_OBJ(wnd)->m_on_selection_changed);
	wnd_msg_free_handlers(LISTBOX_OBJ(wnd)->m_on_changed);
} /* End of 'listbox_free_handlers' function */

/* Set list box class default styles */
void listbox_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "list-style", "white:green");
	cfg_set_var(list, "focus-list-style", "white:blue");

	/* Set kbinds */
	cfg_set_var(list, "kbind.move_down", "<Down>;<Ctrl-n>;j");
	cfg_set_var(list, "kbind.move_up", "<Up>;<Ctrl-p>;k");
	cfg_set_var(list, "kbind.screen_down", "<PageDown>;<Ctrl-v>;d");
	cfg_set_var(list, "kbind.screen_up", "<PageUp>;<Alt-v>;u");
	cfg_set_var(list, "kbind.move_to_start", "<Ctrl-a>;<Home>;g");
	cfg_set_var(list, "kbind.move_to_end", "<Ctrl-e>;<End>;G");
	cfg_set_var(list, "kbind.select_item", "<Space>");
} /* End of 'listbox_class_set_default_styles' function */

/* Create item selection message data */
wnd_msg_data_t listbox_msg_changed_new( int item )
{
	wnd_msg_data_t msg_data;
	listbox_msg_changed_t *data;

	data = (listbox_msg_changed_t *)malloc(sizeof(*data));
	data->m_item = item;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'listbox_msg_changed_new' function */

/* Callback function */
wnd_msg_retcode_t listbox_callback_changed( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	listbox_msg_changed_t *d = (listbox_msg_changed_t *)(msg_data->m_data);
	return LISTBOX_MSG_CHANGED_HANDLER(handler)(wnd, d->m_item);
} /* End of 'listbox_callback_changed' function */

/* End of 'wnd_listbox.c' file */

