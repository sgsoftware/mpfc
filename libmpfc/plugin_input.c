/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : inp.c
 * PURPOSE     : SG MPFC. Input plugin management functions
 *               implementation.
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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "inp.h"
#include "mystring.h"
#include "pmng.h"
#include "song.h"
#include "util.h"

/* Initialize input plugin */
in_plugin_t *inp_init( char *name, pmng_t *pmng )
{
	in_plugin_t *p;
	void (*fl)( inp_func_list_t * );

	/* Try to allocate memory for plugin object */
	p = (in_plugin_t *)malloc(sizeof(in_plugin_t));
	if (p == NULL)
	{
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_NOW);
	if (p->m_lib_handler == NULL)
	{
//		util_log("error: %s\n", dlerror());
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "inp_get_func_list");
	if (fl == NULL)
	{
		inp_free(p);
		return NULL;
	}
	p->m_name = pmng_create_plugin_name(name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	p->m_fl.m_pmng = pmng;
	fl(&p->m_fl);
	return p;
} /* End of 'inp_init' function */

/* Free input plugin object */
void inp_free( in_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		if (p->m_fl.m_spec_funcs != NULL)
		{
			int i;

			for ( i = 0; i < p->m_fl.m_num_spec_funcs; i ++ )
				if (p->m_fl.m_spec_funcs[i].m_title != NULL)
					free(p->m_fl.m_spec_funcs[i].m_title);
			free(p->m_fl.m_spec_funcs);
		}
		if (p->m_name != NULL)
			free(p->m_name);
		free(p);
	}
} /* End of 'inp_free' function */

/* Start playing function */
bool_t inp_start( in_plugin_t *p, char *filename )
{
	if (p != NULL && (p->m_fl.m_start != NULL))
		return p->m_fl.m_start(filename);
	return FALSE;
} /* End of 'inp_start' function */

/* End playing function */
void inp_end( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_end != NULL))
		p->m_fl.m_end();
} /* End of 'inp_end' function */

/* Get song information function */
song_info_t *inp_get_info( in_plugin_t *p, char *file_name, int *len )
{
	if (p != NULL && (p->m_fl.m_get_info != NULL))
	{
		song_info_t *si = p->m_fl.m_get_info(file_name, len);
		if (si != NULL)
			si_convert_cs(si, cfg_get_var(pmng_get_cfg(p->m_fl.m_pmng), 
						"charset-output"), p->m_fl.m_pmng);
		return si;
	}
	*len = 0;
	return NULL;
} /* End of 'inp_get_info' function */
	
/* Save song information function */
void inp_save_info( in_plugin_t *p, char *file_name, song_info_t *info )
{
	if (p != NULL && (p->m_fl.m_save_info != NULL) && info != NULL)
	{
		/* Convert charset */
		char *was_cs = (info->m_charset == NULL ? NULL : 
				strdup(info->m_charset));
		pmng_t *pmng = p->m_fl.m_pmng;
		
		si_convert_cs(info, cfg_get_var(pmng_get_cfg(pmng), 
					"charset-save-info"), pmng);
		p->m_fl.m_save_info(file_name, info);
		si_convert_cs(info, was_cs, pmng);
		if (was_cs != NULL)
			free(was_cs);
	}
} /* End of 'inp_save_info' function */
	
/* Get supported file formats */
void inp_get_formats( in_plugin_t *p, char *extensions, char *content_type )
{
	if (p != NULL && (p->m_fl.m_get_formats != NULL))
		p->m_fl.m_get_formats(extensions, content_type);
	else
	{
		if (extensions != NULL)
			strcpy(extensions, "");
		if (content_type != NULL)
			strcpy(content_type, "");
	}
} /* End of 'inp_get_formats' function */
	
/* Get stream function */
int inp_get_stream( in_plugin_t *p, void *buf, int size )
{
	if (p != NULL && (p->m_fl.m_get_stream != NULL))
		return p->m_fl.m_get_stream(buf, size);
	return 0;
} /* End of 'inp_get_stream' function */

/* Seek song */
void inp_seek( in_plugin_t *p, int seconds )
{
	if (p != NULL && (p->m_fl.m_seek != NULL))
		p->m_fl.m_seek(seconds);
} /* End of 'inp_seek' function */

