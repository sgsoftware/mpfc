/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : eqwnd.h
 * PURPOSE     : SG MPFC. Interface for equalizer window functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.07.2003
 * NOTE        : Module prefix 'eqwnd'.
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

#ifndef __SG_MPFC_EQUALIZER_H__
#define __SG_MPFC_EQUALIZER_H__

#include "types.h"
#include "window.h"

/* Equalizer window type */
typedef struct tag_eq_wnd_t
{
	/* Window part */
	wnd_t m_wnd;

	/* Cursor position */
	int m_pos;
} eq_wnd_t;

/* Create a new equalizer window */
eq_wnd_t *eqwnd_new( wnd_t *parent, int x, int y, int w, int h );

/* Initialize equalizer window */
bool eqwnd_init( eq_wnd_t *eq, wnd_t *parent, int x, int y, int w, int h );

/* Destroy equalizer window */
void eqwnd_free( wnd_t *wnd );

/* Handle display message */
void eqwnd_display( wnd_t *wnd, dword data );

/* Handle key message */
void eqwnd_handle_key( wnd_t *wnd, dword data );

/* Display slider */
int eqwnd_display_slider( wnd_t *wnd, int x, int start_y, int end_y,
							bool hl, float val, char *str );

#endif

/* End of 'eqwnd.h' file */

