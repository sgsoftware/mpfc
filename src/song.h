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
#include "mystring.h"
#include "song_info.h"
#include "vfs.h"

/* Declare this type */
struct tag_in_plugin_t;

/* Song flags */
typedef enum
{
	SONG_SCHEDULE = 1 << 0,
	SONG_INFO_READ = 1 << 1,
	SONG_INFO_WRITE = 1 << 2
} song_flags_t;

/* Song type */
typedef struct tag_song_t
{
	/* Song title */
	str_t *m_title;

	/* Song full name (i.e. with plugin prefix if it was specified) */
	char *m_full_name;

	/* File name without plugin prefix. This is used for the most part */
	char *m_file_name;

	/* Short name and file extension */
	char *m_short_name, *m_file_ext;

	/* Song length */
	int m_len;

	/* Song start and end (for projected songs) */
	int m_start_time, m_end_time;

	/* Song information */
	song_info_t *m_info;

	/* Flags */
	song_flags_t m_flags;

	/* Song object references counter */
	int m_ref_count;

	/* Default title (used when no info is found) */
	char *m_default_title;

	/* Song we are redirected to */
	struct tag_song_t *m_redirect;

	/* Input plugin being used to play this song */
	in_plugin_t *m_inp;

	/* Song mutex */
	pthread_mutex_t m_mutex;
} song_t;

/* Create a new song */
song_t *song_new( vfs_file_t *file, char *title, int len );

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

/* Get input plugin */
in_plugin_t *song_get_inp( song_t *song, file_t **fd );

/* Lock/unlock song */
#define song_lock(s) (pthread_mutex_lock(&((s)->m_mutex)))
#define song_unlock(s) (pthread_mutex_unlock(&((s)->m_mutex)))

#endif

/* End of 'song.h' file */

