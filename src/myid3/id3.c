/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : id3.c
 * PURPOSE     : SG MPFC. ID3 tags handling library functions 
 *               implementation.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "myid3.h"

/* Create a new empty tag */
id3_tag_t *id3_new( void )
{
	id3_tag_t *tag;

	/* Allocate memory */
	tag = (id3_tag_t *)malloc(sizeof(id3_tag_t));
	if (tag == NULL)
		return NULL;

	/* Initialize tags data */
	id3_v1_new(&tag->m_v1);
	id3_v2_new(&tag->m_v2);
	tag->m_version = 2;

	/* Return tag */
	return tag;
} /* End of 'id3_new' function */
	
/* Read tag from file */
id3_tag_t *id3_read( char *filename )
{
	FILE *fd;
	id3_tag_t *tag;
	byte id[3];
	bool_t has_v1 = FALSE, has_v2 = FALSE;

	/* Try to open file */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return NULL;

	/* Find tags */
	fread(id, 1, 3, fd);
	if (!strncmp(id, "ID3", 3))
		has_v2 = TRUE;
	fseek(fd, -ID3_V1_TOTAL_SIZE, SEEK_END);
	fread(id, 1, 3, fd);
	if (!strncmp(id, "TAG", 3))
		has_v1 = TRUE;

	/* No tag found */
	if (!has_v1 && !has_v2)
	{
		fclose(fd);
		return NULL;
	}

	/* Allocate memory for tag */
	tag = (id3_tag_t *)malloc(sizeof(id3_tag_t));
	if (tag == NULL)
		return NULL;
	tag->m_version = (has_v2 ? 2 : 1);

	/* Read tags */
	if (has_v1)
		id3_v1_read(&tag->m_v1, fd);
	else
		id3_v1_new(&tag->m_v1);
	if (has_v2)
		id3_v2_read(&tag->m_v2, fd);
	else
		id3_v2_new(&tag->m_v2);
	
	/* Close file */
	fclose(fd);
	return tag;
} /* End of 'id3_read' function */

/* Save tag to file */
void id3_write( id3_tag_t *tag, char *filename )
{
	if (tag == NULL || filename == NULL)
		return;

	/* Write tags */
	id3_v1_write(&tag->m_v1, filename);
	id3_v2_write(&tag->m_v2, filename);
} /* End of 'id3_write' function */

/* Extract next frame from tag */
void id3_next_frame( id3_tag_t *tag, id3_frame_t *frame )
{
	if (tag == NULL)
		return;

	if (tag->m_version == 1)
		id3_v1_next_frame(&tag->m_v1, frame);
	else
		id3_v2_next_frame(&tag->m_v2, frame);
} /* End of 'id3_next_frame' function */

/* Set frame value */
void id3_set_frame( id3_tag_t *tag, char *name, char *val )
{
	if (tag == NULL || name == NULL || val == NULL)
		return;

	/* Set frame to tags */
	id3_v1_set_frame(&tag->m_v1, name, val);
	id3_v2_set_frame(&tag->m_v2, name, val);
} /* End of 'id3_set_frame' function */

/* Free tag */
void id3_free( id3_tag_t *tag )
{
	if (tag == NULL)
		return;

	if (tag->m_v1.m_stream != NULL)
		free(tag->m_v1.m_stream);
	if (tag->m_v2.m_stream != NULL)
		free(tag->m_v2.m_stream);
	free(tag);
} /* End of 'id3_free' function */

/* Free frame */
void id3_free_frame( id3_frame_t *f )
{
	if (f != NULL && f->m_val != NULL)
		free(f->m_val);
} /* End of 'id3_free_frame' function */

/* Create an empty ID3V1 */
void id3_v1_new( id3_tag_data_t *tag )
{
	/* Allocate memory for stream */
	tag->m_stream = (byte *)malloc(tag->m_stream_len = ID3_V1_TOTAL_SIZE);
	if (tag->m_stream == NULL)
		return;

	/* Fill stream */
	memcpy(tag->m_stream, "TAG", 3);
	memset(tag->m_stream + 3, ' ', tag->m_stream_len - 3);

	/* Set frames data */
	tag->m_frames_start = tag->m_stream + ID3_V1_TITLE_OFFSET;
	tag->m_cur_frame = tag->m_frames_start;
} /* End of 'id3_v1_new' function */

