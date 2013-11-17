/******************************************************************
 * Copyright (C) 2005 - 2011 by SG Software.
 *
 * SG MPFC. Cue format support plugin.
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

//#include "config.h"
#include <ctype.h>
#define __MPFC_OUTER__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include "libcue/libcue.h"
#include "types.h"
#include "plp.h"
#include "pmng.h"
#include "util.h"

/* Plugins manager */
static pmng_t *cue_pmng = NULL;

/* Plugin description */
static char *cue_desc = "Cue format support plugin";

/* Plugin author */
static char *cue_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Logger */
static logger_t *cue_log = NULL;

/* Get supported formats function */
void cue_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "cue");
	if (content_type != NULL)
		strcpy(content_type, "");
} /* End of 'cue_get_formats' function */

static long cue_get_track_begin( Track *track )
{
	/* Use index 1 */
	int index = (track_get_nindex(track) == 1 ? 0 : 1);
	return track_get_start(track) + track_get_index(track, index);
}

static char *cue_get_genre(Cd *cd, Track *track)
{
	char *res;

	res = cdtext_get(PTI_GENRE, track_get_cdtext(track));
	if (res && *res)
		return res;
	res = rem_get(REM_GENRE, track_get_rem(track));
	if (res && *res)
		return res;

	res = cdtext_get(PTI_GENRE, cd_get_cdtext(cd));
	if (res && *res)
		return res;
	res = rem_get(REM_GENRE, cd_get_rem(cd));
	if (res && *res)
		return res;
	return NULL;
}

static char *cue_get_comment(Cd *cd, Track *track)
{
	char *res;

	res = rem_get(REM_COMMENT, track_get_rem(track));
	if (res && *res)
		return res;
	res = rem_get(REM_COMMENT, cd_get_rem(cd));
	if (res && *res)
		return res;
	return NULL;
}

/* Parse playlist and handle its contents */
plp_status_t cue_for_each_item( char *pl_name, void *ctx, plp_func_t f )
{
	/* Load cue sheet */
	FILE *fd = fopen(pl_name, "rt");
	if (!fd)
	{
		logger_error(cue_log, 0, "cue: failed to load cue sheet %s", pl_name);
		return PLP_STATUS_FAILED;
	}
	Cd *cd = cue_parse_file(fd, pl_name);
	if (!cd)
	{
		logger_error(cue_log, 0, "cue: failed to load cue sheet %s", pl_name);
		fclose(fd);
		return PLP_STATUS_FAILED;
	}
	fclose(fd);

	/* Handle tracks */
	int num_tracks = cd_get_ntrack(cd);
	plp_status_t res = PLP_STATUS_OK;
	for ( int i = 1; i <= num_tracks; ++i )
	{
		Track *track = cd_get_track(cd, i);
		char *filename = track_get_filename(track);
		if (!filename)
			continue;

		song_metadata_t metadata = SONG_METADATA_EMPTY;

		/* Set bounds */
		long start = cue_get_track_begin(track);
		long end = -1;
		if (i < num_tracks)
		{
			Track *next_track = cd_get_track(cd, i + 1);
			char *next_name = track_get_filename(next_track);
			if (next_name && !strcmp(filename, next_name))
				end = cue_get_track_begin(next_track);
		}
		metadata.m_start_time = SECONDS_TO_TIME(start) / 75;
		metadata.m_end_time = (end < 0 ? -1 : SECONDS_TO_TIME(end) / 75);

		/* Set song info */
		song_info_t *si = si_new();
		si_set_album(si, cdtext_get(PTI_TITLE, cd_get_cdtext(cd)));
		si_set_year(si, rem_get(REM_DATE, cd_get_rem(cd)));
		si_set_artist(si, cdtext_get(PTI_PERFORMER, cd_get_cdtext(cd)));
		si_set_name(si, cdtext_get(PTI_TITLE, track_get_cdtext(track)));
		si_set_genre(si, cue_get_genre(cd, track));
		si_set_comments(si, cue_get_comment(cd, track));

		char track_num_str[10];
		snprintf(track_num_str, sizeof(track_num_str), "%02d", i);
		si_set_track(si, track_num_str);
		metadata.m_song_info = si;

		plp_status_t st = f(ctx, filename, &metadata);
		if (st != PLP_STATUS_OK)
		{
			res = st;
			break;
		}
	}

	/* Free memory */
	cd_delete(cd);
	return res;
} /* End of 'cue_for_each_item' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = cue_desc;
	pd->m_author = cue_author;
	PLIST_DATA(pd)->m_get_formats = cue_get_formats;
	PLIST_DATA(pd)->m_for_each_item = cue_for_each_item;
	cue_pmng = pd->m_pmng;
	cue_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'cue.c' file */

