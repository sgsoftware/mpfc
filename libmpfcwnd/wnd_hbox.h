/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_hbox.h
 * PURPOSE     : SG MPFC Window Library. Interface for horizontal
 *               box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 16.08.2004
 * NOTE        : Module prefix 'hbox'.
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

#ifndef __SG_MPFC_WND_HBOX_H__
#define __SG_MPFC_WND_HBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Horizontal box type */
typedef struct 
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Distance between children */
	int m_dist;
} hbox_t;

/* Convert window object to vertical box type */
#define HBOX_OBJ(wnd)	((hbox_t *)wnd)

/* Create a new horizontal box */
hbox_t *hbox_new( wnd_t *parent, char *title, int dist );

/* Horizontal box constructor */
bool_t hbox_construct( hbox_t *hbox, wnd_t *parent, char *title, int dist );

/* Get the desired size */
void hbox_get_desired_size( dlgitem_t *di, int *width, int *height );

/* Set position */
void hbox_set_pos( dlgitem_t *di, int x, int y, int width, int height );

/*
 * Class functions
 */

/* Initialize hbox class */
wnd_class_t *hbox_class_init( wnd_global_data_t *global );

#endif

/* End of 'wnd_hbox.h' file */


