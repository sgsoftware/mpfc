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

/* Start playing */
bool_t midi_start( char *filename );

/* End playing */
void midi_end( void );

/* Get song length */
int midi_get_len( char *filename );

/* Get supported file formats */
void midi_get_formats( char *buf );

/* Get audio stream */
int midi_get_stream( void *buf, int size );

/* Seek */
void midi_seek( int shift );

/* Get song audio parameters */
void midi_get_audio_params( int *ch, int *freq, dword *fmt );

/* Get current time */
int midi_get_cur_time( void );

/* Get content type */
void midi_get_content_type( char *buf );

/* Set song title */
void midi_set_song_title( char *title, char *filename );

#endif

/* End of 'timidi.h' file */

