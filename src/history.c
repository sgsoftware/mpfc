/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : history.c
 * PURPOSE     : SG MPFC. Edit history functions implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "error.h"
#include "history.h"

/* Initialize list */
hist_list_t *hist_list_new( void )
{
	hist_list_t *l;

	/* Allocate memory for list */
	l = (hist_list_t *)malloc(sizeof(hist_list_t));
	if (l == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Set fields */
	l->m_head = l->m_tail = l->m_cur = NULL;
	return l;
} /* End of 'hist_list_new' function */

/* Free list */
void hist_list_free( hist_list_t *l )
{
	if (l == NULL)
		return;

	if (l->m_head != NULL)
	{
		struct tag_hist_list_entry_t *t, *t1;
		
		for ( t = l->m_head; t != NULL; )
		{
			t1 = t->m_next;
			free(t);
			free(t->m_text);
			t = t1;
		}
	}
	free(l);
} /* End of 'hist_list_free' function */

/* Add an item to list */
void hist_add_item( hist_list_t *l, char *text )
{
	struct tag_hist_list_entry_t *t;
	
	if (l == NULL)
		return;

	if (l->m_tail == NULL)
	{
		t = l->m_tail = l->m_head = 
			(struct tag_hist_list_entry_t *)malloc(sizeof(*t));
		t->m_prev = NULL;
	}
	else
	{
		t = l->m_tail->m_next = 
			(struct tag_hist_list_entry_t *)malloc(sizeof(*t));
		t->m_prev = l->m_tail;
	}
	t->m_next = NULL;
	t->m_text = strdup(text);
	l->m_tail = t;
} /* End of 'hist_add_item' function */

/* End of 'history.c' file */

