/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : plist.h
 * PURPOSE     : SG MPFC. Interface for play list manipulation
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.09.2003
 * NOTE        : Module prefix 'plist'.
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
#include "song.h"
#include "window.h"

/* Play list type */
typedef struct
{
	/* List start position and height in the window */
	int m_start_pos, m_height;

	/* Size of scrolled part */
	int m_scrolled;

	/* Selection start and end */
	int m_sel_start, m_sel_end;

	/* Currently playing song */
	int m_cur_song;

	/* Visual mode flag */
	bool_t m_visual;
	
	/* List size */
	int m_len;

	/* Songs list */
	song_t **m_list;

	/* Mutex for synchronization play list operations */
	pthread_mutex_t m_mutex;
} plist_t;

/* Sort criterias */
#define PLIST_SORT_BY_TITLE 0
#define PLIST_SORT_BY_NAME  1
#define PLIST_SORT_BY_PATH  2

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

/* Create a new play list */
plist_t *plist_new( int start_pos, int height );

/* Destroy play list */
void plist_free( plist_t *pl );

/* Add a file to play list (it may be directory) */
bool_t plist_add( plist_t *pl, char *filename );

/* Add single file to play list */
int plist_add_one_file( plist_t *pl, char *filename );

/* Add a song to play list */
int plist_add_song( plist_t *pl, char *filename, char *title, int len, 
		int where );

/* Add a play list file to play list */
int plist_add_list( plist_t *pl, char *filename );

/* Add a directory to play list */
int plist_add_dir( plist_t *pl, char *filename );

/* Low level song adding */
bool_t __plist_add_song( plist_t *pl, char *filename, char *title, int len,
	   int where );

/* Save play list */
bool_t plist_save( plist_t *pl, char *filename );

/* Sort play list */
void plist_sort( plist_t *pl, bool_t global, int criteria );

/* Remove selected songs from play list */
void plist_rem( plist_t *pl );

/* Search for string */
bool_t plist_search( plist_t *pl, char *str, int dir, int criteria );

/* Move cursor in play list */
void plist_move( plist_t *pl, int y, bool_t relative );

/* Move selection in play list */
void plist_move_sel( plist_t *pl, int y, bool_t relative );

/* Centrize view */
void plist_centrize( plist_t *pl );

/* Display play list */
void plist_display( plist_t *pl, wnd_t *wnd );

/* Lock play list */
void plist_lock( plist_t *pl );

/* Unlock play list */
void plist_unlock( plist_t *pl );

/* Add an object */
void plist_add_obj( plist_t *pl, char *name );

/* Set song information */
void plist_set_song_info( plist_t *pl, int index );

/* Reload all songs information */
void plist_reload_info( plist_t *pl );

#endif

/* End of 'plist.h' file */

