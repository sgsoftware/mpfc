/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : editbox.h
 * PURPOSE     : SG Konsamp. Interface for edit box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2003
 * NOTE        : Module prefix 'ebox'.
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

#ifndef __SG_KONSAMP_EDIT_BOX_H__
#define __SG_KONSAMP_EDIT_BOX_H__

#include "types.h"
#include "window.h"

/* Edit box type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Edit box label */
	char m_label[80];

	/* Edit box text */
	char m_text[256];
	int m_len;

	/* Last pressed key */
	int m_last_key;

	/* The maximal text length */
	int m_max_len;

	/* Cursor position */
	int m_cursor;

	/* Scroll value */
	int m_scrolled;
} editbox_t;

/* Create a new edit box */
editbox_t *ebox_new( wnd_t *parent, int x, int y, int width, 
						int height,	int max_len, char *label, char *text );

/* Initialize edit box */
bool ebox_init( editbox_t *ebox, wnd_t *parent, int x, int y, int width, 
					int height,	int max_len, char *label, char *text );

/* Destroy edit box */
void ebox_destroy( wnd_t *wnd );

/* Edit box display function */
int ebox_display( wnd_t *wnd, dword data );

/* Edit box key handler function */
int ebox_handle_key( wnd_t *wnd, dword data );

/* Add a character to edit box */
void ebox_add( editbox_t *box, char c );

/* Delete a character */
void ebox_del( editbox_t *box, int index );

/* Move cursor */
void ebox_move( editbox_t *box, bool rel, int offset );

/* Set new cursor position */
void ebox_set_cursor( editbox_t *box, int new_pos );

#endif

/* End of 'editbox.h' file */

