/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for file box functions.
 * $Id$
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

#ifndef __SG_MPFC_WND_FILEBOX_H__
#define __SG_MPFC_WND_FILEBOX_H__

#include "types.h"
#include "wnd.h"
#include "wnd_editbox.h"

/* File box flags */
typedef enum
{
	FILEBOX_ONLY_DIRS = 1 << 0,
} filebox_flags_t;

/* File box type */
typedef struct
{
	/* Edit box part */
	editbox_t m_wnd;

	/* Loaded names list */
	struct filebox_name_t
	{
		char *m_name;
		struct filebox_name_t *m_next, *m_prev;
	} *m_names;
	bool_t m_names_valid;

	/* Flags */
	filebox_flags_t m_flags;

	/* Start of the text that we are inserting */
	int m_insert_start;

	/* Names building temporary values */
	str_t *m_pattern;
	bool_t m_use_global, m_not_first;

	/* Command box flag */
	bool_t m_command_box;
} filebox_t;

/* Convert window object to file box type */
#define FILEBOX_OBJ(wnd)	((filebox_t *)wnd)

/* Create a new file box */
filebox_t *filebox_new( wnd_t *parent, char *id, char *text, char letter,
		int width );

/* File box constructor */
bool_t filebox_construct( filebox_t *fb, wnd_t *parent, char *id, char *text, 
		char letter, int width );

/* Create an edit box with label */
filebox_t *filebox_new_with_label( wnd_t *parent, char *title, char *id,
		char *text, char letter, int width );

#endif

/* End of 'filebox.h' file */

