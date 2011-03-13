/******************************************************************
 * Copyright (C) 2005 by SG Software.
 *
 * MPFC Cue plugin. Cue sheet functions implementation.
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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "cue_sheet.h"

/* Parse cue sheet */
cue_sheet_t *cue_sheet_parse( char *file_name )
{
	FILE *fd;
	cue_sheet_t *cs = NULL;
	cue_track_t *cur_track;

	/* Try to open file */
	fd = fopen(file_name, "rt");
	if (fd == NULL)
		goto fail;

	/* Allocate memory */
	cs = (cue_sheet_t *)malloc(sizeof(*cs));
	if (cs == NULL)
		goto fail;
	memset(cs, 0, sizeof(*cs));
	cur_track = cue_sheet_add_track(cs);
	if (cur_track == NULL)
		goto fail;

	/* Parsing loop */
	for ( ;; )
	{
		char line_buf[1024], *line = line_buf;
		cue_sheet_tag_t tag;

		/* Parse a line */
		if (feof(fd))
			break;
		if (fgets(line, sizeof(line_buf), fd) == NULL)
			break;
		tag = cue_sheet_get_line_tag(&line);
		if (tag == CUE_SHEET_TAG_PERFORMER)
		{
			cue_sheet_set_str(&cur_track->m_performer, 
					cue_sheet_get_string(&line));
		}
		else if (tag == CUE_SHEET_TAG_TITLE)
		{
			cue_sheet_set_str(&cur_track->m_title, 
					cue_sheet_get_string(&line));
		}
		else if (tag == CUE_SHEET_TAG_FILE)
		{
			cue_sheet_set_str(&cs->m_file_name, cue_sheet_get_string(&line));
		}
		else if (tag == CUE_SHEET_TAG_TRACK)
		{
			cur_track = cue_sheet_add_track(cs);
			if (cur_track == NULL)
				goto fail;
		}
		else if (tag == CUE_SHEET_TAG_INDEX)
		{
			int index_num, timestamp;
			index_num = cue_sheet_get_int(&line);
			timestamp = cue_sheet_get_timestamp(&line);
			if (index_num >= 0 && index_num < MAX_INDICES_COUNT)
				cur_track->m_indices[index_num] = timestamp;
		}
	}

	/* Close file */
	fclose(fd);
	return cs;
fail:
	if (cs != NULL)
		cue_sheet_free(cs);
	if (fd != NULL)
		fclose(fd);
	return NULL;
} /* End of 'cue_sheet_parse' function */

/* Free cue sheet */
void cue_sheet_free( cue_sheet_t *cs )
{
	int i;

	if (cs == NULL)
		return;

	for ( i = 0; i < cs->m_num_tracks; i ++ )
	{
		cue_track_t *track = cs->m_tracks[i];

		if (track == NULL)
			continue;

		if (track->m_title != NULL)
			free(track->m_title);
		if (track->m_performer != NULL)
			free(track->m_performer);
		free(track);
	}

	if (cs->m_file_name != NULL)
		free(cs->m_file_name);
	if (cs->m_tracks != NULL)
		free(cs->m_tracks);
	free(cs);
} /* End of 'cue_sheet_free' function */

/* Add a new track */
cue_track_t *cue_sheet_add_track( cue_sheet_t *cs )
{
	cue_track_t *track;
	cue_track_t **list;

	/* Allocate memory */
	track = (cue_track_t *)malloc(sizeof(*track));
	if (track == NULL)
		return NULL;
	memset(track, 0, sizeof(*track));

	/* Append to sheet */
	list = (cue_track_t **)realloc(cs->m_tracks, 
			(cs->m_num_tracks + 1) * sizeof(*cs->m_tracks));
	if (list == NULL)
	{
		free(track);
		return NULL;
	}
	cs->m_tracks = list;
	cs->m_tracks[cs->m_num_tracks ++] = track;
	return track;
} /* End of 'cue_sheet_add_track' function */

/* Skip whitespace */
void cue_sheet_skip_ws( char **line )
{
	while (isspace(**line))
		(*line) ++;
} /* End of 'cue_sheet_skip_ws' function */
		
/* Read tag from line */
cue_sheet_tag_t cue_sheet_get_line_tag( char **line )
{
	int i;
	struct 
	{
		char *tag_string;
		cue_sheet_tag_t tag;
	} tags[] = { {"title", CUE_SHEET_TAG_TITLE},
				 {"performer", CUE_SHEET_TAG_PERFORMER},
				 {"index", CUE_SHEET_TAG_INDEX},
				 {"file", CUE_SHEET_TAG_FILE},
				 {"track", CUE_SHEET_TAG_TRACK},
				};

	cue_sheet_skip_ws(line);

	for ( i = 0; i < sizeof(tags) / sizeof(*tags); i ++ )
	{
		char *s = tags[i].tag_string;
		int len = strlen(s);

		if (!strncasecmp(*line, s, len))
		{
			(*line) += len;
			return tags[i].tag;
		}
	}
	return CUE_SHEET_TAG_UNKNOWN;
} /* End of 'cue_sheet_get_line_tag' function */

/* Read a string from line */
char *cue_sheet_get_string( char **line )
{
	int len;
	char *str, *close_quote;

	cue_sheet_skip_ws(line);

	/* Opening quote */
	if ((**line) != '\"')
		return NULL;
	(*line) ++;

	/* Search for closing quote */
	close_quote = strchr(*line, '\"');
	if (close_quote == NULL)
		return NULL;

	/* Make string */
	len = close_quote - (*line);
	str = (char *)malloc(len + 1);
	if (str == NULL)
		return NULL;
	memcpy(str, *line, len);
	str[len] = 0;
	
	/* Advance line pointer */
	(*line) += (len + 1);
	return str;
} /* End of 'cue_sheet_get_string' function */

/* Read an integer from line */
int cue_sheet_get_int( char **line )
{
	int num = 0;

	cue_sheet_skip_ws(line);

	while (isdigit(**line))
	{
		num *= 10;
		num += (**line) - '0';
		(*line) ++;
	}
	return num;
} /* End of 'cue_sheet_get_int' function */

/* Read a timestamp from line */
int cue_sheet_get_timestamp( char **line )
{
	int min, sec, frames;

	min = cue_sheet_get_int(line);
	if ((**line) != ':')
		return -1;
	(*line) ++;
	sec = cue_sheet_get_int(line);
	if ((**line) != ':')
		return -1;
	(*line) ++;
	frames = cue_sheet_get_int(line);
	return (min * 60 + sec) * 75 + frames;
} /* End of 'cue_sheet_get_timestamp' function */

/* Update string in sheet */
void cue_sheet_set_str( char **str, char *value )
{
	if ((*str) != NULL)
		free(*str);

	(*str) = value;
} /* End of 'cue_set_str' function */

/* End of 'cue_sheet.c' file */

