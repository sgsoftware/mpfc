/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : inp.h
 * PURPOSE     : SG MPFC. Interface for input plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.02.2004
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
#include "mystring.h"
#include "song.h"
#include "song_info.h"

/* Input plugin flags */
#define INP_OWN_OUT 0x00000001
#define INP_OWN_SOUND 0x00000002

/* Special function flags */
#define INP_SPEC_SAVE_INFO 0x00000001

struct tag_pmng_t;

/* Special input plugin function type */
typedef struct
{
	/* Function title */
	char *m_title;

	/* Flags */
	dword m_flags;

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

	/* Get stream function */
	int (*m_get_stream)( void *buf, int size );

	/* Seek song */
	void (*m_seek)( int seconds );

	/* Pause playing */
	void (*m_pause)( void );

	/* Resume playing */
	void (*m_resume)( void );

	/* Get current time */
	int (*m_get_cur_time)( void );

	/* Get song information function */
	song_info_t *(*m_get_info)( char *file_name, int *len );
	
	/* Save song information function */
	void (*m_save_info)( char *file_name, song_info_t *info );
	
	/* Get supported file formats */
	void (*m_get_formats)( char *extensions, char *content_type );
	
	/* Get song audio parameters */
	void (*m_get_audio_params)( int *channels, int *frequency, dword *fmt,
									int *bitrate );

	/* Update equalizer parameters */
	void (*m_set_eq)( void );

	/* Initialize songs array that respects to the object */
	song_t **(*m_init_obj_songs)( char *name, int *num_songs );

	/* Set a title for song with empty song info */
	str_t *(*m_set_song_title)( char *filename );

	/* Set next song name */
	void (*m_set_next_song)( char *name );

	/* Reserved */
	byte m_reserved[68];

	/* Information about plugin */
	char *m_about;

	/* Plugins manager */
	struct tag_pmng_t *m_pmng;

	/* Special functions list */
	int m_num_spec_funcs;
	inp_spec_func_t *m_spec_funcs;

	/* Plugin flags */
	dword m_flags;

	/* Reserved data */
	byte m_reserved1[108];
} inp_func_list_t;

/* Input plugin type */
typedef struct tag_in_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin name */
	char *m_name;

	/* Functions list */
	inp_func_list_t m_fl;
} in_plugin_t;

/* Initialize input plugin */
in_plugin_t *inp_init( char *name, struct tag_pmng_t *pmng );

/* Free input plugin object */
void inp_free( in_plugin_t *plugin );

/* Start playing function */
bool_t inp_start( in_plugin_t *p, char *filename );

/* End playing function */
void inp_end( in_plugin_t *p );

/* Get stream function */
int inp_get_stream( in_plugin_t *p, void *buf, int size );

/* Seek song */
void inp_seek( in_plugin_t *p, int seconds );

/* Pause playing */
void inp_pause( in_plugin_t *p );

/* Resume playing */
void inp_resume( in_plugin_t *p );

/* Get current time */
int inp_get_cur_time( in_plugin_t *p );

/* Get song information function */
song_info_t *inp_get_info( in_plugin_t *p, char *file_name, int *len );
	
/* Save song information function */
void inp_save_info( in_plugin_t *p, char *file_name, song_info_t *info );
	
/* Get supported file formats */
void inp_get_formats( in_plugin_t *p, char *extensions, char *content_type );
	
/* Get song audio parameters */
void inp_get_audio_params( in_plugin_t *p, int *channels, 
							int *frequency, dword *fmt, int *bitrate );

/* Set equalizer parameters */
void inp_set_eq( in_plugin_t *p );

/* Initialize songs array that respects to the object */
song_t **inp_init_obj_songs( in_plugin_t *p, char *name, int *num_songs );

/* Set song title */
str_t *inp_set_song_title( in_plugin_t *p, char *filename );

/* Set next song name */
void inp_set_next_song( in_plugin_t *p, char *name );

/* Get information about plugin */
char *inp_get_about( in_plugin_t *p );

/* Get plugin flags */
dword inp_get_flags( in_plugin_t *p );

/* Get number of special functions */
int inp_get_num_specs( in_plugin_t *p );

/* Get special function title */
char *inp_get_spec_title( in_plugin_t *p, int index );

/* Get special function flags */
dword inp_get_spec_flags( in_plugin_t *p, int index );

/* Call special function */
void inp_spec_func( in_plugin_t *p, int index, char *filename );

#endif

/* End of 'inp.h' file */

