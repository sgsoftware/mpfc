/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_label.c
 * PURPOSE     : MPFC Window Library. Label functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.08.2004
 * NOTE        : Module prefix 'label'.
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
#include "wnd_label.h"

/* Create a new label */
label_t *label_new( wnd_t *parent, char *text )
{
	label_t *l;

	/* Allocate memory */
	l = (label_t *)malloc(sizeof(*l));
	if (l == NULL)
		return NULL;
	memset(l, 0, sizeof(*l));
	WND_OBJ(l)->m_class = wnd_basic_class_init(WND_GLOBAL(parent));

	/* Initialize label */
	if (!label_construct(l, parent, text))
	{
		free(l);
		return NULL;
	}
	WND_FLAGS(l) |= WND_FLAG_INITIALIZED;
	return l;
} /* End of 'label_new' function */

/* Label constructor */
bool_t label_construct( label_t *l, wnd_t *parent, char *text )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(l), parent, text, NULL, 
				label_get_desired_size, NULL, DLGITEM_NOTABSTOP))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(l), "display", label_on_display);
	return TRUE;
} /* End of 'label_construct' function */

/* Calculate size desired by this label */
void label_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	*width = strlen(WND_OBJ(di)->m_title);
	*height = 1;
} /* End of 'label_get_desired_size' function */

/* 'display' message handler */
wnd_msg_retcode_t label_on_display( wnd_t *wnd )
{
	wnd_move(wnd, 0, 0, 0);
	wnd_set_fg_color(wnd, WND_COLOR_WHITE);
	wnd_set_bg_color(wnd, WND_COLOR_BLACK);
	wnd_set_attrib(wnd, WND_ATTRIB_BOLD);
	wnd_putstring(wnd, 0, 0, wnd->m_title);
} /* End of 'label_on_display' function */

/* End of 'wnd_label.c' file */

