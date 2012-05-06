/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for views list control functions.
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

#ifndef __SG_MPFC_WND_VIEWS_H__
#define __SG_MPFC_WND_VIEWS_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Views list type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;
} views_t;

/* Convert window object to views list type */
#define VIEWS_OBJ(wnd)	((views_t *)wnd)

/* Create a new views list */
views_t *views_new( wnd_t *parent, char *title );

/* Views list constructor */
bool_t views_construct( views_t *v, wnd_t *parent, char *title ); 

/* Get the desired size */
void views_get_desired_size( dlgitem_t *di, int *width, int *height );

/* Set position */
void views_set_pos( dlgitem_t *di, int x, int y, int width, int height );

/*
 * Class functions
 */

/* Initialize views class */
wnd_class_t *views_class_init( wnd_global_data_t *global );

#endif

/* End of 'wnd_views.h' file */

