/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : song.c
 * PURPOSE     : SG MPFC. Songs manipulation functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.03.2004
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
#include "error.h"
#include "file.h"
#include "inp.h"
#include "mystring.h"
#include "player.h"
#include "pmng.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* Create a new song */
song_t *song_new( char *filename, char *title, int len )
{
	song_t *song;
	char *ext;
	in_plugin_t *inp;

	/* Get file extension */
	ext = util_extension(filename);

	/* Choose appropriate input plugin */
	inp = pmng_search_format(player_pmng, ext);
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
	strncpy(song->m_file_name, filename, sizeof(song->m_file_name));
	song->m_file_name[sizeof(song->m_file_name) - 1] = 0;
	//util_rem_slashes(song->m_file_name);
	song->m_file_ext = ext;
	song->m_short_name = util_short_name(song->m_file_name);
	song->m_info = NULL;
	song->m_inp = inp;
	song->m_flags = 0;
	song->m_len = len;
	song->m_title = NULL;
	if (title == NULL)
		song_update_title(song);
	else
		song->m_title = str_new(title);
	return song;
} /* End of 'song_new' function */

/* Free song */
void song_free( song_t *song )
{
	if (song == NULL)
		return;

	str_free(song->m_title);
	si_free(song->m_info);
	free(song);
} /* End of 'song_free' function */

/* Update song information */
void song_update_info( song_t *song )
{
	if (song == NULL || (song->m_flags & SONG_SAVE_INFO))
		return;

	si_free(song->m_info);
	song->m_info = inp_get_info(song->m_inp, song->m_file_name, 
			&song->m_len);
	song_update_title(song);
} /* End of 'song_update_info' function */

/* Fill song title from data from song info and other parameters */
void song_update_title( song_t *song )
{
	char *fmt;
	str_t *str;
	bool_t finish = FALSE;
	song_info_t *info;

	if (song == NULL)
		return;

	/* Free current title */
	str_free(song->m_title);
	
	/* Case that we have no info */
	info = song->m_info;
	if (info == NULL || !(info->m_flags & SI_INITIALIZED) ||
			(info->m_flags & SI_ONLY_OWN))
	{
		song->m_title = inp_set_song_title(song->m_inp, song->m_file_name);
		if (cfg_get_var_int(cfg_list, "convert-underscores2spaces"))
			str_replace_char(song->m_title, '_', ' ');
		return;
	}

	/* Use specified title format */
	fmt = cfg_get_var(cfg_list, "title-format");
	str = song->m_title = str_new("");
	if (fmt != NULL)
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
					str_cat_cptr(str, info->m_artist);
					break;
				case 'a':
					str_cat_cptr(str, info->m_album);
					break;
				case 'f':
					str_cat_cptr(str, song->m_short_name);
					break;
				case 'F':
					str_cat_cptr(str, song->m_file_name);
					break;
				case 'e':
					str_cat_cptr(str, song->m_file_ext);
					break;
				case 't':
					str_cat_cptr(str, info->m_name);
					break;
				case 'n':
					str_cat_cptr(str, info->m_track);
					break;
				case 'y':
					str_cat_cptr(str, info->m_year);
					break;
				case 'g':
					str_cat_cptr(str, info->m_genre);
					break;
				case 'c':
					str_cat_cptr(str, info->m_comments);
					break;
				case 0:
					finish = TRUE;
					break;
				}
			}
			else
			{
				str_insert_char(str, *fmt, str->m_len);
			}
		}
	}
	else
	{
		str_printf(str, "%s - %s", info->m_artist, info->m_name);
	}
} /* End of 'song_get_title_from_info' function */

/* Get input plugin */
in_plugin_t *song_get_inp( song_t *song )
{
	/* Do nothing if we already no plugin */
	if (song->m_inp != NULL)
		return song->m_inp;

	/* Choose appropriate input plugin */
	song->m_inp = pmng_search_format(player_pmng, song->m_file_ext);
	if (song->m_inp == NULL)
	{
		file_t *fd = file_open(song->m_file_name, "rb", player_print_msg);
		if (fd == NULL)
			return NULL;
		song->m_inp = pmng_search_content_type(player_pmng,
				file_get_content_type(fd));
		file_close(fd);
	}
	return song->m_inp;
} /* End of 'song_get_inp' function */

/* End of 'song.c' file */

