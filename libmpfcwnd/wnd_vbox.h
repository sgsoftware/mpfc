/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_vbox.h
 * PURPOSE     : SG MPFC Window Library. Interface for vertical
 *               box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 16.08.2004
 * NOTE        : Module prefix 'vbox'.
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

#ifndef __SG_MPFC_WND_VBOX_H__
#define __SG_MPFC_WND_VBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Vertical box type */
typedef struct 
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Distance between children */
	int m_dist;
} vbox_t;

/* Convert window object to vertical box type */
#define VBOX_OBJ(wnd)	((vbox_t *)wnd)

/* Create a new vertical box */
vbox_t *vbox_new( wnd_t *parent, char *title, int dist );

/* Vertical box constructor */
bool_t vbox_construct( vbox_t *vbox, wnd_t *parent, char *title, int dist );

/* Get the desired size */
void vbox_get_desired_size( dlgitem_t *di, int *width, int *height );

/* Set position */
void vbox_set_pos( dlgitem_t *di, int x, int y, int width, int height );

/*
 * Class functions
 */

/* Initialize vbox class */
wnd_class_t *vbox_class_init( wnd_global_data_t *global );

#endif

/* End of 'wnd_vbox.h' file */

