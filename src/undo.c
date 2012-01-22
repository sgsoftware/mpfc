/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Undo list management functions implementation.
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
#include "player.h"
#include "plist.h"
#include "vfs.h"
#include "undo.h"

/* Initialize undo list */
undo_list_t *undo_new( void )
{
	undo_list_t *ul;

	/* Allocate memory */
	ul = (undo_list_t *)malloc(sizeof(undo_list_t));
	if (ul == NULL)
		return NULL;

	/* Fill memory */
	ul->m_head = ul->m_tail = ul->m_cur = NULL;
	return ul;
} /* End of 'undo_new' function */

/* Free undo list */
void undo_free( undo_list_t *ul )
{
	if (ul == NULL)
		return;

	undo_free_list(ul->m_head);
	free(ul);
} /* End of 'undo_free' function */

/* Add an action to list */
void undo_add( undo_list_t *ul, struct tag_undo_list_item_t *item )
{
	if (ul == NULL || item == NULL)
		return;

	/* Free list tail */
	if (ul->m_cur != NULL)
		ul->m_tail = ul->m_cur->m_prev;
	undo_free_list(ul->m_cur);

	/* Add */
	if (ul->m_tail == NULL)
	{
		ul->m_head = ul->m_tail = item;
		ul->m_head->m_prev = ul->m_head->m_next = NULL;
	}
	else
	{
		ul->m_tail->m_next = item;
		item->m_prev = ul->m_tail;
		item->m_next = NULL;
		ul->m_tail = item;
	}
	ul->m_cur = NULL;
} /* End of 'undo_add' function */

/* Move forward */
void undo_fw( undo_list_t *ul )
{
	if (ul == NULL || ul->m_cur == NULL)
		return;

	/* Do action and move */
	undo_do(ul->m_cur);
	ul->m_cur = ul->m_cur->m_next;
} /* End of 'undo_fw' function */

/* Move backward */
void undo_bw( undo_list_t *ul )
{
	if (ul == NULL || ul->m_cur == ul->m_head)
		return;

	/* Move and undo action */
	if (ul->m_cur == NULL)
		ul->m_cur = ul->m_tail;
	else
		ul->m_cur = ul->m_cur->m_prev;
	undo_undo(ul->m_cur);
} /* End of 'undo_bw' function */

/* Do action */
void undo_do( struct tag_undo_list_item_t *item )
{
	int i;
	bool_t was_store = player_store_undo;

	if (item == NULL)
		return;

	player_store_undo = FALSE;

	/* Add files */
	if (item->m_type == UNDO_ADD)
	{
		struct tag_undo_list_add_t *data = &item->m_data.m_add;
		char *was_val;
		plist_add_set(player_plist, data->m_set);
		plist_flush_scheduled(player_plist);
	}
	/* Remove songs */
	else if (item->m_type == UNDO_REM)
	{
		struct tag_undo_list_rem_t *data = &item->m_data.m_rem;
		int was_start = player_plist->m_sel_start,
			was_end = player_plist->m_sel_end;
		
		player_plist->m_sel_start = data->m_start_pos;
		player_plist->m_sel_end = data->m_start_pos + data->m_num_files - 1;
		plist_rem(player_plist);
		player_plist->m_sel_start = was_start;
		player_plist->m_sel_end = was_end;
		UNDO_FIX_SEL(player_plist);
	}
	/* Move selection */
	else if (item->m_type == UNDO_MOVE)
	{
		struct tag_undo_list_move_t *data = &item->m_data.m_move_plist;
		int was_start = player_plist->m_sel_start,
			was_end = player_plist->m_sel_end;
		
		player_plist->m_sel_start = data->m_start;
		player_plist->m_sel_end = data->m_end;
		plist_move_sel(player_plist, data->m_to, FALSE);
		player_plist->m_sel_start = was_start;
		player_plist->m_sel_end = was_end;
		UNDO_FIX_SEL(player_plist);
	}
	/* Sort */
	else if (item->m_type == UNDO_SORT)
	{
		int i;
		struct tag_undo_list_sort_t *data = &item->m_data.m_sort;
		song_t **list = (song_t **)malloc(sizeof(song_t *) * 
				player_plist->m_len);
		plist_lock(player_plist);
		memcpy(list, player_plist->m_list, sizeof(song_t *) * 
				player_plist->m_len);
		for ( i = 0; i < player_plist->m_len; i ++ )
			player_plist->m_list[data->m_transform[i]] = list[i];
		if (player_plist->m_cur_song >= 0)
			player_plist->m_cur_song = 
				data->m_transform[player_plist->m_cur_song];
		plist_unlock(player_plist);
		free(list);
	}
	player_store_undo = was_store;
} /* End of 'undo_do' function */

