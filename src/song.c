/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : song.c
 * PURPOSE     : SG Konsamp. Songs manipulation functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 20.04.2003
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
	for ( i = strlen(filename) - 1; i >= 0 && filename[i] != '.'; i -- );
	ext = &filename[i + 1];

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
	{
		song_update_info(song);
	}
	else
	{
		strcpy(song->m_title, title);
		song->m_info = NULL;
	}
	song->m_len = len;

	/* Get song length and information */
	if (!len)
		song->m_len = song->m_inp->m_fl.m_get_len(song->m_file_name);
		
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

	if (info == NULL)
	{
		int i;
		
		/*if (song->m_info != NULL)
		{
			free(song->m_info);
			song->m_info = NULL;
		}*/

		song->m_info = (song_info_t *)malloc(sizeof(song_info_t));
		strcpy(song->m_info->m_artist, "");
		strcpy(song->m_info->m_name, "");
		strcpy(song->m_info->m_album, "");
		strcpy(song->m_info->m_year, "");
		strcpy(song->m_info->m_comments, "");
		song->m_info->m_genre = GENRE_ID_UNKNOWN;
		song->m_info->m_genre_data = 0xFF;

		for ( i = strlen(song->m_file_name) - 1; 
				(i >= 0) && (song->m_file_name[i] != '/'); i -- );
		strcpy(song->m_title, &song->m_file_name[i + 1]);
	}
	else
	{
		if (song->m_info == NULL)
			song->m_info = (song_info_t *)malloc(sizeof(song_info_t));
		memcpy(song->m_info, info, sizeof(song_info_t));
		sprintf(song->m_title, "%s - %s", info->m_artist, info->m_name);
	}
} /* End of 'song_set_info' function */

/* Update song information */
void song_update_info( song_t *song )
{
	song_info_t si;

	if (song->m_inp->m_fl.m_get_info(song->m_file_name, &si))
	{
		/* Convert codepages */
		cp_to_out(si.m_artist, si.m_artist);
		cp_to_out(si.m_name, si.m_name);
		cp_to_out(si.m_album, si.m_album);
		cp_to_out(si.m_comments, si.m_comments);
		
		song_set_info(song, &si);
	}
	else
		song_set_info(song, NULL);
} /* End of 'song_update_info' function */

/* End of 'song.c' file */

