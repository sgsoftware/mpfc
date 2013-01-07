/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for help screen functions.
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

#ifndef __SG_MPFC_HELP_SCREEN_H__
#define __SG_MPFC_HELP_SCREEN_H__

#include "types.h"
#include "wnd.h"

/* Help screen window type */
typedef struct 
{
	/* Window part */
	wnd_t m_wnd;

	/* Current screen number */
	int m_screen;

	/* Items */
	char **m_items;
	int m_num_items;

	/* Help screen type */
	int m_type;
} help_screen_t;

/* Help screen types */
#define HELP_PLAYER 0
#define HELP_BROWSER 1

/* Create new help screen */
help_screen_t *help_new( wnd_t *parent, int type );

/* Initialize help screen */
bool_t help_construct( help_screen_t *help, wnd_t *parent, int type );

/* Destructor */
void help_destructor( wnd_t *wnd );

/* Handle display message */
wnd_msg_retcode_t help_on_display( wnd_t *wnd );

/* Handle 'action' message */
wnd_msg_retcode_t help_on_action( wnd_t *wnd, char *action );

/* Add item */
void help_add( help_screen_t *h, char *name );

/* Initialize help screen in player mode */
void help_init_player( help_screen_t *h );

/* Initialize help screen in browser mode */
void help_init_browser( help_screen_t *h );

/* Initialize help screen class */
wnd_class_t *help_class_init( wnd_global_data_t *global );

/* Set help screen class default styles */
void help_class_set_default_styles( cfg_node_t *list );

#endif

/* End of 'help_screen.h' file */

