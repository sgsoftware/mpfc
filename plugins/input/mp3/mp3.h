/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.h
 * PURPOSE     : SG Konsamp. Interface for MP3 input plugin 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.02.2004
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

#include "types.h"
#include "file.h"
#include "song_info.h"

/* Start play function */
bool_t mp3_start( char *filename );

/* End playing function */
void mp3_end( void );

/* Get supported formats function */
void mp3_get_formats( char *extensions, char *content_type );

/* Get stream function */
int mp3_get_stream( void *buf, int size );

/* Seek song */
void mp3_seek( int seconds );

/* Get song information (exported) */
song_info_t *mp3_get_info( char *filename, int *len );

/* Set equalizer parameters */
void mp3_set_eq( void );

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate );

/* Decode a frame */
static void mp3_decode_frame( void );

/* Save ID3 tag */
static void mp3_save_tag( char *filename, byte *tag, int tag_size );

/* Apply equalizer to frame */
static void mp3_apply_eq( void );

/* Buffering read from file */
static int mp3_read( void *ptr, int size, file_t *fd );

/* Read song parameters */
static void mp3_read_song_params( void );

/* Read mp3 file header */
static void mp3_read_header( char *filename, struct mad_header *head );

/* End of 'mp3.h' file */

