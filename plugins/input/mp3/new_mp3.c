/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.c
 * PURPOSE     : SG Konsamp. MP3 input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
 * NOTE        : Module prefix 'mp3'.
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

#include <mad.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "error.h"
#include "inp.h"
#include "mp3.h"
#include "song_info.h"

#define MP3_IN_BUF_SIZE (5*8192)

#define MP3_MAD_FIXED_TO_USHORT(fixed) ((fixed) = (fixed) >> \
					(MAD_F_FRACBITS - 15))

/* libmad structures */
struct mad_stream mp3_stream;
struct mad_frame mp3_frame;
struct mad_synth mp3_synth;
mad_timer_t mp3_timer;

/* Input buffer */
byte mp3_in_buf[MP3_IN_BUF_SIZE];

/* Currently opened file */
FILE *mp3_fd = NULL;

/* Decoded frames counter */
dword mp3_frame_count = 0;

/* Samples pointer */
dword mp3_samples = 0;

/* Current seek value */
int mp3_seek_val = 0;

/* Current song audio parameters */
int mp3_channels = 0, mp3_freq = 0, mp3_bits = 0;

/* Start play function */
bool mp3_start( char *filename )
{
	/* Open file */
	mp3_fd = fopen(filename, "rb");
	if (mp3_fd == NULL)
		return FALSE;
	
	/* Initialize libmad structures */
	mp3_frame_count = 0;
	mad_stream_init(&mp3_stream);
	mad_frame_init(&mp3_frame);
	mad_synth_init(&mp3_synth);
	mad_timer_reset(&mp3_timer);

	/* Decode first frame */
	mp3_decode_frame();
	
	/* Save song parameters */
	mp3_channels = 2;
	mp3_freq = 44100;
	mp3_bits = 16;
	mp3_samples = 0;
	mp3_seek_val = 0;
	return TRUE;
} /* End of 'mp3_start' function */

/* End playing function */
void mp3_end( void )
{
	if (mp3_fd != NULL)
	{
		mad_synth_finish(&mp3_synth);
		mad_frame_finish(&mp3_frame);
		mad_stream_finish(&mp3_stream);
		fclose(mp3_fd);
	}
} /* End of 'mp3_end' function */

/* Get supported formats function */
void mp3_get_formats( char *buf )
{
	strcpy(buf, "mp3");
} /* End of 'mp3_get_formats' function */

/* Get song length */
int mp3_get_len( char *filename )
{
	return 0;
} /* End of 'mp3_get_len' function */

/* Get song information */
bool mp3_get_info( char *filename, song_info_t *info )
{
	char id3_tag[128];
	FILE *fd;
	int i;

	/* Read ID3 tag */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return FALSE;
	fseek(fd, -128, SEEK_END);
	fread(id3_tag, sizeof(id3_tag), 1, fd);
	fclose(fd);

	/* Parse tag */
	if (id3_tag[0] != 'T' || id3_tag[1] != 'A' || id3_tag[2] != 'G')
		return FALSE;
	strncpy(info->m_name, &id3_tag[3], 30);
	for ( i = 29; i >= 0 && info->m_name[i] == ' '; i -- );
	info->m_name[i + 1] = 0;
	strncpy(info->m_artist, &id3_tag[33], 30);
	for ( i = 29; i >= 0 && info->m_artist[i] == ' '; i -- );
	info->m_artist[i + 1] = 0;
	
	return TRUE;
} /* End of 'mp3_get_info' function */

/* Get stream function */
int mp3_get_stream( void *buf, int size )
{
	int len = 0;
	byte *out_ptr = (byte *)buf;
	
	if (mp3_fd == NULL)
		return 0;

	/* Decode frame if samples counter is zero */
	if (!mp3_samples)
	{
		/* Fill input buffer */
		if (mp3_stream.buffer == NULL || mp3_stream.error == MAD_ERROR_BUFLEN)
		{
			int read_size, remaining;
			byte *read_start;

			/* Prepare input buffer */
			if (mp3_stream.next_frame != NULL)
			{
				remaining = mp3_stream.bufend - mp3_stream.next_frame;
				memmove(mp3_in_buf, mp3_stream.next_frame, remaining);
				read_start = mp3_in_buf + remaining;
				read_size = MP3_IN_BUF_SIZE - remaining;
			}
			else
			{
				read_size = MP3_IN_BUF_SIZE;
				read_start = mp3_in_buf;
				remaining = 0;
			}

			/* Fill buffer */
			read_size = fread(read_start, 1, read_size, mp3_fd);
			if (feof(mp3_fd))
				return 0;
	
			/* Pipe new buffer to libmad's stream decoder facility */
			mad_stream_buffer(&mp3_stream, mp3_in_buf, read_size + remaining);
			mp3_stream.error = 0;
		}

		/* Decode frame */
		if (mad_frame_decode(&mp3_frame, &mp3_stream))
			return mp3_get_stream(buf, size);

		/* If it is the first frame - get file information */
		if (!mp3_frame_count)
		{
			struct mad_header head = mp3_frame.header;
			FILE *fd;

			mp3_freq = head.samplerate;
			mp3_bits = 16;
			mp3_channels = 2;

		}

		/* Accounting */
		mp3_frame_count ++;
		mad_timer_add(&mp3_timer, mp3_frame.header.duration);

		/* Synthesize PCM samples */
		mad_synth_frame(&mp3_synth, &mp3_frame);
	}

	/* Convert libmad's samples from fixed point format */
	for ( ;mp3_samples < mp3_synth.pcm.length && len < size; mp3_samples ++ )
	{
		word sample;

		/* Left channel */
		sample = MP3_MAD_FIXED_TO_USHORT(mp3_synth.pcm.samples[0][mp3_samples]);
		*(out_ptr ++) = sample & 0xFF;
		*(out_ptr ++) = sample >> 8;
		len += 2;

		/* Right channel */
		if (MAD_NCHANNELS(&mp3_frame.header) == 2)
			sample = MP3_MAD_FIXED_TO_USHORT(
					mp3_synth.pcm.samples[1][mp3_samples]);
		*(out_ptr ++) = sample & 0xFF;
		*(out_ptr ++) = sample >> 8;
		len += 2;

	}

	if (mp3_samples >= mp3_synth.pcm.length)
		mp3_samples = 0;

	return len;
} /* End of 'mp3_get_stream' function */

/* Seek song */
void mp3_seek( int shift )
{
} /* End of 'mp3_seek' function */

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, int *bits )
{
	*ch = mp3_channels;
	*freq = mp3_freq;
	*bits = mp3_bits;
} /* End of 'mp3_get_audio_params' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = mp3_start;
	fl->m_end = mp3_end;
	fl->m_get_stream = mp3_get_stream;
	fl->m_get_len = mp3_get_len;
	fl->m_get_info = mp3_get_info;
	fl->m_seek = mp3_seek;
	fl->m_get_audio_params = mp3_get_audio_params;
	fl->m_get_formats = mp3_get_formats;
} /* End of 'inp_get_func_list' function */

/* Decode a frame */
void mp3_decode_frame( void )
{
} /* End of 'mp3_decode_frame' function */

/* End of 'mp3.c' file */