/* Create an empty ID3V2 */
void id3_v2_new( id3_tag_data_t *tag )
{
	/* Allocate memory for stream */
	tag->m_stream_len = ID3_HEADER_SIZE;
	tag->m_stream = (byte *)malloc(tag->m_stream_len);
	if (tag->m_stream == NULL)
		return;

	/* Fill stream */
	memset(tag->m_stream, 0, tag->m_stream_len);
	memcpy(tag->m_stream, "ID3", 3);
	*(tag->m_stream + 3) = 4;

	/* Set frames data */
	tag->m_frames_start = tag->m_stream + ID3_HEADER_SIZE;
	tag->m_cur_frame = tag->m_frames_start;
} /* End of 'id3_v2_new' function */

/* Read ID3V1 */
void id3_v1_read( id3_tag_data_t *tag, FILE *fd )
{
	/* Seek to tag start */
	fseek(fd, -128, SEEK_END);
	
	/* Create an empty tag at first */
	id3_v1_new(tag);

	/* Read data from file */
	fread(tag->m_stream, 1, tag->m_stream_len, fd);
} /* End of 'id3_v1_read' function */

/* Read ID3V2 */
void id3_v2_read( id3_tag_data_t *tag, FILE *fd )
{
	byte flags;
	int real_size, ext_size;
	byte header[ID3_HEADER_SIZE];

	/* Seek to tag start */
	fseek(fd, 0, SEEK_SET);

	/* Read header */
	fread(header, 1, ID3_HEADER_SIZE, fd);
	flags = *(header + 5);
	real_size = ID3_CONVERT_FROM_SYNCHSAFE(*(dword *)(header + 6));

	/* Calculate real size */
	real_size += ID3_HEADER_SIZE;
	if (flags & ID3_HAS_FOOTER)
		real_size += ID3_FOOTER_SIZE;

	/* Allocate memory for stream and read it */
	tag->m_stream_len = real_size;
	tag->m_stream = (byte *)malloc(real_size);
	if (tag->m_stream == NULL)
		return;
	memcpy(tag->m_stream, header, ID3_HEADER_SIZE);
	fread(tag->m_stream + ID3_HEADER_SIZE, 1, 
			tag->m_stream_len - ID3_HEADER_SIZE, fd);

	/* Get extended header size */
	if (flags & ID3_HAS_EXT_HEADER)
		ext_size = ID3_CONVERT_FROM_SYNCHSAFE(
				*(dword *)(tag->m_stream + ID3_HEADER_SIZE));
	else
		ext_size = 0;

	/* Set frames information */
	tag->m_frames_start = tag->m_cur_frame = 
		tag->m_stream + ID3_HEADER_SIZE + ext_size;
} /* End of 'id3_v2_read' function */

/* Save ID3V1 tag to file */
void id3_v1_write( id3_tag_data_t *tag, char *filename )
{
	FILE *fd;
	char magic[3];

	/* Open file */
	fd = fopen(filename, "r+b");
	if (fd == NULL)
		return;

	/* Determine if there exists tag */
	fseek(fd, -ID3_V1_TOTAL_SIZE, SEEK_END);
	fread(magic, 1, 3, fd);
	if (strncmp(magic, "TAG", 3))
		fseek(fd, 0, SEEK_END);
	else
		fseek(fd, -3, SEEK_CUR);

	/* Write tag */
	fwrite(tag->m_stream, 1, ID3_V1_TOTAL_SIZE, fd);

	/* Close file */
	fclose(fd);
} /* End of 'id3_v1_write' function */

