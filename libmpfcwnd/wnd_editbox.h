/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_editbox.h
 * PURPOSE     : MPFC Window Library. Interface for edit box
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'editbox'.
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

#ifndef __SG_MPFC_WND_EDITBOX_H__
#define __SG_MPFC_WND_EDITBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Edit box type */
typedef struct
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Edit box text */
	char *m_text;
	int m_len;

	/* Cursor position */
	int m_cursor;
} editbox_t;

/* Convert window object to edit box type */
#define EDITBOX_OBJ(wnd)	((editbox_t *)wnd)

/* Create a new edit box */
editbox_t *editbox_new( wnd_t *parent, char *id, int x, int y, int width, 
		int height );

/* Edit box constructor */
bool_t editbox_construct( editbox_t *eb, wnd_t *parent, char *id, int x, int y, 
		int width, int height );

/* Destructor */
void editbox_destructor( wnd_t *wnd );

/* Set edit box text */
void editbox_set_text( editbox_t *eb, char *text );

/* 'display' message handler */
wnd_msg_retcode_t editbox_on_display( wnd_t *wnd );

/* 'keydown' message handler */
wnd_msg_retcode_t editbox_on_keydown( wnd_t *wnd, wnd_key_t key );

#endif

/* End of 'wnd_editbox.h' file */

