/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_dlgitem.h
 * PURPOSE     : MPFC Window Library. Interface for common dialog
 *               item functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'dlgitem'
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

#ifndef __SG_MPFC_WND_DLG_ITEM_H__
#define __SG_MPFC_WND_DLG_ITEM_H__

#include "types.h"
#include "wnd.h"

/* Dialog item type */
typedef struct 
{
	/* Window part */
	wnd_t m_wnd;

	/* Item ID string */
	char *m_id;
} dlgitem_t;

/* Convert window object to dialog item type */
#define DLGITEM_OBJ(wnd)	((dlgitem_t *)wnd)

/* Construct dialog item */
bool_t dlgitem_construct( dlgitem_t *di, char *title, char *id, wnd_t *parent, 
		int x, int y, int width, int height );

/* Destructor */
void dlgitem_destructor( wnd_t *wnd );

/* 'keydown' message handler */
wnd_msg_retcode_t dlgitem_on_keydown( wnd_t *wnd, wnd_key_t key );

#endif

/* End of 'wnd_dlgitem.h' file */