/* Save ID3V2 tag to file */
void id3_v2_write( id3_tag_data_t *tag, char *filename )
{
	FILE *fd;
	int file_size;
	byte *data;
	byte magic[4];
	int prev_size;
	int size = tag->m_stream_len;

	/* Open file */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return;

	/* Get information about current tag */
	fread(magic, 1, 3, fd);
	prev_size = 0;
	if (magic[0] == 'I' && magic[1] == 'D' && magic[2] == '3')
	{
		byte s[4];
		byte f;
		fseek(fd, 5, SEEK_SET);
		fread(&f, 1, 1, fd);
		fread(&s, 1, 4, fd);
		prev_size = s[3] | (s[2] << 7) | (s[1] << 14) | (s[0] << 21);
		prev_size += 10;
		if (f & 16)
			prev_size += 10;
	}

	/* Get file size */
	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd) - prev_size;
	data = (byte *)malloc(file_size + size + 10);
	if (data == NULL)
	{
		fclose(fd);
		return;
	}

	/* Fill data */
	fseek(fd, prev_size, SEEK_SET);
	fread(&data[size], 1, file_size, fd);
	memcpy(data, tag->m_stream, size);
	fclose(fd);

	/* Write file */
	fd = fopen(filename, "wb");
	if (fd == NULL)
	{
		free(data);
		return;
	}
	fwrite(data, 1, file_size + size, fd);
	fclose(fd);
	free(data);
} /* End of 'id3_v2_write' function */

/* Read next frame in ID3V1 */
void id3_v1_next_frame( id3_tag_data_t *tag, id3_frame_t *frame )
{
	frame->m_version = 1;
	switch (tag->m_cur_frame - tag->m_stream)
	{
	case ID3_V1_TITLE_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_TITLE);
		id3_copy2frame(frame, &tag->m_cur_frame, ID3_V1_TITLE_SIZE);
		break;
	case ID3_V1_ARTIST_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_ARTIST);
		id3_copy2frame(frame, &tag->m_cur_frame, ID3_V1_ARTIST_SIZE);
		break;
	case ID3_V1_ALBUM_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_ALBUM);
		id3_copy2frame(frame, &tag->m_cur_frame, ID3_V1_ALBUM_SIZE);
		break;
	case ID3_V1_YEAR_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_YEAR);
		id3_copy2frame(frame, &tag->m_cur_frame, ID3_V1_YEAR_SIZE);
		break;
	case ID3_V1_COMMENT_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_COMMENT);
		id3_copy2frame(frame, &tag->m_cur_frame, ID3_V1_COMMENT_SIZE);
		break;
	case ID3_V1_TRACK_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_TRACK);
		frame->m_val = (char *)malloc(4);
		sprintf(frame->m_val, "%d", (int)*(tag->m_cur_frame));
		tag->m_cur_frame += ID3_V1_TRACK_SIZE;
		break;
	case ID3_V1_GENRE_OFFSET:
		strcpy(frame->m_name, ID3_FRAME_GENRE);
		frame->m_val = (char *)malloc(4);
		sprintf(frame->m_val, "%d", (int)*(tag->m_cur_frame));
		tag->m_cur_frame += ID3_V1_GENRE_SIZE;
		break;
	default:
		strcpy(frame->m_name, ID3_FRAME_FINISHED);
		frame->m_val = NULL;
		break;
	}
} /* End of 'id3_v1_next_frame' function */

/* Read next frame in ID3V2 */
void id3_v2_next_frame( id3_tag_data_t *tag, id3_frame_t *frame )
{
	long size;
	word flags;

	/* Check if any frames are left in stream */
	frame->m_version = 2;
	if (!ID3_IS_VALID_FRAME_NAME(tag->m_cur_frame) || 
			(tag->m_cur_frame - tag->m_stream >= tag->m_stream_len))
	{
		strcpy(frame->m_name, ID3_FRAME_FINISHED);
		frame->m_val = NULL;
		return;
	}

	/* Read frame header */
	memcpy(frame->m_name, tag->m_cur_frame, 4);
	frame->m_name[4] = 0;
	tag->m_cur_frame += 4;
	size = *(dword *)tag->m_cur_frame;
	tag->m_cur_frame += 4;
	size = ID3_CONVERT_FROM_SYNCHSAFE(size);
	flags = *(word *)tag->m_cur_frame;
	tag->m_cur_frame += 2;

	/* Handle flags */
	if (flags & ID3_FRAME_GROUPING)
	{
		tag->m_cur_frame ++;
		size --;
	}
	if (flags & ID3_FRAME_ENCRYPTION)
	{
		tag->m_cur_frame ++;
		size --;
	}
	if (flags & ID3_FRAME_DATA_LEN)
	{
		tag->m_cur_frame += 4;
		size -= 4;
	}

	/* Alias frame name */
	if (!strcmp(frame->m_name, "TDRC"))
		strcpy(frame->m_name, ID3_FRAME_YEAR);

	/* Check bounds */
	if (tag->m_cur_frame + size > tag->m_stream + tag->m_stream_len ||
			tag->m_cur_frame < tag->m_stream || size < 0)
	{
		strcpy(frame->m_name, ID3_FRAME_FINISHED);
		frame->m_val = NULL;
		return;
	}

	/* Save frame value */
	if (size)
		tag->m_cur_frame ++;
	id3_copy2frame(frame, &tag->m_cur_frame, size ? size - 1 : size);
} /* End of 'id3_next_frame_v2' function */

