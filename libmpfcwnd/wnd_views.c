/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Views list control functions implementation.
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
#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_views.h"

/* Create a new views list */
views_t *views_new( wnd_t *parent, char *title )
{
	views_t *v;

	/* Allocate memory */
	v = (views_t *)malloc(sizeof(*v));
	if (v == NULL)
		return NULL;
	memset(v, 0, sizeof(*v));
	WND_OBJ(v)->m_class = views_class_init(WND_GLOBAL(parent));

	/* Call constructor */
	if (!views_construct(v, parent, title))
	{
		free(v);
		return NULL;
	}
	WND_OBJ(v)->m_flags |= WND_FLAG_INITIALIZED;
	return v;
} /* End of 'views_new' function */

/* Views list constructor */
bool_t views_construct( views_t *v, wnd_t *parent, char *title )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(v), parent, title, NULL, 
				views_get_desired_size, views_set_pos, 0, DLGITEM_NOTABSTOP))
		return FALSE;
	return TRUE;
} /* End of 'views_construct' function */

/* Get the desired size */
void views_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	wnd_t *child;

	/* Determine maximal children size */
	(*width) = 0;
	(*height) = 0;
	for ( child = WND_OBJ(di)->m_child; child != NULL; child = child->m_next )
	{
		int dw, dh;
		dlgitem_get_size(DLGITEM_OBJ(child), &dw, &dh);
		if (dw > (*width))
			(*width) = dw;
		if (dh > (*height))
			(*height) = dh;
	}
} /* End of 'views_get_desired_size' function */

/* Set position */
void views_set_pos( dlgitem_t *di, int x, int y, int width, int height )
{
	wnd_t *child;
	for ( child = WND_OBJ(di)->m_child; child != NULL; child = child->m_next )
	{
		dlgitem_set_pos(DLGITEM_OBJ(child), 0, 0, width, height);
	}
} /* End of 'views_set_pos' function */

/* Initialize views class */
wnd_class_t *views_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "views", dlgitem_class_init(global), NULL, 
			NULL, NULL);
} /* End of 'views_class_init' function */

/* End of 'wnd_views.c' file */

