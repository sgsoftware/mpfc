/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : dlgbox.h
 * PURPOSE     : SG Konsamp. Interface for dialog box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 7.03.2003
 * NOTE        : Module prefix 'dlg'.
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

#ifndef __SG_KONSAMP_DLG_BOX_H__
#define __SG_KONSAMP_DLG_BOX_H__

#include "types.h"
#include "window.h"

/* Dialog box type */
typedef struct
{
	/* Window object */
	wnd_t m_wnd;

	/* Dialog box caption */
	char m_caption[256];
} dlgbox_t;

/* Create a new dialog box */
dlgbox_t *dlg_new( wnd_t *parent, int x, int y, int w, int h,
	   				char *caption );

/* Initialize edit box */
bool dlg_init( dlgbox_t *dlg, wnd_t *parent, int x, int y, int w, int h,
	   				char *caption  );

/* Destroy dialog box */
void dlg_destroy( wnd_t *wnd );

/* Dialog box display function */
int dlg_display( wnd_t *wnd, dword data );

/* Dialog box key handler function  */
int dlg_handle_key( wnd_t *wnd, dword data );

#endif

/* End of 'dlgbox.h' file */

