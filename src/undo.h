/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for undo list management functions.
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

#ifndef __SG_MPFC_UNDO_H__
#define __SG_MPFC_UNDO_H__

#include "types.h"
#include "plist.h"

/* Undo actions */
#define UNDO_MOVE		0
#define UNDO_ADD		1
#define UNDO_ADD_OBJ	2
#define UNDO_REM		3
#define UNDO_SORT		4

/* Undo list type */
typedef struct tag_undo_list_t
{
	struct tag_undo_list_item_t
	{
		/* Action type */
		byte m_type;

		/* Action data */
		union
		{
			struct tag_undo_list_move_t
			{
				int m_start, m_end, m_to;
			} m_move_plist;
			struct tag_undo_list_add_t
			{
				plist_set_t *m_set;
				int m_num_songs;
			} m_add;
			struct tag_undo_list_add_obj_t
			{
				char *m_obj_name;
				int m_num_songs;
			} m_add_obj;
			struct tag_undo_list_rem_t
			{
				struct song_name
				{
					char *m_fullname;
					char *m_filename;
					song_metadata_t m_metadata;
				} *m_files;
				int m_num_files;
				int m_start_pos;
			} m_rem;
			struct tag_undo_list_sort_t
			{
				int *m_transform;
				int m_was_song;
			} m_sort;
		} m_data;

		/* Pointers to next and previous items */
		struct tag_undo_list_item_t *m_next, *m_prev;
	} *m_head, *m_tail, *m_cur;
} undo_list_t;

/* Fix manual selection update */
#define UNDO_FIX_SEL(pl) \
	if (!(pl)->m_len) \
		(pl)->m_sel_start = (pl)->m_sel_end = -1; \
	else \
	{ \
		if ((pl)->m_sel_start < 0)	\
			(pl)->m_sel_start = 0; \
		else if ((pl)->m_sel_start >= (pl)->m_len)	\
			(pl)->m_sel_start = (pl)->m_len - 1; \
		if ((pl)->m_sel_end < 0)	\
			(pl)->m_sel_end = 0; \
		else if ((pl)->m_sel_end >= (pl)->m_len)	\
			(pl)->m_sel_end = (pl)->m_len - 1; \
	}

/* Initialize undo list */
undo_list_t *undo_new( void );

/* Free undo list */
void undo_free( undo_list_t *ul );

/* Add an action to list */
void undo_add( undo_list_t *ul, struct tag_undo_list_item_t *item );

/* Move forward */
void undo_fw( undo_list_t *ul );

/* Move backward */
void undo_bw( undo_list_t *ul );

/* Do action */
void undo_do( struct tag_undo_list_item_t *item );

/* Undo action */
void undo_undo( struct tag_undo_list_item_t *item );

/* Free list */
void undo_free_list( struct tag_undo_list_item_t *l );

#endif

/* End of 'undo.h' file */

