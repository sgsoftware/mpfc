/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : song.c
 * PURPOSE     : SG MPFC. Songs manipulation functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.09.2003
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "codepages.h"
#include "error.h"
#include "inp.h"
#include "pmng.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* Create a new song */
song_t *song_new( char *filename, char *title, int len )
{
	song_t *song;
	song_info_t si;
	int i;
	char *ext;
	in_plugin_t *inp;

	/* Get song extension */
	ext = util_get_ext(filename);

	/* Choose appropriate input plugin */
	inp = pmng_search_format(ext);
	if (inp == NULL)
		return NULL;
	
	/* Try to allocate memory for new song */
	song = (song_t *)malloc(sizeof(song_t));
	if (song == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Set song fields */
	strcpy(song->m_file_name, filename);
	song->m_info = NULL;
	song->m_inp = inp;
	if (title == NULL)
		song_set_info(song, NULL);
	else
	{
		strcpy(song->m_title, title);
		song->m_info = NULL;
	}
	song->m_len = len;
	return song;
} /* End of 'song_new' function */

/* Free song */
void song_free( song_t *song )
{
	if (song != NULL)
	{
		if (song->m_info != NULL)
			free(song->m_info);
		free(song);
	}
} /* End of 'song_free' function */

/* Save song information */
void song_set_info( song_t *song, song_info_t *info )
{
	if (song == NULL)
		return;

	/* Copy info */
	if (info == NULL)
	{
		song->m_info = (song_info_t *)malloc(sizeof(song_info_t));
		memset(song->m_info, 0, sizeof(song_info_t));
		song->m_info->m_genre = GENRE_ID_UNKNOWN;
		song->m_info->m_loaded = FALSE;

	}
	else
	{
		if (song->m_info == NULL)
			song->m_info = (song_info_t *)malloc(sizeof(song_info_t));
		memcpy(song->m_info, info, sizeof(song_info_t));
		song->m_info->m_loaded = TRUE;
	}

	/* Set title */
	if (info == NULL || info->m_only_own)
	{
		int i;
		for ( i = strlen(song->m_file_name) - 1; 
				(i >= 0) && (song->m_file_name[i] != '/'); i -- );
		strcpy(song->m_title, &song->m_file_name[i + 1]);
	}
	else
	{
		sprintf(song->m_title, "%s - %s", info->m_artist, info->m_name);
	}
} /* End of 'song_set_info' function */

/* Update song information */
void song_update_info( song_t *song )
{
	song_info_t si;

	memset(&si, 0, sizeof(si));
	if (inp_get_info(song->m_inp, song->m_file_name, &si))
	{
		/* Convert codepages */
		cp_to_out(si.m_artist, si.m_artist);
		cp_to_out(si.m_name, si.m_name);
		cp_to_out(si.m_album, si.m_album);
		cp_to_out(si.m_comments, si.m_comments);
		cp_to_out(si.m_track, si.m_track);
		cp_to_out(si.m_year, si.m_year);
		cp_to_out(si.m_own_data, si.m_own_data);
		
		song_set_info(song, &si);
	}
	else
		song_set_info(song, NULL);
} /* End of 'song_update_info' function */

/* Initialize song info and length */
void song_init_info_and_len( song_t *song )
{
	if (song == NULL)
		return;

	song->m_len = inp_get_len(song->m_inp, song->m_file_name);
	song_update_info(song);	
} /* End of 'song_init_info_and_len' function */

/* End of 'song.c' file */

