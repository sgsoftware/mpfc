/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC Window Library. Vertical box functions implementation.
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
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_vbox.h"

/* Create a new vertical box */
vbox_t *vbox_new( wnd_t *parent, char *title, int dist )
{
	vbox_t *vbox;
	wnd_class_t *klass;

	/* Allocate memory for box */
	vbox = (vbox_t *)malloc(sizeof(*vbox));
	if (vbox == NULL)
		return NULL;
	memset(vbox, 0, sizeof(*vbox));
	WND_OBJ(vbox)->m_class = vbox_class_init(WND_GLOBAL(parent));

	/* Initialize box */
	if (!vbox_construct(vbox, parent, title, dist))
	{
		free(vbox);
		return NULL;
	}
	WND_OBJ(vbox)->m_flags |= WND_FLAG_INITIALIZED;
	return vbox;
} /* End of 'vbox_new' function */

/* Vertical box constructor */
bool_t vbox_construct( vbox_t *vbox, wnd_t *parent, char *title, int dist )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(vbox), parent, title, "", 
				vbox_get_desired_size, vbox_set_pos, 0,
				DLGITEM_NOTABSTOP | (title != NULL ? DLGITEM_BORDER : 0)))
		return FALSE;

	/* Initialize box fields */
	vbox->m_dist = dist;
	return TRUE;
} /* End of 'vbox_construct' function */

/* Get the desired size */
void vbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	wnd_t *child;
	wnd_t *wnd = WND_OBJ(di);
	vbox_t *box = VBOX_OBJ(di);

	/* Width is the maximal desired by our children width; 
	 * height is the sum of desired by the children */
	*width = 0;
	*height = 0;
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		int dw, dh;
		dlgitem_get_size(DLGITEM_OBJ(child), &dw, &dh);
		if (dw > (*width))
			(*width) = dw;
		(*height) += (dh + box->m_dist);
	}
	if (WND_FLAGS(di) & WND_FLAG_BORDER)
	{
		(*width) += 2;
		(*height) += 2;
	}
} /* End of 'vbox_get_desired_size' function */

/* Set position */
void vbox_set_pos( dlgitem_t *di, int x, int y, int width, int height )
{
	int cx, cys, cye;
	vbox_t *box = VBOX_OBJ(di);
	wnd_t *child;

	/* Arrange the children */
	cx = 0;
	if (WND_FLAGS(di) & WND_FLAG_BORDER)
	{
		cys = 0;
		cye = height - 1;
	}
	else
	{
		cys = 0;
		cye = height;
	}
	for ( child = WND_OBJ(di)->m_child; child != NULL; child = child->m_next )
	{
		dlgitem_t *child_di = DLGITEM_OBJ(child);
		int w, h;

		/* Get size desired by this child */
		dlgitem_get_size(child_di, &w, &h);

		/* Set child size */
		if (!(child_di->m_flags & DLGITEM_PACK_END))
		{
			dlgitem_set_pos(child_di, cx, cys, w, h);
			cys += (h + box->m_dist);
		}
		else
		{
			cye -= h;
			dlgitem_set_pos(child_di, cx, cye, w, h);
			cye -= box->m_dist;
		}
	}
} /* End of 'vbox_set_pos' function */

/* Initialize vbox class */
wnd_class_t *vbox_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "vbox", dlgitem_class_init(global), NULL, 
			NULL, NULL);
} /* End of 'vbox_class_init' function */

/* End of 'wnd_vbox.c' file */

