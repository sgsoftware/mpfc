/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : myid3.h
 * PURPOSE     : SG MPFC. Interface for ID3 tags handling library.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.11.2003
 * NOTE        : Module prefix 'id3'.
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

#ifndef __SG_MPFC_MYID3_H__
#define __SG_MPFC_MYID3_H__

#include <stdio.h>
#include "types.h"

/* Tag data */
typedef struct
{
	/* Stream */
	int m_stream_len;
	byte *m_stream;

	/* Frames info */
	byte *m_frames_start, *m_cur_frame;
} id3_tag_data_t;

/* Tag object type */
typedef struct
{
	/* Version number (1 or 2) */
	int m_version;

	/* V1 and V2 tags data */
	id3_tag_data_t m_v1, m_v2;
} id3_tag_t;

/* Frame type */
typedef struct
{
	/* Frame ID */
	char m_name[5];

	/* Value */
	char *m_val;

	/* Tag version */
	byte m_version;
} id3_frame_t;

/* ID3V1 fields offsets and sizes */
#define ID3_V1_TITLE_OFFSET 3
#define ID3_V1_TITLE_SIZE 30
#define ID3_V1_ARTIST_OFFSET (ID3_V1_TITLE_OFFSET+ID3_V1_TITLE_SIZE)
#define ID3_V1_ARTIST_SIZE 30
#define ID3_V1_ALBUM_OFFSET (ID3_V1_ARTIST_OFFSET+ID3_V1_ARTIST_SIZE)
#define ID3_V1_ALBUM_SIZE 30
#define ID3_V1_YEAR_OFFSET (ID3_V1_ALBUM_OFFSET+ID3_V1_ALBUM_SIZE)
#define ID3_V1_YEAR_SIZE 4
#define ID3_V1_COMMENT_OFFSET (ID3_V1_YEAR_OFFSET+ID3_V1_YEAR_SIZE)
#define ID3_V1_COMMENT_SIZE 30
#define ID3_V1_TRACK_OFFSET (ID3_V1_COMMENT_OFFSET+ID3_V1_COMMENT_SIZE - 1)
#define ID3_V1_TRACK_SIZE 1
#define ID3_V1_GENRE_OFFSET (ID3_V1_COMMENT_OFFSET+ID3_V1_COMMENT_SIZE)
#define ID3_V1_GENRE_SIZE 1
#define ID3_V1_TOTAL_SIZE 128

/* Some frame names */
#define ID3_FRAME_TITLE    "TIT2"
#define ID3_FRAME_ARTIST   "TPE1"
#define ID3_FRAME_ALBUM    "TALB"
#define ID3_FRAME_YEAR     "TYER"
#define ID3_FRAME_TRACK    "TRCK"
#define ID3_FRAME_GENRE    "TCON"
#define ID3_FRAME_COMMENT  "COMM"
#define ID3_FRAME_FINISHED ""

/* ID3V2 flags */
#define ID3_HAS_EXT_HEADER 0x40
#define ID3_HAS_FOOTER 0x10

/* ID3V2 frame flags */
#define ID3_FRAME_GROUPING 0x4000
#define ID3_FRAME_ENCRYPTION 0x0400
#define ID3_FRAME_DATA_LEN 0x0100

/* Get ID3V2 flags */
#define ID3_GET_FLAGS(tag) (*(byte *)((tag)->m_stream + 5))

/* ID3V2 structures sizes */
#define ID3_HEADER_SIZE 10
#define ID3_FOOTER_SIZE 10

/* Check if this is a valid frame name */
#define ID3_IS_VALID_FRAME_SYM(c) \
	(((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9'))
#define ID3_IS_VALID_FRAME_NAME(name) \
	(ID3_IS_VALID_FRAME_SYM(*(name)) && \
	 ID3_IS_VALID_FRAME_SYM(*((name) + 1)) && \
	 ID3_IS_VALID_FRAME_SYM(*((name) + 2)) && \
	 ID3_IS_VALID_FRAME_SYM(*((name) + 3)))


/* Convert between synchsafe and normal integers */
#define ID3_CONVERT_FROM_SYNCHSAFE(num) \
	((((num) & 0x7F) << 21) | (((num) & 0x7F00) << 6) | \
	 (((num) & 0x7F0000) >> 9) | (((num) & 0x7F000000) >> 24))
#define ID3_CONVERT_TO_SYNCHSAFE(num) \
	((((num) & 0x7F) << 24) | (((num) & 0x3F80) << 9) | \
	 (((num) & 0x1FC000) >> 6) | (((num) & 0xFE00000) >> 21))

/* Create a new empty tag */
id3_tag_t *id3_new( void );
	
/* Read tag from file */
id3_tag_t *id3_read( char *filename );

/* Write tag to file */
void id3_write( id3_tag_t *tag, char *filename );

/* Extract next frame from tag */
void id3_next_frame( id3_tag_t *tag, id3_frame_t *frame );

/* Set frame value */
void id3_set_frame( id3_tag_t *tag, char *name, char *val );

/* Remove tags from file */
void id3_remove( char *filename );

/* Free tag */
void id3_free( id3_tag_t *tag );

/* Free frame */
void id3_free_frame( id3_frame_t *f );

/* Create an empty ID3V1 */
void id3_v1_new( id3_tag_data_t *tag );

/* Create an empty ID3V2 */
void id3_v2_new( id3_tag_data_t *tag );

/* Read ID3V1 */
void id3_v1_read( id3_tag_data_t *tag, FILE *fd );

/* Read ID3V2 */
void id3_v2_read( id3_tag_data_t *tag, FILE *fd );

/* Read next frame in ID3V1 */
void id3_v1_next_frame( id3_tag_data_t *tag, id3_frame_t *frame );

/* Read next frame in ID3V2 */
void id3_v2_next_frame( id3_tag_data_t *tag, id3_frame_t *frame );

/* Set frame value in ID3V1 */
void id3_v1_set_frame( id3_tag_data_t *tag, char *name, char *val );

/* Set frame value in ID3V2 */
void id3_v2_set_frame( id3_tag_data_t *tag, char *name, char *val );

/* Save ID3V1 tag to file */
void id3_v1_write( id3_tag_data_t *tag, char *filename );

/* Save ID3V2 tag to file */
void id3_v2_write( id3_tag_data_t *tag, char *filename );

/* Remove ending spaces in string */
void id3_rem_end_spaces( char *str, int len );

/* Copy string to frame */
void id3_copy2frame( id3_frame_t *f, byte **ptr, int size );

#endif

/* End of 'myid3.h' file */

