/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wav.h
 * PURPOSE     : SG MPFC. Interface for WAV input plugin functions.
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
#include <string.h>
#include "types.h"
#include "file.h"
#include "inp.h"
#include "song_info.h"

/* Get field from format */
#define WAV_FMT_GET_FORMAT(buf) *(word *)(buf)
#define WAV_FMT_GET_CHANNELS(buf) *(word *)(((byte *)(buf) + 2))
#define WAV_FMT_GET_SAMPLES_PER_SEC(buf) *(dword *)(((byte *)(buf) + 4))
#define WAV_FMT_GET_AVG_BYTES_PER_SEC(buf) *(dword *)(((byte *)(buf) + 8))
#define WAV_FMT_GET_BLOCK_ALIGN(buf) *(word *)(((byte *)(buf) + 12))
#define WAV_PCM_FMT_GET_BPS(buf) *(word *)(((byte *)(buf) + 14))

/* Supported formats */
#define WAV_FMT_PCM 1
#define WAV_FMT_ADPCM 2

/* Check that format is supported */
#define WAV_FMT_SUPPORTED(fmt) ((fmt) <= 2)

/* Start play function */
bool_t wav_start( char *filename );

/* End playing function */
void wav_end( void );

/* Get supported formats function */
void wav_get_formats( char *extensions, char *content_type );

/* Get song information */
song_info_t *wav_get_info( char *filename, int *len );

/* Save song information */
void wav_save_info( char *filename, song_info_t *info );

/* Get stream function */
int wav_get_stream( void *buf, int size );

/* Seek song */
void wav_seek( int seconds );

/* Get audio parameters */
void wav_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate );

/* Read the next chunk. Returns TRUE when 'data' chunk is read */
static bool_t wav_read_next_chunk( file_t *fd, void **fmt_buf, 
										dword *data_size );

/* Read and decode data from ADPCM */
static int wav_read_adpcm( void *buf, int size );

/* End of 'wav.h' file */

