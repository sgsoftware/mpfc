/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * SG MPFC. 
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
#define __MPFC_OUTER__
#include <FLAC/all.h>
#include <sys/soundcard.h>
#include <string.h>
#include "types.h"
#include "inp.h"
#include "pmng.h"

/* Plugins manager */
static pmng_t *flac_pmng = NULL;

/* Plugin description */
static char *flac_desc = "";

/* Plugin author */
static char *flac_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Logger */
static logger_t *flac_log = NULL;

/* Time to seek */
static int flac_seek_pos = -1;

/* Audio parameters */
static int flac_freq = 0, flac_channels = 0, flac_fmt = 0;
static int flac_max_bs, flac_bpp;

/* Decoder object */
static FLAC__StreamDecoder *flac_decoder = NULL;

/* Buffer for writing decoded stream */
static byte *flac_buf = NULL, *flac_buf_ptr = NULL;
static int flac_buf_size = 0;

/* Current time */
static int flac_cur_time = 0;

/* Type for client data */
typedef struct
{
	bool_t m_only_get_info;
	int m_samplerate;
	int m_fmt;
	int m_channels;
	int m_bpp;
	int m_max_bs;
	int m_length;
} flac_client_data_t;
flac_client_data_t flac_client_data;

/* Write decoded data callback */
FLAC__StreamDecoderWriteStatus flac_write_callback( const FLAC__StreamDecoder *decoder,
		const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *client_data )
{
	int i, j;
	byte *fb = flac_buf;
	flac_client_data_t *cd = (flac_client_data_t *)client_data;

	logger_debug(flac_log, "flac: in flac_write_callback with blocksize %d",
			frame->header.blocksize);

	if (cd->m_only_get_info)
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;

	/* Write to buffer */
	for ( i = 0; i < frame->header.blocksize; i ++ )
	{
		for ( j = 0; j < frame->header.channels; j ++ )
		{
			FLAC__int32 sample = buf[j][i];
			if (frame->header.bits_per_sample == 8)
			{
				(*fb ++) = sample;
				flac_buf_size ++;
			}
			else if (frame->header.bits_per_sample == 16)
			{
				*((int16_t *)fb) = sample;
				fb += 2;
				flac_buf_size += 2;
			}
		}
	}
	logger_debug(flac_log, "flac: flac_buf_size = %d", flac_buf_size);

	/* Update time */
	if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER)
		flac_cur_time = frame->header.number.sample_number / flac_freq;
	else
		flac_cur_time = frame->header.number.frame_number * 
			frame->header.blocksize / flac_freq;
	logger_debug(flac_log, "flac: current time is %d", flac_cur_time);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
} /* End of 'flac_write_callback' function */

/* Process metadata callback */
void flac_metadata_callback( const FLAC__StreamDecoder *decoder,
		const FLAC__StreamMetadata *metadata, void *client_data )
{
	flac_client_data_t *cd = (flac_client_data_t *)client_data;

	logger_debug(flac_log, "flac: flac_metadata_callback with type %d", 
			metadata->type);

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		const FLAC__StreamMetadata_StreamInfo *si = &metadata->data.stream_info;
		unsigned bpp = si->bits_per_sample;

		cd->m_samplerate = si->sample_rate;
		cd->m_channels = si->channels;
		cd->m_max_bs = si->max_blocksize;
		cd->m_bpp = bpp;
		cd->m_length = si->total_samples / si->sample_rate;
		if (bpp == 8)
			cd->m_fmt = AFMT_S8;
		else if (bpp == 16)
			cd->m_fmt = AFMT_S16_LE;
		else
			logger_error(flac_log, 0, "flac: %d bits per sample not supported", bpp);
	}
} /* End of 'flac_metadata_callback' function */

/* Error callback */
void flac_error_callback( const FLAC__StreamDecoder *decoder,
		FLAC__StreamDecoderErrorStatus status, void *client_data )
{
	logger_error(flac_log, 0, "flac: got stream decoder error %d", status);
} /* End of 'flac_error_callback' function */

