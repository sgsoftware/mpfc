/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : button.h
 * PURPOSE     : SG MPFC. Interface for button functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.02.2004
 * NOTE        : Module prefix 'btn'.
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

#ifndef __SG_MPFC_BUTTON_H__
#define __SG_MPFC_BUTTON_H__

#include "types.h"
#include "window.h"

/* Button type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Button text */
	char *m_text;
} button_t;

/* Button notification messages */
#define BTN_CLICKED 0

/* Create a new button */
button_t *btn_new( wnd_t *parent, int x, int y, int w, char *text );

/* Initialize button */
bool_t btn_init( button_t *btn, wnd_t *parent, int x, int y, int w, char *text );

/* Destroy button */
void btn_destroy( wnd_t *wnd );

/* Button display function */
void btn_display( wnd_t *wnd, dword data );

/* Button key handler function */
void btn_handle_key( wnd_t *wnd, dword data );

/* Button left mouse click handler */
void btn_handle_mouse( wnd_t *wnd, dword data );

/* Postponed notify message handler */
void btn_handle_pp_notify( wnd_t *wnd, dword data );

#endif

/* End of 'button.h' file */

