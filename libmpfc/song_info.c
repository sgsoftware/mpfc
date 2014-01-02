/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Song info management functions implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "pmng.h"
#include "song_info.h"

/* Initialize song info */
song_info_t *si_new( void )
{
	song_info_t *si;

	/* Allocate memory */
	si = (song_info_t *)malloc(sizeof(song_info_t));
	if (si == NULL)
		return NULL;

	/* Set empty fields */
	memset(si, 0, sizeof(*si));
	si->m_name = strdup("");
	si->m_artist = strdup("");
	si->m_album = strdup("");
	si->m_year = strdup("");
	si->m_track = strdup("");
	si->m_comments = strdup("");
	si->m_own_data = strdup("");
	si->m_genre = strdup("");
	return si;
} /* End of 'si_new' function */

/* Duplicate song info */
song_info_t *si_dup( song_info_t *info )
{
	song_info_t *si;
	
	if (info == NULL)
		return NULL;

	/* Allocate memory */
	si = (song_info_t *)malloc(sizeof(*si));
	if (si == NULL)
		return NULL;

	/* Copy fields */
	memset(si, 0, sizeof(*si));
	si->m_name = strdup(info->m_name);
	si->m_artist = strdup(info->m_artist);
	si->m_album = strdup(info->m_album);
	si->m_year = strdup(info->m_year);
	si->m_track = strdup(info->m_track);
	si->m_comments = strdup(info->m_comments);
	si->m_genre = strdup(info->m_genre);
	si->m_own_data = strdup(info->m_own_data);
	si->m_flags = info->m_flags;
	return si;
} /* End of 'si_dup' function */

/* Free song info */
void si_free( song_info_t *si )
{
	if (si == NULL)
		return;

	/* Free memory */
	free(si->m_name);
	free(si->m_artist);
	free(si->m_album);
	free(si->m_year);
	free(si->m_track);
	free(si->m_comments);
	free(si->m_own_data);
	free(si->m_genre);
	free(si);
} /* End of 'si_free' function */

/* Set song name */
void si_set_name( song_info_t *si, const char *name )
{
	if (si == NULL)
		return;

	free(si->m_name);
	si->m_name = strdup(name == NULL ? "" : name);
	if (name != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_name' function */

/* Set artist name */
void si_set_artist( song_info_t *si, const char *artist )
{
	if (si == NULL)
		return;

	free(si->m_artist);
	si->m_artist = strdup(artist == NULL ? "" : artist);
	if (artist != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_artist' function */

/* Set album name */
void si_set_album( song_info_t *si, const char *album )
{
	if (si == NULL)
		return;

	free(si->m_album);
	si->m_album = strdup(album == NULL ? "" : album);
	if (album != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_album' function */

/* Set year */
void si_set_year( song_info_t *si, const char *year )
{
	if (si == NULL)
		return;

	free(si->m_year);
	si->m_year = strdup(year == NULL ? "" : year);
	if (year != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_year' function */

/* Set track */
void si_set_track( song_info_t *si, const char *track )
{
	if (si == NULL)
		return;

	free(si->m_track);
	si->m_track = strdup(track == NULL ? "" : track);
	if (track != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_track' function */

/* Set comments */
void si_set_comments( song_info_t *si, const char *comments )
{
	if (si == NULL)
		return;

	free(si->m_comments);
	si->m_comments = strdup(comments == NULL ? "" : comments);
	if (comments != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_comments' function */

/* Set genre */
void si_set_genre( song_info_t *si, const char *genre )
{
	if (si == NULL)
		return;

	free(si->m_genre);
	si->m_genre = strdup(genre == NULL ? "" : genre);
	if (genre != NULL)
		si->m_flags |= SI_INITIALIZED;
} /* End of 'si_set_genre' function */

/* Set own data */
void si_set_own_data( song_info_t *si, const char *own_data )
{
	if (si == NULL)
		return;

	free(si->m_own_data);
	si->m_own_data = strdup(own_data == NULL ? "" : own_data);
} /* End of 'si_set_own_data' function */

/* End of 'song_info.h' file */

