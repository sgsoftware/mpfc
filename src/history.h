/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : history.h
 * PURPOSE     : SG MPFC. Interface for edit history functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.02.2004
 * NOTE        : Module prefix 'hist'.
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

#ifndef __SG_MPFC_HISTORY_H__
#define __SG_MPFC_HISTORY_H__

#include "types.h"

/* Edit box history list type */
typedef struct tag_hist_list_t
{
	/* List */
	struct tag_hist_list_entry_t
	{
		char *m_text;
		struct tag_hist_list_entry_t *m_next, *m_prev;
	} *m_head, *m_tail, *m_cur;
} hist_list_t;

/* Initialize list */
hist_list_t *hist_list_new( void );

/* Free list */
void hist_list_free( hist_list_t *l );

/* Add an item to list */
void hist_add_item( hist_list_t *l, char *text );

#endif

/* End of 'history.h' file */

