/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : inp.h
 * PURPOSE     : SG MPFC. Interface for input plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 14.11.2003
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

#ifndef __SG_MPFC_INP_H__
#define __SG_MPFC_INP_H__

#include "types.h"
#include "genre_list.h"
#include "song.h"
#include "song_info.h"

/* Input plugin flags */
#define INP_NO_OUTP 0x00000001

/* Special input plugin function type */
typedef struct
{
	/* Function title */
	char *m_title;

	/* Function */
	void (*m_func)( char *filename );
} inp_spec_func_t;

/* Plugin functions list structure */
typedef struct
{
	/* Start playing function */
	bool_t (*m_start)( char *filename );

	/* End playing function */
	void (*m_end)( void );

	/* Get song length function */
	int (*m_get_len)( char *filename );

	/* Get song information function */
	bool_t (*m_get_info)( char *file_name, song_info_t *info );
	
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

	/* Update equalizer parameters */
	void (*m_set_eq)( void );

	/* Genres list */
	genre_list_t *m_glist;

	/* Plugin flags */
	dword m_flags;

	/* Initialize songs array that respects to the object */
	song_t **(*m_init_obj_songs)( char *name, int *num_songs );

	/* Pause playing */
	void (*m_pause)( void );

	/* Resume playing */
	void (*m_resume)( void );

	/* Get current time */
	int (*m_get_cur_time)( void );

	/* Get content type that respects to this format */
	void (*m_get_content_type)( char *buf );

	/* Message printer */
	void (*m_print_msg)( char *msg );

	/* Special functions list */
	int m_num_spec_funcs;
	inp_spec_func_t *m_spec_funcs;

	/* Set a title for song with empty song info */
	void (*m_set_song_title)( char *title, char *filename );
} inp_func_list_t;

/* Input plugin type */
typedef struct tag_in_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin name */
	char m_name[256];

	/* Functions list */
	inp_func_list_t m_fl;
} in_plugin_t;

/* Initialize input plugin */
in_plugin_t *inp_init( char *name );

/* Free input plugin object */
void inp_free( in_plugin_t *plugin );

/* Start playing function */
bool_t inp_start( in_plugin_t *p, char *filename );

/* End playing function */
void inp_end( in_plugin_t *p );

/* Get song length function */
int inp_get_len( in_plugin_t *p, char *filename );

/* Get song information function */
bool_t inp_get_info( in_plugin_t *p, char *file_name, song_info_t *info );
	
/* Save song information function */
void inp_save_info( in_plugin_t *p, char *file_name, song_info_t *info );
	
/* Get supported file formats */
void inp_get_formats( in_plugin_t *p, char *buf );
	
/* Get stream function */
int inp_get_stream( in_plugin_t *p, void *buf, int size );

/* Seek song */
void inp_seek( in_plugin_t *p, int shift );

/* Get song audio parameters */
void inp_get_audio_params( in_plugin_t *p, int *channels, 
							int *frequency, dword *fmt );

/* Set equalizer parameters */
void inp_set_eq( in_plugin_t *p );

/* Get genre list */
genre_list_t *inp_get_glist( in_plugin_t *p );

/* Get plugin flags */
dword inp_get_flags( in_plugin_t *p );

/* Initialize songs array that respects to the object */
song_t **inp_init_obj_songs( in_plugin_t *p, char *name, int *num_songs );

/* Pause playing */
void inp_pause( in_plugin_t *p );

/* Resume playing */
void inp_resume( in_plugin_t *p );

/* Get current time */
int inp_get_cur_time( in_plugin_t *p );

/* Get content type */
void inp_get_content_type( in_plugin_t *p, char *buf );

/* Get number of special functions */
int inp_get_num_specs( in_plugin_t *p );

/* Get special function title */
char *inp_get_spec_title( in_plugin_t *p, int index );

/* Call special function */
void inp_spec_func( in_plugin_t *p, int index, char *filename );

/* Set song title */
void inp_set_song_title( in_plugin_t *p, char *title, char *filename );

#endif

/* End of 'inp.h' file */

