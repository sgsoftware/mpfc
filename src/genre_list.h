/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : genre_list.h
 * PURPOSE     : SG MPFC. Interface for genres list management 
 *               functions.
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

#ifndef __SG_MPFC_GENRE_LIST_H__
#define __SG_MPFC_GENRE_LIST_H__

#include "types.h"

/* Genre list type */
typedef struct tag_genre_list_t
{
	struct tag_glist_item_t
	{
		char *m_name;
		byte m_data;
	} *m_list;
	int m_size;
} genre_list_t;

/* Create a new genre list */
genre_list_t *glist_new( void );

/* Free genre list */
void glist_free( genre_list_t *l );

/* Add genre to list */
void glist_add( genre_list_t *l, char *name, byte data );

/* Get genre name */
char *glist_get_name( genre_list_t *l, int id );

/* Get genre name by its id */
char *glist_get_name_by_id( genre_list_t *l, int id );

/* Get genre id by its name */
byte glist_get_id_by_name( genre_list_t *l, char *name );

/* Convert genre string to id */
int glist_str2num( char *str );

#endif

/* End of 'genre_list.h' file */

