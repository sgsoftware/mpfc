/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * SG MPFC. Main types for exporting to plugins.
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

#ifndef __SG_MPFC_MAIN_TYPES_H__
#define __SG_MPFC_MAIN_TYPES_H__

#include <pthread.h>
#include "types.h"
#include "types.h"
#include "mystring.h"
#include "song_info.h"

/* Song flags */
typedef enum
{
	SONG_SCHEDULE = 1 << 0,
	SONG_INFO_READ = 1 << 1,
	SONG_INFO_WRITE = 1 << 2,
	SONG_STATIC_INFO = 1 << 3
} song_flags_t;

typedef int64_t song_time_t;

/* Song type */
typedef struct tag_song_t
{
	/* Song title */
	str_t *m_title;

	/* Full name in an URI form (with prefix and escaped special symbols) */
	char *m_fullname;

	/* Real file name (might be null if the song has been created from an URI) */
	char *m_filename;

	/* Sliced song length */
	song_time_t m_len;

	/* Full song length */
	song_time_t m_full_len;

	/* Song start and end (for projected songs) */
	song_time_t m_start_time, m_end_time;

	/* Song information */
	song_info_t *m_info;

	/* Flags */
	song_flags_t m_flags;

	/* Song object references counter */
	int m_ref_count;

	/* Default title (used when no info is found) */
	char *m_default_title;

	/* Song mutex */
	pthread_mutex_t m_mutex;
} song_t;

static inline int TIME_TO_SECONDS(song_time_t x) { return x / 1000000000LL; }
static inline song_time_t SECONDS_TO_TIME(int x) { return x * 1000000000LL; }

/* Metadata for making a song 
 * All these fields are optional */
typedef struct tag_song_metadata_t
{
	const char *m_title;
	song_time_t m_len;
	song_info_t *m_song_info;
	song_time_t m_start_time;
	song_time_t m_end_time;
} song_metadata_t;

#define SONG_METADATA_EMPTY { NULL, -1, NULL, -1, -1 }

/* Play list type */
typedef struct
{
	/* List start position in the window */
	int m_start_pos;

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

/* Player statuses */
#define PLAYER_STATUS_PLAYING 	0
#define PLAYER_STATUS_PAUSED	1
#define PLAYER_STATUS_STOPPED	2

/* Player context */
typedef struct
{
	/* Current song playing time */
	song_time_t m_cur_time;

	/* Current audio parameters */
	int m_bitrate, m_freq, m_channels;

	/* Player status */
	int m_status;

	/* Current volume */
#define VOLUME_MIN 0.
#define VOLUME_DEF 1.
#define VOLUME_MAX 10.
#define VOLUME_STEP 0.05
	double m_volume;
} player_context_t;

#endif

/* End of 'main_types.h' file */