/* Get song audio parameters */
void inp_get_audio_params( in_plugin_t *p, int *channels, 
							int *frequency, dword *fmt, int *bitrate )
{
	if (p != NULL && (p->m_fl.m_get_audio_params != NULL))
		p->m_fl.m_get_audio_params(channels, frequency, fmt, bitrate);
	else 
	{
		*channels = 0;
		*frequency = 0;
		*fmt = 0;
		*bitrate = 0;
	}
} /* End of 'inp_get_audio_params' function */

/* Set equalizer parameters */
void inp_set_eq( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_set_eq != NULL))
		p->m_fl.m_set_eq();
} /* End of 'inp_set_eq' function */

/* Get plugin flags */
dword inp_get_flags( in_plugin_t *p )
{
	if (p != NULL)
		return p->m_fl.m_flags;
	return 0;
} /* End of 'inp_get_flags' function */

/* Initialize songs array that respects to the object */
song_t **inp_init_obj_songs( in_plugin_t *p, char *name, int *num_songs )
{
	if (p != NULL && (p->m_fl.m_init_obj_songs != NULL))
		return p->m_fl.m_init_obj_songs(name, num_songs);
	return NULL;
} /* End of 'inp_init_obj_songs' function */

/* Pause playing */
void inp_pause( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_pause != NULL))
		p->m_fl.m_pause();
} /* End of 'inp_pause' function */

/* Resume playing */
void inp_resume( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_resume != NULL))
		p->m_fl.m_resume();
} /* End of 'inp_resume' function */

/* Get current time */
int inp_get_cur_time( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_get_cur_time != NULL))
		return p->m_fl.m_get_cur_time();
	return -1;
} /* End of 'inp_get_cur_time' function */

/* Get number of special functions */
int inp_get_num_specs( in_plugin_t *p )
{
	if (p != NULL)
		return p->m_fl.m_num_spec_funcs;
	return 0;
} /* End of 'inp_get_num_specs' function */

/* Get special function title */
char *inp_get_spec_title( in_plugin_t *p, int index )
{
	if (p != NULL && index >= 0 && index < p->m_fl.m_num_spec_funcs &&
			p->m_fl.m_spec_funcs != NULL)
		return p->m_fl.m_spec_funcs[index].m_title;
	return NULL;
} /* End of 'inp_get_spec_title' function */

/* Get special function flags */
dword inp_get_spec_flags( in_plugin_t *p, int index )
{
	if (p != NULL && index >= 0 && index < p->m_fl.m_num_spec_funcs &&
			p->m_fl.m_spec_funcs != NULL)
		return p->m_fl.m_spec_funcs[index].m_flags;
	return 0;
} /* End of 'inp_get_spec_flags' function */

/* Call special function */
void inp_spec_func( in_plugin_t *p, int index, char *filename )
{
	if (p != NULL && p->m_fl.m_spec_funcs != NULL && index >= 0 && 
			index < p->m_fl.m_num_spec_funcs && 
			p->m_fl.m_spec_funcs[index].m_func != NULL)
		p->m_fl.m_spec_funcs[index].m_func(filename);
} /* End of 'inp_spec_func' function */

/* Set song title */
str_t *inp_set_song_title( in_plugin_t *p, char *filename )
{
	if (filename == NULL)
		return;
	
	if (p != NULL && p->m_fl.m_set_song_title != NULL)
		return p->m_fl.m_set_song_title(filename);
	else
		return str_new(util_short_name(filename));
} /* End of 'inp_set_song_title' function */

/* Set next song name */
void inp_set_next_song( in_plugin_t *p, char *name )
{
	if (p != NULL && p->m_fl.m_set_next_song != NULL)
		p->m_fl.m_set_next_song(name);
} /* End of 'inp_set_next_song' function */

/* Get information about plugin */
char *inp_get_about( in_plugin_t *p )
{
	if (p != NULL && p->m_fl.m_about != NULL)
		return p->m_fl.m_about;
	else
		return NULL;
} /* End of 'inp_get_about' function */

/* End of 'inp.c' file */

