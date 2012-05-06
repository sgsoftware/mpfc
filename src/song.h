/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for songs manipulation functions.
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

#ifndef __SG_MPFC_SONG_H__
#define __SG_MPFC_SONG_H__

#include <pthread.h>
#include "types.h"
#include "file.h"
#include "inp.h"
#include "main_types.h"
#include "mystring.h"
#include "song_info.h"

/* Create a new song */
song_t *song_new_from_file( char *file, song_metadata_t *metadata );

/* Create a new song */
song_t *song_new_from_uri( char *uri, song_metadata_t *metadata);

/* Add a reference to the song object */
song_t *song_add_ref( song_t *song );

/* Free song */
void song_free( song_t *song );

/* Update song information */
void song_update_info( song_t *song );

/* Fill song title from data from song info and other parameters */
void song_update_title( song_t *song );

/* Write song info */
void song_write_info( song_t *song );

/* Get song file name or full name if it's uri-based */
static inline char* song_get_name( song_t *song )
{
	char *name = song->m_filename;
	if (!name)
		name = song->m_fullname;
	return name;
}

/* Get short filename but only if it is not uri-based */
char* song_get_short_name( song_t *s );

/* Get input plugin */
in_plugin_t *song_get_inp( song_t *song, file_t **fd );

/* Lock/unlock song */
#define song_lock(s) (pthread_mutex_lock(&((s)->m_mutex)))
#define song_unlock(s) (pthread_mutex_unlock(&((s)->m_mutex)))

#endif

/* End of 'song.h' file */

