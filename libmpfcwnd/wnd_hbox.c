/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_hbox.c
 * PURPOSE     : SG MPFC Window Library. Horizontal box functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.08.2004
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

#include <stdlib.h>
#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_hbox.h"

/* Create a new horizontal box */
hbox_t *hbox_new( wnd_t *parent, int dist )
{
	hbox_t *hbox;
	wnd_class_t *klass;

	/* Allocate memory for box */
	hbox = (hbox_t *)malloc(sizeof(*hbox));
	if (hbox == NULL)
		return NULL;
	memset(hbox, 0, sizeof(*hbox));

	/* Initialize window class */
	klass = wnd_basic_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
	{
		free(hbox);
		return NULL;
	}
	WND_OBJ(hbox)->m_class = klass;

	/* Initialize box */
	if (!hbox_construct(hbox, parent, dist))
	{
		free(hbox);
		return NULL;
	}
	WND_OBJ(hbox)->m_flags |= WND_FLAG_INITIALIZED;
	return hbox;
} /* End of 'hbox_new' function */

/* Horizontal box constructor */
bool_t hbox_construct( hbox_t *hbox, wnd_t *parent, int dist )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(hbox), parent, "", "", 
				hbox_get_desired_size, hbox_set_pos, DLGITEM_NOTABSTOP))
		return FALSE;

	/* Set message handlers */
	wnd_msg_add_handler(WND_OBJ(hbox), "keydown", hbox_on_keydown);

	/* Initialize box fields */
	hbox->m_dist = dist;
	return TRUE;
} /* End of 'hbox_construct' function */

/* 'keydown' message handler */
wnd_msg_retcode_t hbox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	return WND_MSG_RETCODE_PASS_TO_PARENT;
} /* End of 'hbox_on_keydown' function */

/* Get the desired size */
void hbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	wnd_t *child;
	wnd_t *wnd = WND_OBJ(di);
	hbox_t *box = HBOX_OBJ(di);

	/* Height is the maximal desired by our children height; 
	 * width is the sum of desired by the children */
	*width = 0;
	*height = 0;
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		int dw, dh;
		dlgitem_get_size(DLGITEM_OBJ(child), &dw, &dh);
		if (dh > (*height))
			(*height) = dh;
		(*width) += (dw + box->m_dist);
	}
} /* End of 'hbox_get_desired_size' function */

/* Set position */
void hbox_set_pos( dlgitem_t *di, int x, int y, int width, int height )
{
	int cxs, cxe, cy;
	hbox_t *box = HBOX_OBJ(di);
	wnd_t *child;

	/* Arrange the children */
	cxs = 0;
	cxe = width;
	cy = 0;
	for ( child = WND_OBJ(di)->m_child; child != NULL; child = child->m_next )
	{
		dlgitem_t *child_di = DLGITEM_OBJ(child);
		int w, h;

		/* Get size desired by this child */
		dlgitem_get_size(child_di, &w, &h);

		/* Set child size */
		if (!(child_di->m_flags & DLGITEM_PACK_END))
		{
			dlgitem_set_pos(child_di, cxs, cy, w, h);
			cxs += (w + box->m_dist);
		}
		else
		{
			cxe -= w;
			dlgitem_set_pos(child_di, cxe, cy, w, h);
			cxe -= box->m_dist;
		}
	}
} /* End of 'hbox_set_pos' function */

/* End of 'wnd_hbox.c' file */


