/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : wav.c
 * PURPOSE     : SG Konsamp. WAV input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
 * NOTE        : Module prefix 'wav'.
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
#include <string.h>
#include "types.h"
#include "error.h"
#include "inp.h"
#include "song_info.h"

/* WAV header type */
typedef struct
{
	char m_riff[4];
	long m_file_size;
	char m_riff_type[4];
	char m_chunk_id1[4];
	long m_chunk_size1;
	short m_format;
	short m_channels;
	long m_samples_per_sec;
	long m_avg_bytes_per_sec;
	short m_block_align;
	short m_bits_per_sample;
	char m_chunk_id2[4];
	long m_chunk_size2;
} wav_header_t;

/* Currently playing file descriptor and name */
FILE *wav_fd = NULL;
char wav_fname[256];

/* Current seek value */
int wav_seek_val = 0;

/* Current song audio parameters */
int wav_channels = 0, wav_freq = 0, wav_bits = 0, wav_avg_bps = 0;

/* Song length */
int wav_len = 0;

/* Start play function */
bool wav_start( char *filename )
{
	wav_header_t head;
	
	/* Try to open file */
	wav_fd = fopen(filename, "rb");
	if (wav_fd == NULL)
		return FALSE;
	strcpy(wav_fname, filename);

	/* Read WAV file header */
	fread(&head, 1, sizeof(head), wav_fd);

	/* Save song parameters */
	wav_channels = head.m_channels;
	wav_freq = head.m_samples_per_sec;
	wav_bits = head.m_bits_per_sample;
	wav_avg_bps = head.m_avg_bytes_per_sec;
	wav_len = head.m_file_size / head.m_avg_bytes_per_sec;

	wav_seek_val = 0;
	return TRUE;
} /* End of 'wav_start' function */

/* End playing function */
void wav_end( void )
{
	/* Close file */
	if (wav_fd != NULL)
	{
		wav_fname[0] = 0;
		fclose(wav_fd);
		wav_fd = NULL;
	}
} /* End of 'wav_end' function */

/* Get supported formats function */
void wav_get_formats( char *buf )
{
	strcpy(buf, "wav");
} /* End of 'wav_get_formats' function */

/* Get song length */
int wav_get_len( char *filename )
{
	wav_header_t head;
	FILE *fd;

	/* If we are playing this file now - return its length */
	if (!strcmp(filename, wav_fname))
		return wav_len;

	/* Read file header */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return 0;
	fread(&head, 1, sizeof(head), fd);
	fclose(fd);
	return head.m_file_size / head.m_avg_bytes_per_sec;
} /* End of 'wav_get_len' function */

/* Get song information */
bool wav_get_info( char *filename, song_info_t *info )
{
	/* WAV files have no song information */
	return FALSE;
} /* End of 'wav_get_info' function */

/* Get stream function */
int wav_get_stream( void *buf, int size )
{
	if (wav_fd != NULL)
	{
		if (wav_seek_val)
		{
			fseek(wav_fd, wav_seek_val * wav_avg_bps, SEEK_CUR);
			wav_seek_val = 0;
		}
		
		memset(buf, 0, size);
		size = fread(buf, 1, size, wav_fd);
	}
	else
		size = 0;
	return size;
} /* End of 'wav_get_stream' function */

/* Seek song */
void wav_seek( int shift )
{
	if (wav_fd != NULL)
	{
//		wav_seek_val += shift;
	}
} /* End of 'wav_seek' function */

/* Get audio parameters */
void wav_get_audio_params( int *ch, int *freq, int *bits )
{
	*ch = wav_channels;
	*freq = wav_freq;
	*bits = wav_bits;
} /* End of 'wav_get_audio_params' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = wav_start;
	fl->m_end = wav_end;
	fl->m_get_stream = wav_get_stream;
	fl->m_get_len = wav_get_len;
	fl->m_get_info = wav_get_info;
	fl->m_seek = wav_seek;
	fl->m_get_audio_params = wav_get_audio_params;
	fl->m_get_formats = wav_get_formats;
} /* End of 'inp_get_func_list' function */

/* End of 'wav.c' file */

