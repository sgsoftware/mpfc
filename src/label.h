/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : label.h
 * PURPOSE     : SG MPFC. Interface for label functions.
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

#ifndef __SG_MPFC_LABEL_H__
#define __SG_MPFC_LABEL_H__

#include "types.h"
#include "window.h"

/* Label type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Label text */
	char *m_text;
} label_t;

/* Create a new label */
label_t *label_new( wnd_t *parent, int x, int y, int w, int h, char *text );

/* Initialize label */
bool label_init( label_t *l, wnd_t *parent, int x, int y, int w, int h, 
		char *text );

/* Destroy label */
void label_destroy( wnd_t *wnd );

/* Label display function */
void label_display( wnd_t *wnd, dword data );

/* Label key handler function */
void label_handle_key( wnd_t *wnd, dword data );

#endif

/* End of 'label.h' file */