/* Read file metadata */
FLAC__StreamDecoder *flac_read_metadata( char *filename, void *client_data )
{
	FLAC__StreamDecoderState fds;
	int buf_size;
	FLAC__StreamDecoder *decoder;

	logger_debug(flac_log, "flac: flac_read_metadata(%s)", filename);

	/* Initialize decoder */
	decoder = FLAC__stream_decoder_new();
	logger_debug(flac_log, "flac: FLAC__stream_decoder_new returned %p", 
			decoder);
	if (decoder == NULL)
	{
		logger_error(flac_log, 0, "flac: FLAC__stream_decoder_new failed");
		FLAC__stream_decoder_delete(decoder);
		return NULL;
	}
	if (FLAC__stream_decoder_init_file(decoder,
			filename, flac_write_callback, flac_metadata_callback, 
			flac_error_callback, client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		logger_error(flac_log, 0, "flac: FLAC__stream_decoder_init_file failed");
		FLAC__stream_decoder_delete(decoder);
		return NULL;
	}

	/* Read metadata */
	if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
	{
		logger_error(flac_log, 0, 
				"flac: FLAC__stream_decoder_process_until_end_of_metadata failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return NULL;
	}
	logger_debug(flac_log, "flac: Samplerate: %d, channels: %d, "
			"max blocksize: %d, bpp: %d",
			flac_freq, flac_channels, flac_max_bs, flac_bpp);
	return decoder;
} /* End of 'flac_read_metadata' function */

/* Start play function */
bool_t flac_start( char *filename )
{
	FLAC__StreamDecoderState fds;
	int buf_size;

	logger_debug(flac_log, "flac: flac_start(%s)", filename);

	/* Set default parameters */
	flac_seek_pos = -1;
	flac_freq = 0;
	flac_channels = 0;
	flac_fmt = 0;

	/* Initialize decoder and read metadata */
	flac_client_data.m_only_get_info = FALSE;
	flac_decoder = flac_read_metadata(filename, &flac_client_data);
	if (flac_decoder == NULL)
		return FALSE;
	flac_freq = flac_client_data.m_samplerate;
	flac_channels = flac_client_data.m_channels;
	flac_fmt = flac_client_data.m_fmt;
	flac_bpp = flac_client_data.m_bpp;
	flac_max_bs = flac_client_data.m_max_bs;

	/* Allocate memory for buffer */
	buf_size = flac_max_bs * flac_channels * (flac_bpp / 8);
	logger_debug(flac_log, "flac: allocating buffer of size %d", buf_size);
	flac_buf = (byte *)malloc(buf_size);
	if (flac_buf == NULL)
	{
		logger_error(flac_log, 0, "flac: no enough memory");
		FLAC__stream_decoder_finish(flac_decoder);
		FLAC__stream_decoder_delete(flac_decoder);
		flac_decoder = NULL;
		return FALSE;
	}
	flac_buf_ptr = flac_buf;
	flac_buf_size = 0;
	flac_cur_time = 0;
	return TRUE;
} /* End of 'flac_start' function */

/* End playing function */
void flac_end( void )
{
	flac_seek_pos = -1;

	if (flac_buf != NULL)
	{
		free(flac_buf);
		flac_buf = NULL;
		flac_buf_ptr = NULL;
	}

	if (flac_decoder != NULL)
	{
		FLAC__stream_decoder_finish(flac_decoder);
		FLAC__stream_decoder_delete(flac_decoder);
		flac_decoder = NULL;
	}
} /* End of 'flac_end' function */

/* Get stream function */
int flac_get_stream( void *buf, int size )
{
	int s;
	FLAC__bool res;

	logger_debug(flac_log, "flac: flac_get_stream(%p, %d). flac_buf_size is %d. "
			"decoder is %p", buf, size, flac_buf_size, flac_decoder);

	/* Seek */
	if (flac_seek_pos >= 0)
	{
		flac_buf_ptr = flac_buf;
		flac_buf_size = 0;

		res = FLAC__stream_decoder_seek_absolute(flac_decoder, 
				flac_seek_pos * flac_freq);
		logger_debug(flac_log, "flac: FLAC__stream_decoder_seek_absolute returned %d", res);
		flac_seek_pos = -1;
	}

	/* Read data */
	if (flac_buf_size == 0)
	{
		res = FLAC__stream_decoder_process_single(flac_decoder);
		logger_debug(flac_log, "flac: FLAC__stream_decoder_process_single returned %d", res);
	}

	/* Flush buffer */
	s = (size < flac_buf_size) ? size : flac_buf_size;
	logger_debug(flac_log, "flac: copying %d bytes from buffer", s);
	memcpy(buf, flac_buf_ptr, s);
	flac_buf_ptr += s;
	flac_buf_size -= s;

	/* Check if we have reached buffer end */
	if (flac_buf_size == 0)
	{
		logger_debug(flac_log, "flac: buffer flushed");
		flac_buf_ptr = flac_buf;
	}

	logger_debug(flac_log, "flac: returning %d bytes", s);
	return s;
} /* End of 'flac_get_stream' function */

/* Get audio parameters */
void flac_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = flac_channels;
	*freq = flac_freq;
	*fmt = flac_fmt;
	(*bitrate) = 0;
} /* End of 'flac_get_audio_params' function */

/* Get formats supported */
void flac_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "flac");
} /* End of 'flac_get_formats' function */

