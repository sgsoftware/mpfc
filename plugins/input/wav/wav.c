/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wav.c
 * PURPOSE     : SG MPFC. WAV input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.02.2004
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
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include "types.h"
#include "inp.h"
#include "file.h"
#include "song_info.h"
#include "pmng.h"
#include "util.h"
#include "wav.h"

/* Currently playing file descriptor and name */
static file_t *wav_fd = NULL;
static char wav_fname[MAX_FILE_NAME];

/* Current seek value */
static int wav_seek_val = -1;

/* Current song audio parameters */
static int wav_channels = 0, wav_freq = 0, wav_avg_bps = 0;
static dword wav_fmt = 0;
static int wav_file_size = 0;
static int wav_block_align = 0;

/* Song length */
static int wav_len = 0;

/* Data offset in file */
static int wav_data_offset = 0;

/* Current time */
static int wav_time = 0;

/* Plugin description */
static char *wav_desc = "WAV files playback plugin";

/* Plugin author */
static char *wav_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Start play function */
bool_t wav_start( char *filename )
{
	char riff[4], riff_type[4];
	long file_size;
	void *buf = NULL;
	dword data_size = 0;
	
	/* Try to open file */
	wav_fd = file_open(filename, "rb", NULL);
	if (wav_fd == NULL)
		return FALSE;
	util_strncpy(wav_fname, filename, sizeof(wav_fname));

	/* Read WAV file header */
	file_read(riff, 1, sizeof(riff), wav_fd);
	file_read(&wav_file_size, 1, sizeof(file_size), wav_fd);
	file_read(riff_type, 1, sizeof(riff_type), wav_fd);

	/* Check file validity */
	if (riff[0] != 'R' || riff[1] != 'I' || riff[2] != 'F' || riff[3] != 'F' ||
			riff_type[0] != 'W' || riff_type[1] != 'A' ||
			riff_type[2] != 'V' || riff_type[3] != 'E')
	{
		wav_end();
		return FALSE;
	}

	/* Read chunks until 'data' */
	while (!wav_read_next_chunk(wav_fd, (void **)(&buf), &data_size));

	/* Check format */
	if (!data_size || buf == NULL || 
			!(WAV_FMT_GET_FORMAT(buf) == 1))
	{
		free(buf);
		wav_end();
		return FALSE;
	}

	/* Save song parameters */
	wav_data_offset = file_tell(wav_fd);
	wav_channels = WAV_FMT_GET_CHANNELS(buf);
	wav_freq = WAV_FMT_GET_SAMPLES_PER_SEC(buf);
	wav_avg_bps = WAV_FMT_GET_AVG_BYTES_PER_SEC(buf);
	wav_block_align = WAV_FMT_GET_BLOCK_ALIGN(buf);
	switch (WAV_FMT_GET_FORMAT(buf))
	{
	case 1:
		wav_fmt = (WAV_PCM_FMT_GET_BPS(buf) == 8) ?
			AFMT_U8 : AFMT_S16_LE;
		break;
	}

	wav_len = data_size / wav_avg_bps;
	wav_seek_val = -1;
	wav_time = 0;
	free(buf);
	return TRUE;
} /* End of 'wav_start' function */

/* End playing function */
void wav_end( void )
{
	/* Close file */
	if (wav_fd != NULL)
	{
		strcpy(wav_fname, "");
		file_close(wav_fd);
		wav_time = 0;
		wav_fd = NULL;
	}
} /* End of 'wav_end' function */

/* Get supported formats function */
void wav_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "wav");
	if (content_type != NULL)
		strcpy(content_type, "audio/wav");
} /* End of 'wav_get_formats' function */

/* Get stream function */
int wav_get_stream( void *buf, int size )
{
	if (wav_fd != NULL)
	{
		if (wav_seek_val != -1)
		{
			file_seek(wav_fd, wav_data_offset + wav_seek_val * wav_avg_bps, 
					SEEK_SET);
			wav_seek_val = -1;
		}
		
		memset(buf, 0, size);
		size = file_read(buf, 1, size, wav_fd);
		wav_time = (file_tell(wav_fd) - wav_data_offset) / wav_avg_bps;
	}
	else
		size = 0;
	return size;
} /* End of 'wav_get_stream' function */

/* Seek song */
void wav_seek( int seconds )
{
	if (wav_fd != NULL)
	{
		wav_seek_val = seconds;
	}
} /* End of 'wav_seek' function */