/* Set frame value in ID3V1 */
void id3_v1_set_frame( id3_tag_data_t *tag, char *name, char *val )
{
	if (!strcmp(name, ID3_FRAME_TITLE))
		strncpy(tag->m_stream + ID3_V1_TITLE_OFFSET, val, ID3_V1_TITLE_SIZE);
	else if (!strcmp(name, ID3_FRAME_ARTIST))
		strncpy(tag->m_stream + ID3_V1_ARTIST_OFFSET, val, ID3_V1_ARTIST_SIZE);
	else if (!strcmp(name, ID3_FRAME_ALBUM))
		strncpy(tag->m_stream + ID3_V1_ALBUM_OFFSET, val, ID3_V1_ALBUM_SIZE);
	else if (!strcmp(name, ID3_FRAME_YEAR))
		strncpy(tag->m_stream + ID3_V1_YEAR_OFFSET, val, ID3_V1_YEAR_SIZE);
	else if (!strcmp(name, ID3_FRAME_COMMENT))
		strncpy(tag->m_stream + ID3_V1_COMMENT_OFFSET, val, 
				ID3_V1_COMMENT_SIZE);
	else if (!strcmp(name, ID3_FRAME_GENRE))
	{
		byte g;
		g = atoi(val + 1);
		*(tag->m_stream + ID3_V1_GENRE_OFFSET) = g;
	}
} /* End of 'id3_v1_set_frame' function */

/* Set frame value in ID3V2 */
void id3_v2_set_frame( id3_tag_data_t *tag, char *name, char *val )
{
	byte *p;
	bool_t found, finished;
	int len = strlen(val), diff, diff1, diff2, was_len, size, 
		new_size = (len) ? len + 1 : 0;

	/* Set frame position to the beginning */
	p = tag->m_frames_start;
	diff1 = tag->m_frames_start - tag->m_stream;
	diff2 = tag->m_cur_frame - tag->m_stream;

	/* Search for specified frame */
	for ( found = finished = FALSE; !found && !finished; )
	{
		char *id;
		
		/* Read frame header */
		id = p;
		size = ID3_CONVERT_FROM_SYNCHSAFE(*(dword *)(p + 4));

		/* Compare frame ID with needed */
		if (!strncmp(id, name, 4))
			found = TRUE;
		/* Frames finished */
		else if (!ID3_IS_VALID_FRAME_NAME(id) || 
				(p - tag->m_stream >= tag->m_stream_len))
			finished = TRUE;
		/* Move to the next frame */
		else
			p += (10 + size);
	}

	/* Update existing frame */
	diff = p - tag->m_stream;
	was_len = tag->m_stream_len;
	if (found)
	{
		/* Shift tag */
		if (size < new_size)
		{
			tag->m_stream_len += (new_size - size);
			tag->m_stream = (byte *)realloc(tag->m_stream, tag->m_stream_len);
			p = tag->m_stream + diff;
			memmove(p + new_size + 10, p + size + 10, 
					was_len - diff - size - 10);
		}
		else if (size > new_size)
		{
			memmove(p + new_size + 10, p + size + 10, 
					was_len - diff - size - 10);
			tag->m_stream_len += (new_size - size);
			tag->m_stream = (byte *)realloc(tag->m_stream, tag->m_stream_len);
			p = tag->m_stream + diff;
		}
		
		/* Write */
		memset(p, 0, new_size + 10);
		memcpy(p, name, 4);
		*(dword *)(p + 4) = ID3_CONVERT_TO_SYNCHSAFE(new_size);
		memcpy(p + 11, val, len);
		*(dword *)(tag->m_stream + 6) = ID3_CONVERT_TO_SYNCHSAFE(
				tag->m_stream_len - 
				((ID3_GET_FLAGS(tag) & ID3_HAS_FOOTER) ? 20 : 10));
	}
	/* Create new frame */
	else if (finished)
	{
		/* Reallocate memory */
		tag->m_stream_len += (new_size + 10);
		tag->m_stream = (byte *)realloc(tag->m_stream, tag->m_stream_len);

		/* Shift data */
		p = tag->m_stream + diff;
		memmove(p + new_size + 10, p, was_len - diff);

		/* Write frame */
		memset(p, 0, new_size + 10);
		memcpy(p, name, 4);
		*(dword *)(p + 4) = ID3_CONVERT_TO_SYNCHSAFE(new_size);
		memcpy(p + 11, val, len);
		*(dword *)(tag->m_stream + 6) = ID3_CONVERT_TO_SYNCHSAFE(
				tag->m_stream_len - 
				((ID3_GET_FLAGS(tag) & ID3_HAS_FOOTER) ? 20 : 10));
	}

	/* Update frames info */
	tag->m_frames_start = tag->m_stream + diff1;
	tag->m_cur_frame = tag->m_stream + diff2;
} /* End of 'id3_v2_set_frame' function */

