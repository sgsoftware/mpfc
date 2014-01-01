/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for label functions.
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

#ifndef __SG_MPFC_WND_LABEL_H__
#define __SG_MPFC_WND_LABEL_H__

#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Label flags */
typedef enum
{
	/* Text is not bold */
	LABEL_NOBOLD = 1 << 0,
} label_flags_t;

/* Parsed label text */
typedef struct
{
	/* Effective text is "pre letter post"
	 * but 'letter' will be highlighted */
	char *pre;
	char letter;
	char *post;

	int width;
	int height;
} label_text_t;

/* Label type */
typedef struct 
{
	/* Dialog item part */
	dlgitem_t m_wnd;

	/* Label text */
	label_text_t m_text;

	/* Label flags */
	label_flags_t m_flags;
} label_t;

/* Convert window object to label type */
#define LABEL_OBJ(wnd)	((label_t *)wnd)

/* Create a new label */
label_t *label_new( wnd_t *parent, char *text, char *id, label_flags_t flags );

/* Label constructor */
bool_t label_construct( label_t *l, wnd_t *parent, char *text, char *id,
		label_flags_t flags );

/* Destructor */
void label_destructor( wnd_t *wnd );

/* Parse label title */
void label_text_parse( label_text_t *text, char *str );

/* Free label title struct */
void label_text_free( label_text_t *text );

/* Display a label-like text */
void label_text_display( wnd_t *wnd, label_text_t *text );

/* Create a label with another label */
label_t *label_new_with_label( wnd_t *parent, char *title, char *text,
		char *id, label_flags_t flags );

/* Set label text */
void label_set_text( label_t *l, char *text );

/* Calculate size desired by this label */
void label_get_desired_size( dlgitem_t *di, int *width, int *height );

/* 'display' message handler */
wnd_msg_retcode_t label_on_display( wnd_t *wnd );

/*
 * Class functions
 */

/* Initialize label class */
wnd_class_t *label_class_init( wnd_global_data_t *global );

#endif

/* End of 'wnd_label.h' file */

