/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : timidi.h
 * PURPOSE     : SG MPFC. Interface for TiMidi (playing midi 
 *               through TiMidity) plugin functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.01.2004
 * NOTE        : Module prefix 'midi'.
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

#ifndef __SG_MPFC_TIMIDI_H__
#define __SG_MPFC_TIMIDI_H__

#include "types.h"
#include "mystring.h"

/* Start playing */
bool_t midi_start( char *filename );

/* End playing */
void midi_end( void );

/* Get supported file formats */
void midi_get_formats( char *extensions, char *content_type );

/* Get audio stream */
int midi_get_stream( void *buf, int size );

/* Seek */
void midi_seek( int shift );

/* Get song audio parameters */
void midi_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate );

/* Get current time */
int midi_get_cur_time( void );

/* Set song title */
str_t *midi_set_song_title( char *filename );

#endif

/* End of 'timidi.h' file */