/* Remove ending spaces in string */
void id3_rem_end_spaces( char *str, int len )
{
	for ( len --; len >= 0 && str[len] == ' '; len -- );
	str[len + 1] = 0;
} /* End of 'id3_rem_end_spaces' function */

/* Copy string to frame */
void id3_copy2frame( id3_frame_t *f, byte **ptr, int size )
{	
	byte pos = 0;
	int i;
	bool_t zeros_block = FALSE;
	
	if (f == NULL)
		return;

	/* Check for track in v1 frame */
	if (f->m_version == 1 && !strcmp(f->m_name, ID3_FRAME_COMMENT) &&
			(*ptr)[28] == 0 && (*ptr)[29] != 0)
		size --;
	
	/* Seek to the last string in frame */
	for ( i = 0; i < size; i ++ )
	{
		if (!(*ptr)[i])
			zeros_block = TRUE;
		else if (zeros_block)
		{
			pos = i;
			zeros_block = FALSE;
		}
	}
	size -= pos;
	(*ptr) += pos;

	/* Allocate memory */
	f->m_val = (char *)malloc(size + 1);

	/* Copy */
	memcpy(f->m_val, (char *)(*ptr), size);
	id3_rem_end_spaces(f->m_val, size);

	/* Shift frame pointer */
	(*ptr) += size;
} /* End of 'id3_copy2frame' function */

/* Remove tags from file */
void id3_remove( char *filename )
{
	FILE *fd;
	byte *buf;
	int file_size;

	fd = fopen(filename, "rb");
	if (fd == NULL)
		return;
	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	buf = (byte *)malloc(file_size);
	if (buf == NULL)
	{
		fclose(fd);
		return;
	}
	fseek(fd, 0, SEEK_SET);
	fread(buf, 1, file_size, fd);
	fclose(fd);

	/* Find and remove ID3V2 */
	if (file_size >= 10 && !strncmp(buf, "ID3", 3))
	{
		dword size;

		/* Get ID3V2 size */
		size = ID3_CONVERT_FROM_SYNCHSAFE(*(dword *)(buf + 6));

		/* Calculate real size */
		size += ID3_HEADER_SIZE;
		if (*(buf + 5) & ID3_HAS_FOOTER)
			size += ID3_FOOTER_SIZE;
		
		/* Remove tag */
		memmove(buf, buf + size, file_size - size);
		file_size -= size;
	}

	/* Find and remove ID3V1 */
	if (file_size >= ID3_V1_TOTAL_SIZE && 
			!strncmp(buf + file_size - ID3_V1_TOTAL_SIZE, "TAG", 3))
		file_size -= ID3_V1_TOTAL_SIZE;

	/* Save */
	fd = fopen(filename, "wb");
	if (fd == NULL)
	{
		free(buf);
		return;
	}
	fwrite(buf, 1, file_size, fd);
	fclose(fd);
	free(buf);
} /* End of 'id3_remove' function */

/* End of 'id3.c' file */

