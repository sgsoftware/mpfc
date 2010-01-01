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
#include "file.h"
#include "inp.h"
#include "mystring.h"
#include "song_info.h"
#include "vfs.h"

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
	int m_cur_time;

	/* Current audio parameters */
	int m_bitrate, m_freq, m_stereo;

	/* Player status */
	int m_status;

	/* Current volume and balance */
	int m_volume;
	int m_balance;
} player_context_t;

#endif

/* End of 'main_types.h' file */