/* Undo action */
void undo_undo( struct tag_undo_list_item_t *item )
{
	bool_t was_store = player_store_undo;
	
	if (item == NULL)
		return;

	player_store_undo = FALSE;

	/* Add files action */
	if (item->m_type == UNDO_ADD)
	{
		struct tag_undo_list_add_t *data = &item->m_data.m_add;
		int was_start = player_plist->m_sel_start,
			was_end = player_plist->m_sel_end;

		player_plist->m_sel_start = player_plist->m_len - data->m_num_songs;
		player_plist->m_sel_end = player_plist->m_len - 1;
		plist_rem(player_plist);
		player_plist->m_sel_start = was_start;
		player_plist->m_sel_end = was_end;
		UNDO_FIX_SEL(player_plist);
	}
	/* Add object action */
	else if (item->m_type == UNDO_ADD_OBJ)
	{
		struct tag_undo_list_add_obj_t *data = &item->m_data.m_add_obj;
		int was_start = player_plist->m_sel_start,
			was_end = player_plist->m_sel_end;
		
		player_plist->m_sel_start = player_plist->m_len - data->m_num_songs;
		player_plist->m_sel_end = player_plist->m_len - 1;
		plist_rem(player_plist);
		player_plist->m_sel_start = was_start;
		player_plist->m_sel_end = was_end;
		UNDO_FIX_SEL(player_plist);
	}
	/* Remove action */
	else if (item->m_type == UNDO_REM)
	{
		struct tag_undo_list_rem_t *data = &item->m_data.m_rem;
		int i;
		for ( i = 0; i < data->m_num_files; i ++ )
		{
			struct song_name *sn = &data->m_files[i];
			song_t *song = (sn->m_filename ?
					song_new_from_file(sn->m_filename, &sn->m_metadata) :
					song_new_from_uri(sn->m_fullname, &sn->m_metadata));
			
			plist_add_song(player_plist, song, data->m_start_pos + i);
		}
		plist_flush_scheduled(player_plist);
	}
	/* Move selection action */
	else if (item->m_type == UNDO_MOVE)
	{
		struct tag_undo_list_move_t *data = &item->m_data.m_move_plist;
		int was_start = player_plist->m_sel_start,
			was_end = player_plist->m_sel_end, 
			was_scrolled = player_plist->m_scrolled;
		
		player_plist->m_sel_start = data->m_to;
		player_plist->m_sel_end = data->m_to + data->m_end - data->m_start;
		plist_move_sel(player_plist, data->m_start, FALSE);
		player_plist->m_sel_start = was_start;
		player_plist->m_sel_end = was_end;
		player_plist->m_scrolled = was_scrolled;
		UNDO_FIX_SEL(player_plist);
	}
	/* Sort action */
	else if (item->m_type == UNDO_SORT)
	{
		int i, j;
		struct tag_undo_list_sort_t *data = &item->m_data.m_sort;
		song_t **list = (song_t **)malloc(sizeof(song_t *) * 
				player_plist->m_len);
		plist_lock(player_plist);
		memcpy(list, player_plist->m_list, sizeof(song_t *) * 
				player_plist->m_len);
		for ( i = 0; i < player_plist->m_len; i ++ )
			player_plist->m_list[i] = list[data->m_transform[i]];
		player_plist->m_cur_song = data->m_was_song;
		plist_unlock(player_plist);
		free(list);
	}
	player_store_undo = was_store;
} /* End of 'undo_undo' function */

/* Free list */
void undo_free_list( struct tag_undo_list_item_t *l )
{
	struct tag_undo_list_item_t *t;
	int i;

	if (l == NULL)
		return;

	if (l->m_prev != NULL)
		l->m_prev->m_next = NULL;
	for ( t = l; t != NULL; )
	{
		struct tag_undo_list_item_t *t1 = t->m_next;
		switch (t->m_type)
		{
		case UNDO_ADD:
			plist_set_free(t->m_data.m_add.m_set);
			break;
		case UNDO_ADD_OBJ:
			if (t->m_data.m_add_obj.m_obj_name != NULL)
				free(t->m_data.m_add_obj.m_obj_name);
			break;
		case UNDO_REM:
			if (t->m_data.m_rem.m_files != NULL)
			{
				for ( i = 0; i < t->m_data.m_rem.m_num_files; i ++ )
				{
					struct song_name *sn = &t->m_data.m_rem.m_files[i];
					if (sn->m_fullname)
						free(sn->m_fullname);
					if (sn->m_filename)
						free(sn->m_filename);
				}
				free(t->m_data.m_rem.m_files);
			}
			break;
		case UNDO_SORT:
			if (t->m_data.m_sort.m_transform != NULL)
				free(t->m_data.m_sort.m_transform);
			break;
		}
		free(t);
		t = t1;
	}
} /* End of 'undo_free_list' function */

/* End of 'undo.c' file */

