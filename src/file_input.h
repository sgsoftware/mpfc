/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : file_input.h
 * PURPOSE     : SG Konsamp. Interface for file input edit box
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2003
 * NOTE        : Module prexix 'fin'.
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

#ifndef __SG_KONSAMP_FILE_INPUT_H__
#define __SG_KONSAMP_FILE_INPUT_H__

#include "types.h"
#include "editbox.h"
#include "window.h"

/* Directory contents list type */
typedef struct tag_fin_list
{
	char m_name[256];
	struct tag_fin_list *m_next;
} fin_list_t;

/* File input edit box type */
typedef struct
{
	/* Common edit box object */
	editbox_t m_box;

	/* Current path */
	char m_cur_path[256];

	/* Cursor position before path expansion */
	int m_was_cursor;

	/* Length of pasted text (after path expansion) */
	int m_paste_len;

	/* Directory contents */
	fin_list_t *m_dir_list, *m_cur_list;
} file_input_box_t;

/* Create a new file input box */
file_input_box_t *fin_new( wnd_t *parent, int x, int y, int width, 
							char *label );

/* Initialize file input box */
bool fin_init( file_input_box_t *fin, wnd_t *parent, int x, int y,
	   			int w, char *label );

/* Destroy file input box */
void fin_destroy( wnd_t *wnd );

/* File input box key handler function */
int fin_handle_key( wnd_t *wnd, dword data );

/* Expand file name */
void fin_expand( file_input_box_t *box );

/* Search for current file name in list */
bool fin_search( file_input_box_t *box );

/* Paste current match */
void fin_paste( file_input_box_t *box );

/* Rebuild directory list */
void fin_rebuild_list( file_input_box_t *box );

/* Add an entry to list */
fin_list_t *fin_add_entry( file_input_box_t *box, char *name,
	   						fin_list_t *l );

/* Free directory list */
void fin_free_list( file_input_box_t *box );

#endif

/* End of 'file_input.h' file */

