/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : dlg_ctrl.h
 * PURPOSE     : SG MPFC. Interface for dialog control items 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.11.2003
 * NOTE        : Module prefix 'dlgctrl'.
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

#ifndef __SG_MPFC_DLG_CTRL_H__
#define __SG_MPFC_DLG_CTRL_H__

#include "types.h"
#include "window.h"

/* Dialog control item type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Control type (OK or cancel) */
	byte m_type;
} dlgctrl_t;

/* Control types */
#define DLGCTRL_OK 0
#define DLGCTRL_CANCEL 1

/* Create a new dialog control */
dlgctrl_t *dlgctrl_new( wnd_t *parent, int x, int y, int type );

/* Initialize dialog control */
bool_t dlgctrl_init( dlgctrl_t *btn, wnd_t *parent, int x, int y, byte type);

/* Destroy dialog control */
void dlgctrl_destroy( wnd_t *wnd );

/* Dialog control display function */
void dlgctrl_display( wnd_t *wnd, dword data );

/* Dialog control left mouse click handler */
void dlgctrl_handle_mouse( wnd_t *wnd, dword data );

#endif

/* End of 'dlg_ctrl.h' file */