/* Get audio parameters */
void wav_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = wav_channels;
	*freq = wav_freq;
	*fmt = wav_fmt;
	*bitrate = 0;
} /* End of 'wav_get_audio_params' function */

/* Get current time */
int wav_get_cur_time( void )
{
	return wav_time;
} /* End of 'wav_get_cur_time' function */

/* Get song information */
song_info_t *wav_get_info( char *filename, int *length )
{
	int size, samplerate, bps, channels, block_align, bits, len;
	song_info_t *si;
	char str[1024];
	
	/* Get audio parameters */
	if (!strcmp(filename, wav_fname))
	{
		samplerate = wav_freq;
		bps = wav_avg_bps;
		channels = wav_channels;
		block_align = wav_block_align;
		bits = (wav_fmt == AFMT_U8 || wav_fmt == AFMT_S8) ? 8 : 16;
		size = wav_file_size;
		len = wav_len;
	}
	else
	{
		file_t *fd;
		byte *buf;
		dword data_size = 0;

		/* Read file header */
		fd = file_open(filename, "rb", NULL);
		if (fd == NULL)
			return NULL;
		file_seek(fd, 4, SEEK_SET);
		file_read(&size, 1, 4, fd);
		file_seek(fd, 4, SEEK_CUR);
		while (!wav_read_next_chunk(fd, (void **)(&buf), &data_size));
		if (!data_size || buf == NULL || !(WAV_FMT_GET_FORMAT(buf) == 1))
		{
			file_close(fd);
			return NULL;
		}
		file_close(fd);

		/* Get parameters */
		channels = WAV_FMT_GET_CHANNELS(buf);
		samplerate = WAV_FMT_GET_SAMPLES_PER_SEC(buf);
		bps = WAV_FMT_GET_AVG_BYTES_PER_SEC(buf);
		block_align = WAV_FMT_GET_BLOCK_ALIGN(buf);
		bits = WAV_PCM_FMT_GET_BPS(buf);
		len = data_size / bps;
		free(buf);
	}

	/* Initialize info */
	si = si_new();
	si->m_flags |= SI_ONLY_OWN;
	snprintf(str, sizeof(str),
			_("File size: %i bytes\n"
			"Length: %i seconds\n"
			"Bits/sample: %i\n"
			"Format: PCM\n"
			"Channels: %i\n"
			"Samplerate: %i Hz\n"
			"Bytes/sec: %i\n"
			"Block align: %i"),
			size, len, bits, channels, samplerate, bps, block_align);
	si_set_own_data(si, str);
	*length = len;
	return si;
} /* End of 'wav_get_info' function */ 

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = wav_desc;
	pd->m_author = wav_author;
	INP_DATA(pd)->m_start = wav_start;
	INP_DATA(pd)->m_end = wav_end;
	INP_DATA(pd)->m_get_stream = wav_get_stream;
	INP_DATA(pd)->m_seek = wav_seek;
	INP_DATA(pd)->m_get_audio_params = wav_get_audio_params;
	INP_DATA(pd)->m_get_formats = wav_get_formats;
	INP_DATA(pd)->m_get_cur_time = wav_get_cur_time;
	INP_DATA(pd)->m_get_info = wav_get_info;
} /* End of 'plugin_exchange_data' function */

/* Read the next chunk. Returns TRUE when 'data' chunk is read */
static bool_t wav_read_next_chunk( file_t *fd, void **fmt_buf, 
										dword *data_size )
{
	char chunk_id[4];
	long chunk_size;
	
	if (fd == NULL || file_eof(fd))
		return TRUE;

	/* Read chunk header */
	file_read(chunk_id, 1, sizeof(chunk_id), fd);
	file_read(&chunk_size, 1, sizeof(chunk_size), fd);

	/* Parse chunk */
	if (!strncmp(chunk_id, "data", 4))
	{
		(*data_size) = chunk_size;
		return TRUE;
	}
	else if (!strncmp(chunk_id, "fmt ", 4))
	{
		(*fmt_buf) = malloc(chunk_size);
		if ((*fmt_buf) == NULL)
			return FALSE;
		file_read(*fmt_buf, 1, chunk_size, fd);
	}
	else
	{
		file_seek(fd, chunk_size, SEEK_CUR);
	}

	return FALSE;
} /* End of 'wav_read_next_chunk' function */

/* End of 'wav.c' file */

