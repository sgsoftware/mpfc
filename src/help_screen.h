/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : help_screen.h
 * PURPOSE     : SG MPFC. Interface for help screen functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.04.2003
 * NOTE        : Module prefix 'help'.
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

#ifndef __SG_MPFC_HELP_SCREEN_H__
#define __SG_MPFC_HELP_SCREEN_H__

#include "types.h"
#include "window.h"

/* The maximal number of items in help screen */
#define HELP_MAX_ITEMS 40

/* Help screen window type */
typedef struct tag_help_screen_t
{
	/* Window part */
	wnd_t m_wnd;

	/* Current screen number */
	int m_screen;

	/* Number of screens */
	int m_num_screens;

	/* Screen height */
	int m_screen_size;

	/* Items */
	char m_items[HELP_MAX_ITEMS][80];
	int m_num_items;
} help_screen_t;

/* Create new help screen */
help_screen_t *help_new( wnd_t *parent, int x, int y, int w, int h );

/* Initialize help screen */
bool_t help_init( help_screen_t *help, wnd_t *parent, int x, int y, 
	   int w, int h );

/* Destroy help screen */
void help_free( wnd_t *wnd );

/* Handle display message */
void help_display( wnd_t *wnd, dword data );

/* Handle key message */
void help_handle_key( wnd_t *wnd, dword data );

/* Add item */
void help_add( help_screen_t *h, char *name );

#endif

/* End of 'help_screen.h' file */

