/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : song.h
 * PURPOSE     : SG MPFC. Interface for songs manipulation
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 7.03.2003
 * NOTE        : Module prefix 'song'.
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

#include "types.h"
#include "song_info.h"

/* Declare this type */
struct tag_in_plugin_t;

/* Song flags */
#define SONG_SCHEDULE 0x00000001
#define SONG_GET_INFO 0x00000002
#define SONG_SAVE_INFO 0x00000004

/* Song type */
typedef struct
{
	/* Song title */
	char m_title[256];

	/* Song file name */
	char m_file_name[256];

	/* Song length */
	int m_len;

	/* Song information */
	song_info_t *m_info;

	/* Flags */
	dword m_flags;

	/* Input plugin being used to play this song */
	struct tag_in_plugin_t *m_inp;
} song_t;

/* Create a new song */
song_t *song_new( char *filename, char *title, int len );

/* Free song */
void song_free( song_t *song );

/* Save song information */
void song_set_info( song_t *song, song_info_t *info );

/* Update song information */
void song_update_info( song_t *song );

/* Initialize song info and length */
void song_init_info_and_len( song_t *song );

/* Fill song title from data from song info and other parameters */
void song_get_title_from_info( song_t *song );

/* Get song genre name */
char *song_get_genre_name( song_t *song );

/* Get input plugin */
struct tag_in_plugin_t *song_get_inp( song_t *song );

#endif

/* End of 'song.h' file */

