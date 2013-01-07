/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Genres list management functions implementation.
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

#include <stdlib.h>
#include <string.h>
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
		{
			int i;
			
			for ( i = 0; i < l->m_size; i ++ )
				free(l->m_list[i].m_name);
			free(l->m_list);
		}
		free(l);
	}
} /* End of 'glist_free' function */

/* Add genre to list */
void glist_add( genre_list_t *l, char *name, byte data )
{
	if (l == NULL)
		return;

	l->m_list = (struct tag_glist_item_t *)realloc(l->m_list,
				sizeof(*(l->m_list)) * (l->m_size + 1));
	l->m_list[l->m_size].m_name = strdup(name);
	l->m_list[l->m_size ++].m_data = data;
} /* End of 'glist_add' function */

/* Get genre name */
char *glist_get_name( genre_list_t *l, int id )
{
	if (l == NULL || id < 0 || id >= l->m_size)
		return NULL;
	return l->m_list[id].m_name;
} /* End of 'glist_get_name' function */

/* Get genre name by its id */
char *glist_get_name_by_id( genre_list_t *l, int id )
{
	int i;

	if (l == NULL)
		return NULL;

	for ( i = 0; i < l->m_size; i ++ )
		if (l->m_list[i].m_data == id)
			return l->m_list[i].m_name;
	return NULL;
} /* End of 'glist_get_name_by_id' function */

/* Convert genre string to id */
int glist_str2num( const char *str )
{
	int num = -1;

	if (str == NULL)
		return -1;

	for ( ; *str; str ++ )
	{
		if (*str >= '0' && *str <= '9')
		{
			if (num == -1)
				num = (*str - '0');
			else
			{
				num *= 10;
				num += (*str - '0');
			}
		}
		else if (*str == '(' && num < 0)
			continue;
		else if (*str == ')' && num >= 0)
			return num;
		else
			return -1;
	}
	return num;
} /* End of 'glist_str2num' function */

/* Get genre id by its name */
byte glist_get_id_by_name( genre_list_t *l, char *name )
{
	int id, i;
	
	if (l == NULL || name == NULL)
		return 0xFF;

	/* If name is in fact number - return it */
	id = glist_str2num(name);
	if (id >= 0)
		return id;
	
	/* Search for genre with such name */
	for ( i = 0; i < l->m_size; i ++ )
		if (!strcmp(l->m_list[i].m_name, name))
			return l->m_list[i].m_data;
	return 0xFF;
} /* End of 'glist_get_id_by_name' function */

/* End of 'genre_list.c' file */

