/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : song.c
 * PURPOSE     : SG MPFC. Songs manipulation functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 14.11.2003
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
#include "cfg.h"
#include "codepages.h"
#include "error.h"
#include "file.h"
#include "inp.h"
#include "player.h"
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
	if (inp == NULL && file_get_type(filename) == FILE_TYPE_REGULAR)
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
	util_rem_slashes(song->m_file_name);
	song->m_info = NULL;
	song->m_inp = inp;
	song->m_flags = 0;
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
	}
	else
	{
		if (song->m_info == NULL)
			song->m_info = (song_info_t *)malloc(sizeof(song_info_t));
		memcpy(song->m_info, info, sizeof(song_info_t));
	}

	/* Set title */
	song_get_title_from_info(song);
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
	{
		song_set_info(song, NULL);
		song->m_info->m_not_own_present = si.m_not_own_present;
	}
} /* End of 'song_update_info' function */

/* Initialize song info and length */
void song_init_info_and_len( song_t *song )
{
	if (song == NULL)
		return;

	song->m_len = inp_get_len(song->m_inp, song->m_file_name);
	song_update_info(song);	
} /* End of 'song_init_info_and_len' function */

/* Fill song title from data from song info and other parameters */
void song_get_title_from_info( song_t *song )
{
	char *fmt;
	bool_t finish = FALSE;
	char *str;
	song_info_t *info;

	if (song == NULL)
		return;
	
	info = song->m_info;
	if (info == NULL || !info->m_not_own_present || info->m_only_own)
	{
		inp_set_song_title(song->m_inp, song->m_title, song->m_file_name);
		if (cfg_get_var_int(cfg_list, "convert-underscores2spaces"))
			util_under2spaces(song->m_title);
		return;
	}

	fmt = cfg_get_var(cfg_list, "title-format");
	str = song->m_title;
	*str = 0;
	if (*fmt && strcmp(fmt, "0") && strcmp(fmt, "1"))
	{
		for ( ; *fmt && !finish; fmt ++ )
		{
			char *g;
			
			if (*fmt == '%')
			{
				fmt ++;
				switch (*fmt)
				{
				case 'p':
					strcat(str, info->m_artist);
					break;
				case 'a':
					strcat(str, info->m_album);
					break;
				case 'f':
					strcat(str, util_get_file_short_name(song->m_file_name));
					break;
				case 'F':
					strcat(str, song->m_file_name);
					break;
				case 'e':
					strcat(str, util_get_ext(song->m_file_name));
					break;
				case 't':
					strcat(str, info->m_name);
					break;
				case 'n':
					strcat(str, info->m_track);
					break;
				case 'y':
					strcat(str, info->m_year);
					break;
				case 'g':
					g = song_get_genre_name(song);
					if (g != NULL)
						strcat(str, g);
					break;
				case 'c':
					strcat(str, info->m_comments);
					break;
				case 0:
					finish = TRUE;
					break;
				}
			}
			else
			{
				int len = strlen(str);
				str[len] = *fmt;
				str[len + 1] = 0;
			}
		}
	}
	else
	{
		sprintf(str, "%s - %s", info->m_artist, info->m_name);
	}
} /* End of 'song_get_title_from_info' function */

/* Get song genre name */
char *song_get_genre_name( song_t *song )
{
	genre_list_t *gl;
	song_info_t *info;
	
	if (song == NULL || ((info = song->m_info) == NULL) ||
			((gl = inp_get_glist(song->m_inp)) == NULL))
		return NULL;
	if (info->m_genre == GENRE_ID_OWN_STRING)
		return info->m_genre_data.m_text;
	else if (info->m_genre != GENRE_ID_UNKNOWN)
		return gl->m_list[info->m_genre].m_name;
	else
		return NULL;
} /* End of 'song_get_genre_name' function */

/* Get input plugin */
in_plugin_t *song_get_inp( song_t *song )
{
	char *ext;

	/* Do nothing if we already no plugin */
	if (song->m_inp != NULL)
		return song->m_inp;

	/* Get song extension */
	ext = util_get_ext(song->m_file_name);

	/* Choose appropriate input plugin */
	song->m_inp = pmng_search_format(ext);
	if (song->m_inp == NULL)
	{
		file_t *fd = file_open(song->m_file_name, "rb", player_print_msg);
		if (fd == NULL)
			return NULL;
		song->m_inp = pmng_search_content_type(file_get_content_type(fd));
		file_close(fd);
	}
	return song->m_inp;
} /* End of 'song_get_inp' function */

/* End of 'song.c' file */

