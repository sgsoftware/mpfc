/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : inp.h
 * PURPOSE     : SG Konsamp. Interface for input plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.07.2003
 * NOTE        : Module prefix 'inp'.
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

#ifndef __SG_KONSAMP_INP_H__
#define __SG_KONSAMP_INP_H__

#include "types.h"
#include "genre_list.h"
#include "song.h"
#include "song_info.h"

/* Plugin functions list structure */
typedef struct
{
	/* Start playing function */
	bool (*m_start)( char *filename );

	/* End playing function */
	void (*m_end)( void );

	/* Get song length function */
	int (*m_get_len)( char *filename );

	/* Get song information function */
	bool (*m_get_info)( char *file_name, song_info_t *info );
	
	/* Save song information function */
	void (*m_save_info)( char *file_name, song_info_t *info );
	
	/* Get supported file formats */
	void (*m_get_formats)( char *buf );
	
	/* Get stream function */
	int (*m_get_stream)( void *buf, int size );

	/* Seek song */
	void (*m_seek)( int shift );

	/* Get song audio parameters */
	void (*m_get_audio_params)( int *channels, int *frequency, dword *fmt );

	/* Set equalizer parameters */
	void (*m_set_eq)( float preamp, float bands[10] );

	/* Genres list */
	genre_list_t *m_glist;
} inp_func_list_t;

/* Input plugin type */
typedef struct tag_in_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Functions list */
	inp_func_list_t m_fl;
} in_plugin_t;

/* Initialize input plugin */
in_plugin_t *inp_init( char *name );

/* Free input plugin object */
void inp_free( in_plugin_t *plugin );

#endif

/* End of 'inp.h' file */

