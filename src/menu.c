/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : menu.c
 * PURPOSE     : SG MPFC. Menu functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 23.04.2003
 * NOTE        : Module prefix 'menu'.
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
#include "types.h"
#include "error.h"
#include "menu.h"
#include "window.h"

/* Check that menu object is valid */
#define MENU_ASSERT(menu) \
	if ((menu) == NULL) return
#define MENU_ASSERT_RET(menu, ret) \
	if ((menu) == NULL) return (ret)

/* Create a new menu */
menu_t *menu_new( wnd_t *parent, int x, int y, int w, int h )
{
	menu_t *menu;

	/* Allocate memory for menu */
	menu = (menu_t *)malloc(sizeof(menu_t));
	if (menu == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize menu */
	if (!menu_init(menu, parent, x, y, w, h))
	{
		free(menu);
		return NULL;
	}

	return menu;
} /* End of 'menu_new' function */

/* Initialize menu */
bool_t menu_init( menu_t *menu, wnd_t *parent, int x, int y, int w, int h )
{
	/* Initialize window */
	if (!wnd_init(&menu->m_wnd, parent, x, y, w, h))
		return FALSE;

	/* Register handlers */
	wnd_register_handler(menu, WND_MSG_DISPLAY, menu_display);
	wnd_register_handler(menu, WND_MSG_KEYDOWN, menu_handle_key);
	WND_OBJ(menu)->m_wnd_destroy = menu_destroy;

	/* Set menu-specific fields */
	menu->m_num_items = 0;
	menu->m_items = NULL;
	menu->m_cursor = -1;
	WND_OBJ(menu)->m_flags |= (WND_INITIALIZED);
	return TRUE;
} /* End of 'menu_init' function */

/* Destroy menu */
void menu_destroy( wnd_t *wnd )
{
	menu_t *menu = (menu_t *)wnd;

	MENU_ASSERT(menu);
	
	if (menu->m_items != NULL)
		free(menu->m_items);
	wnd_destroy_func(wnd);
} /* End of 'menu_destroy' function */

/* Add an item to menu */
bool_t menu_add_item( menu_t *menu, char *name, int id, menu_handler handler )
{
	MENU_ASSERT_RET(menu, FALSE);

	/* Allocate memory for items list */
	if (!menu->m_num_items)
		menu->m_items = (struct tag_menu_item_t *)malloc(
				sizeof(*menu->m_items));
	else
		menu->m_items = (struct tag_menu_item_t *)realloc(menu->m_items,
				sizeof(*menu->m_items) * (menu->m_num_items + 1));
	if (menu->m_items == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return FALSE;
	}

	/* Update cursor position */
	if (menu->m_cursor == -1)
		menu->m_cursor = 0;

	/* Store item information */
	strcpy(menu->m_items[menu->m_num_items].m_name, name);
	menu->m_items[menu->m_num_items].m_id = id;
	menu->m_items[menu->m_num_items].m_handler = handler;
	menu->m_num_items ++;
	return TRUE;
} /* End of 'menu_add_item' function */

/* Menu display function */
void menu_display( wnd_t *wnd, dword data )
{
	menu_t *menu = (menu_t *)wnd;
	int i;

	MENU_ASSERT(menu);

	wnd_move(wnd, 0, 0);
	for ( i = 0; i < menu->m_num_items; i ++ )
	{
		if (i == menu->m_cursor)
			wnd_set_attrib(wnd, A_REVERSE);

		wnd_printf(wnd, "%s\n", menu->m_items[i].m_name);

		if (i == menu->m_cursor)
			wnd_set_attrib(wnd, A_NORMAL);
	}
	for ( ; i < wnd->m_height; i ++ )
		wnd_printf(wnd, "\n");
	if (menu->m_cursor != -1)
		wnd_move(wnd, 0, menu->m_cursor);
	else
		wnd_move(wnd_root, wnd_root->m_width - 1, wnd_root->m_height - 1);
} /* End of 'menu_display' function */

/* Menu key handler function */
void menu_handle_key( wnd_t *wnd, dword data )
{
	menu_t *menu = (menu_t *)wnd;
	int key = (int)data;

	MENU_ASSERT(menu);

	switch (key)
	{
	/* Move cursor */
	case KEY_UP:
	case KEY_DOWN:
		menu->m_cursor += ((2 * (key == KEY_UP) - 1) + menu->m_num_items);
		menu->m_cursor %= menu->m_num_items;
		break;
	case KEY_HOME:
		menu->m_cursor = (menu->m_num_items) ? 0 : -1;
		break;
	case KEY_END:
		menu->m_cursor = menu->m_num_items - 1;
		break;

	/* Select item */
	case '\n':
		if (menu->m_cursor >= 0 && 
				menu->m_items[menu->m_cursor].m_handler != NULL)
			menu->m_items[menu->m_cursor].m_handler(menu, 
					menu->m_items[menu->m_cursor].m_id);
		break;
		
	/* Exit */
	case 27:
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;
	}
} /* End of 'menu_handle_key' function */

/* End of 'menu.c' file */

