/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : menu.h
 * PURPOSE     : SG Konsamp. Interface for menu functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 14.02.2003
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

#ifndef __SG_KONSAMP_MENU_H__
#define __SG_KONSAMP_MENU_H__

#include "types.h"
#include "window.h"

/* Menu item handler function */
struct tag_menu_t;
typedef void (*menu_handler)( struct tag_menu_t *menu, int item_id );

/* Menu type */
typedef struct tag_menu_t
{
	/* Window object */
	wnd_t m_wnd;

	/* Menu items list */
	int m_num_items;
	struct tag_menu_item_t
	{
		char m_name[80];
		int m_id;
		menu_handler m_handler;
	} *m_items;

	/* Current cursor position */
	int m_cursor;
} menu_t;

/* Create a new menu */
menu_t *menu_new( wnd_t *parent, int x, int y, int w, int h );

/* Initialize menu */
bool menu_init( menu_t *menu, wnd_t *parent, int x, int y, int w, int h );

/* Destroy menu */
void menu_destroy( wnd_t *wnd );

/* Add an item to menu */
bool menu_add_item( menu_t *menu, char *name, int id, menu_handler handler );

/* Menu display function */
int menu_display( wnd_t *wnd, dword data );

/* Menu key handler function */
int menu_handle_key( wnd_t *wnd, dword data );

#endif

/* End of 'menu.h' file */

