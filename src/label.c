/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : label.c
 * PURPOSE     : SG MPFC. Label functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.08.2003
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
#include "error.h"
#include "label.h"
#include "window.h"

/* Create a new label */
label_t *label_new( wnd_t *parent, int x, int y, int w, int h, char *text )
{
	label_t *l;

	/* Allocate memory for label */
	l = (label_t *)malloc(sizeof(label_t));
	if (l == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize button fields */
	if (!label_init(l, parent, x, y, w, h, text))
	{
		free(l);
		return NULL;
	}
	return l;
} /* End of 'label_new' function */

/* Initialize label */
bool_t label_init( label_t *l, wnd_t *parent, int x, int y, int w, int h, 
		char *text )
{
	/* Create common window part */
	if (!wnd_init(&l->m_wnd, parent, x, y, w, h))
		return FALSE;

	/* Register message handlers */
	wnd_register_handler(l, WND_MSG_DISPLAY, label_display);
	wnd_register_handler(l, WND_MSG_KEYDOWN, label_handle_key);

	/* Set label-specific fields */
	l->m_text = strdup(text);
	WND_OBJ(l)->m_wnd_destroy = label_destroy;
	WND_OBJ(l)->m_flags |= (WND_ITEM | WND_NO_FOCUS | WND_INITIALIZED);
	return TRUE;
} /* End of 'label_init' function */

/* Destroy label */
void label_destroy( wnd_t *wnd )
{
	label_t *l = (label_t *)wnd;
	
	if (l->m_text != NULL)
		free(l->m_text);
		
	/* Destroy window */
	wnd_destroy_func(wnd);
} /* End of 'label_destroy' function */

/* Label display function */
void label_display( wnd_t *wnd, dword data )
{
	label_t *l = (label_t *)wnd;
	char *t;
	int y;

	if (l == NULL || l->m_text == NULL)
		return;

	wnd_move(wnd, 0, 0);
	wnd_printf(wnd, "%s", l->m_text);
} /* End of 'label_display' function */

/* Label key handler function */
void label_handle_key( wnd_t *wnd, dword data )
{
} /* End of 'label_handle_key' function */

/* End of 'label.c' file */

