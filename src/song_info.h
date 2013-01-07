/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for song info management functions.
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

#ifndef __SG_MPFC_SONG_INFO_H__
#define __SG_MPFC_SONG_INFO_H__

#include "types.h"
#include "genre_list.h"

/* Some types */
struct tag_pmng_t;

/* Song information type */
typedef struct tag_song_info_t
{
	char *m_artist;
	char *m_name;
	char *m_album;
	char *m_year;
	char *m_genre;
	char *m_comments;
	char *m_track;
	char *m_own_data;
	char *m_charset;
	genre_list_t *m_glist;
	dword m_flags;
} song_info_t;

/* Song information flags */
#define SI_INITIALIZED 0x00000001
#define SI_ONLY_OWN    0x00000002

/* Initialize song info */
song_info_t *si_new( void );

/* Duplicate song info */
song_info_t *si_dup( song_info_t *si );

/* Free song info */
void si_free( song_info_t *si );

/* Set song name */
void si_set_name( song_info_t *si, const char *name );

/* Set artist name */
void si_set_artist( song_info_t *si, const char *artist );

/* Set album name */
void si_set_album( song_info_t *si, const char *album );

/* Set year */
void si_set_year( song_info_t *si, const char *year );

/* Set track */
void si_set_track( song_info_t *si, const char *track );

/* Set comments */
void si_set_comments( song_info_t *si, const char *comments );

/* Set genre */
void si_set_genre( song_info_t *si, const char *genre );

/* Set own data */
void si_set_own_data( song_info_t *si, const char *own_data );

/* Set charset */
void si_set_charset( song_info_t *si, char *cs );

/* Convert info fields from one charset to another */
void si_convert_cs( song_info_t *si, char *new_cs, struct tag_pmng_t *pmng );

/* Convert one field */
void si_convert_field( song_info_t *si, char **field, char *new_cs, 
							struct tag_pmng_t *pmng );

#endif

/* End of 'song_info.h' file */

