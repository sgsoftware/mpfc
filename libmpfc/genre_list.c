/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : genre_list.c
 * PURPOSE     : SG MPFC. Genres list management functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.08.2003
 * NOTE        : Module prefix 'glist'. 
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
#include "types.h"
#include "error.h"
#include "genre_list.h"

/* Create a new genre list */
genre_list_t *glist_new( void )
{
	genre_list_t *l;

	/* Try to allocate memory */
	l = (genre_list_t *)malloc(sizeof(genre_list_t));
	if (l == NULL)
	{
	//	error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize list */
	l->m_list = NULL;
	l->m_size = 0;
	return l;
} /* End of 'glist_new' function */

/* Free genre list */
void glist_free( genre_list_t *l )
{
	if (l != NULL)
	{
		if (l->m_list != NULL)
			free(l->m_list);
		free(l);
	}
} /* End of 'glist_free' function */

/* Add genre to list */
void glist_add( genre_list_t *l, char *name, byte data )
{
	if (l == NULL)
		return;

	if (l->m_list == NULL)
		l->m_list = (struct tag_glist_item_t *)malloc(sizeof(*(l->m_list)));
	else
		l->m_list = (struct tag_glist_item_t *)realloc(l->m_list,
				sizeof(*(l->m_list)) * (l->m_size + 1));
	strcpy(l->m_list[l->m_size].m_name, name);
	l->m_list[l->m_size ++].m_data = data;
} /* End of 'glist_add' function */

/* Get genre id by its inner data */
byte glist_get_id( genre_list_t *l, byte data )
{
	int i;

	if (l == NULL)
		return 0xFF;

	for ( i = 0; i < l->m_size; i ++ )
		if (l->m_list[i].m_data == data)
			return i;
	return GENRE_ID_UNKNOWN;
} /* End of 'glist_get_id' function */

/* Get genre if by its textual data */
byte glist_get_id_by_text( genre_list_t *l, char *text )
{
	int i;

	if (l == NULL)
		return 0xFF;

	for ( i = 0; i < l->m_size; i ++ )
		if (!strcmp(l->m_list[i].m_name, text))
			return i;
	return GENRE_ID_UNKNOWN;
} /* End of 'glist_get_id_by_text' function */

/* End of 'genre_list.c' file */

