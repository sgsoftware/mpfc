/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.h
 * PURPOSE     : SG Konsamp. Interface for MP3 input plugin 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 23.04.2003
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

/* Start play function */
bool mp3_start( char *filename );

/* End playing function */
void mp3_end( void );

/* Get supported formats function */
void mp3_get_formats( char *buf );

/* Get song length */
int mp3_get_len( char *filename );

/* Get song information */
bool mp3_get_info( char *filename, song_info_t *info );

/* Get stream function */
int mp3_get_stream( void *buf, int size );

/* Seek song */
void mp3_seek( int shift );

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, dword *fmt );

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl );

/* Decode a frame */
void mp3_decode_frame( void );

/* Save ID3 tag */
void mp3_save_tag( char *filename, char *tag );

/* End of 'mp3.h' file */