/* Check whether vorbis comment from metadata matches type */
bool_t flac_comment_matches( FLAC__StreamMetadata_VorbisComment_Entry *comment, 
		char *name, char **data )
{
	char *entry = comment->entry;
	int len = strlen(name);
	
	if (!strncasecmp(entry, name, len) && (entry[len] == '='))
	{
		(*data) = entry + len + 1;
		return TRUE;
	}
	return FALSE;
} /* End of 'flac_comment_matches' function */

/* Get song info */
song_info_t *flac_get_info( char *file_name, int *len )
{
	FLAC__StreamDecoder *decoder;
	flac_client_data_t cd;
	FLAC__StreamMetadata *tags;
	song_info_t *si = NULL;

	(*len) = 0;

	/* Read file info */
	cd.m_only_get_info = TRUE;
	decoder = flac_read_metadata(file_name, &cd);
	if (decoder == NULL)
	{
		return NULL;
	}
	(*len) = cd.m_length;

	/* Free memory */
	FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);

	/* Read tags */
	if (FLAC__metadata_get_tags(file_name, &tags))
	{
		FLAC__StreamMetadata_VorbisComment *comments;
		int i;
		
		si = si_new();

		comments = &tags->data.vorbis_comment;
		for ( i = 0; i < comments->num_comments; i ++ )
		{
			FLAC__StreamMetadata_VorbisComment_Entry *comment = &comments->comments[i];
			char *data;

			if (flac_comment_matches(comment, "title", &data))
				si_set_name(si, data);
			else if (flac_comment_matches(comment, "artist", &data))
				si_set_artist(si, data);
			else if (flac_comment_matches(comment, "album", &data))
				si_set_album(si, data);
			else if (flac_comment_matches(comment, "tracknumber", &data))
				si_set_track(si, data);
			else if (flac_comment_matches(comment, "date", &data))
				si_set_year(si, data);
			else if (flac_comment_matches(comment, "year", &data))
				si_set_year(si, data);
			else if (flac_comment_matches(comment, "genre", &data))
				si_set_genre(si, data);

		}
	}
	return si;
} /* End of 'flac_get_info' function */

/* Seek */
void flac_seek( int pos )
{
	flac_seek_pos = pos;
} /* End of 'flac_seek' function */

/* Get current time */
int flac_get_cur_time( void )
{
	return flac_cur_time;
} /* End of 'flac_get_cur_time' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = flac_desc;
	pd->m_author = flac_author;
	INP_DATA(pd)->m_start = flac_start;
	INP_DATA(pd)->m_end = flac_end;
	INP_DATA(pd)->m_get_stream = flac_get_stream;
	INP_DATA(pd)->m_get_audio_params = flac_get_audio_params;
	INP_DATA(pd)->m_get_formats = flac_get_formats;
	INP_DATA(pd)->m_get_info = flac_get_info;
	INP_DATA(pd)->m_seek = flac_seek;
	INP_DATA(pd)->m_get_cur_time = flac_get_cur_time;
	flac_pmng = pd->m_pmng;
	flac_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'flac.c' file */

