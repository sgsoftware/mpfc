/******************************************************************
 * Copyright (C) 2005 by SG Software.
 *
 * MPFC Cue plugin. Interface for cue sheet functions.
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

#ifndef __SG_MPFC_CUE_SHEET_H__
#define __SG_MPFC_CUE_SHEET_H__

#include "types.h"

typedef struct
{
	/* Title */
	char *m_title;

	/* Performer */
	char *m_performer;

	/* Indices */
#define MAX_INDICES_COUNT 100
	int m_indices[MAX_INDICES_COUNT];
} cue_track_t;

/* Cue sheet */
typedef struct
{
	/* Tracks */
	int m_num_tracks;
	cue_track_t **m_tracks;

	/* Music file name */
	char *m_file_name;
} cue_sheet_t;

/* Cue line tags */
typedef enum
{
	CUE_SHEET_TAG_UNKNOWN,
	CUE_SHEET_TAG_FILE,
	CUE_SHEET_TAG_TITLE,
	CUE_SHEET_TAG_PERFORMER,
	CUE_SHEET_TAG_INDEX,
	CUE_SHEET_TAG_TRACK,
} cue_sheet_tag_t;

/* Parse cue sheet */
cue_sheet_t *cue_sheet_parse( char *file_name );

/* Free cue sheet */
void cue_sheet_free( cue_sheet_t *cs );

/* Add a new track */
cue_track_t *cue_sheet_add_track( cue_sheet_t *cs );

/* Skip whitespace */
void cue_sheet_skip_ws( char **line );

/* Read tag from line */
cue_sheet_tag_t cue_sheet_get_line_tag( char **line );

/* Read a string from line */
char *cue_sheet_get_string( char **line );

/* Read an integer from line */
int cue_sheet_get_int( char **line );

/* Read a timestamp from line */
int cue_sheet_get_timestamp( char **line );

/* Update string in sheet */
void cue_sheet_set_str( char **str, char *value );

#endif

/* End of 'cue_sheet.h' file */

