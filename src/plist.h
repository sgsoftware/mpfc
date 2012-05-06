/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for play list manipulation functions.
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

#ifndef __SG_MPFC_PLIST_H__
#define __SG_MPFC_PLIST_H__

#include <pthread.h>
#include "types.h"
#include "main_types.h"
#include "plp.h"
#include "song.h"
#include "wnd.h"

/* A set of files for adding */
typedef struct 
{
	/* Whether files in set are patterns */
	bool_t m_patterns;
	
	/* Files */
	struct tag_plist_set_t
	{
		char *m_name;
		struct tag_plist_set_t *m_next;
	} *m_head, *m_tail;
} plist_set_t;

/* Get list height */
#define PLIST_HEIGHT (WND_HEIGHT(player_wnd) - 5)

/* Sort criterias */
#define PLIST_SORT_BY_TITLE 0
#define PLIST_SORT_BY_NAME  1
#define PLIST_SORT_BY_PATH  2
#define PLIST_SORT_BY_TRACK 3

/* Search criterias */
#define PLIST_SEARCH_TITLE		0
#define PLIST_SEARCH_NAME		1
#define PLIST_SEARCH_ARTIST		2
#define PLIST_SEARCH_ALBUM		3
#define PLIST_SEARCH_YEAR		4
#define PLIST_SEARCH_COMMENT	5
#define PLIST_SEARCH_GENRE		6
#define PLIST_SEARCH_TRACK		7
#define PLIST_SEARCH_OWN		8

/* Get real selection start and end */
#define PLIST_GET_SEL(pl, start, end) \
	(((pl)->m_sel_start < (pl)->m_sel_end) ? ((start) = (pl)->m_sel_start, \
	 	(end) = (pl)->m_sel_end) : ((end) = (pl)->m_sel_start, \
	 	(start) = (pl)->m_sel_end))

/* Check if there exists song information */
#define PLIST_HAS_INFO(s) (!((s)->m_info == NULL || \
			(!(s)->m_info->m_not_own_present && \
			 !(*((s)->m_info->m_own_data)))))

/* Create a new play list */
plist_t *plist_new( int start_pos );

/* Destroy play list */
void plist_free( plist_t *pl );

/* Add a file to play list (it may be directory) */
bool_t plist_add( plist_t *pl, char *filename );

/* Add a set of files to play list */
bool_t plist_add_set( plist_t *pl, plist_set_t *set );

/* Add single file to play list */
int plist_add_one_file( plist_t *pl, char *file, song_metadata_t *metadata,
		int where );

int plist_add_uri( plist_t *pl, char *uri );

void plist_add_song( plist_t *pl, song_t *song, int where );

/* Add M3U play list */
int plist_add_m3u( plist_t *pl, char *filename );

/* Add PLS play list */
int plist_add_pls( plist_t *pl, char *filename );

/* Save play list */
bool_t plist_save( plist_t *pl, char *filename );

/* Save play list to M3U format */
bool_t plist_save_m3u( plist_t *pl, char *filename );

/* Save play list to PLS format */
bool_t plist_save_pls( plist_t *pl, char *filename );

/* Compare two songs for sorting */
int plist_song_cmp( song_t *s1, song_t *s2, int criteria );

/* Sort play list with specified bounds */
void plist_sort_bounds( plist_t *pl, int start, int end, int criteria );

/* Sort play list */
void plist_sort( plist_t *pl, bool_t global, int criteria );

/* Remove selected songs from play list */
void plist_rem( plist_t *pl );

/* Clear play list */
void plist_clear( plist_t *pl );

/* Search for string */
bool_t plist_search( plist_t *pl, char *str, int dir, int criteria );

/* Move cursor in play list */
void plist_move( plist_t *pl, int y, bool_t relative );

/* Move selection in play list */
void plist_move_sel( plist_t *pl, int y, bool_t relative );

/* Centrize view */
void plist_centrize( plist_t *pl, int index );

/* Display play list */
void plist_display( plist_t *pl, wnd_t *wnd );

/* Lock play list */
void plist_lock( plist_t *pl );

/* Unlock play list */
void plist_unlock( plist_t *pl );

/* Add an object */
int plist_add_obj( plist_t *pl, char *name, char *title, int where );

/* Set song information */
void plist_set_song_info( plist_t *pl, int index );

/* Reload all songs information */
void plist_reload_info( plist_t *pl, bool_t global );

/* Set info for all scheduled songs */
void plist_flush_scheduled( plist_t *pl );

/* Initialize a set of files for adding */
plist_set_t *plist_set_new( bool_t patterns );

/* Duplicate set */
plist_set_t *plist_set_dup( plist_set_t *set );

/* Free files set */
void plist_set_free( plist_set_t *set );

/* Add a file to set */
void plist_set_add( plist_set_t *set, char *name );

/* Export play list to a json object */
struct json_object *plist_export_to_json( plist_t *pl );

/* Import play list from a json object */
void plist_import_from_json( plist_t *pl, struct json_object *js );

#endif

/* End of 'plist.h' file */

