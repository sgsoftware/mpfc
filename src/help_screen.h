/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : help_screen.h
 * PURPOSE     : SG MPFC. Interface for help screen functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.02.2004
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
	char **m_items;
	int m_num_items;

	/* Title */
	char *m_title;

	/* Help screen type */
	int m_type;
} help_screen_t;

/* Help screen types */
#define HELP_PLAYER 0
#define HELP_BROWSER 1
#define HELP_EQWND 2

/* Create new help screen */
help_screen_t *help_new( wnd_t *parent, int type, int x, int y, int w, int h );

/* Initialize help screen */
bool_t help_init( help_screen_t *help, int type, wnd_t *parent, int x, int y, 
	   int w, int h );

/* Destroy help screen */
void help_free( wnd_t *wnd );

/* Handle display message */
void help_display( wnd_t *wnd, dword data );

/* Handle key message */
void help_handle_key( wnd_t *wnd, dword data );

/* Add item */
void help_add( help_screen_t *h, char *name );

/* Initialize help screen in player mode */
void help_init_player( help_screen_t *h );

/* Initialize help screen in browser mode */
void help_init_browser( help_screen_t *h );

/* Initialize help screen in equalizer window mode */
void help_init_eqwnd( help_screen_t *h );

#endif

/* End of 'help_screen.h' file */

