/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : inp.c
 * PURPOSE     : SG Konsamp. Input plugin management functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
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
#include "types.h"
#include "cfg.h"
#include "error.h"
#include "inp.h"
#include "song.h"

/* Initialize input plugin */
in_plugin_t *inp_init( char *name )
{
	in_plugin_t *p;
	void (*fl)( inp_func_list_t * );
	void (*set_vars)( cfg_list_t * );

	/* Try to allocate memory for plugin object */
	p = (in_plugin_t *)malloc(sizeof(in_plugin_t));
	if (p == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_NOW);
	if (p->m_lib_handler == NULL)
	{
		error_set_code(ERROR_IN_PLUGIN_ERROR);
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "inp_get_func_list");
	if (fl == NULL)
	{
		error_set_code(ERROR_IN_PLUGIN_ERROR);
		inp_free(p);
		return NULL;
	}
	util_get_plugin_short_name(p->m_name, name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	fl(&p->m_fl);

	if ((set_vars = dlsym(p->m_lib_handler, "inp_set_vars")) != NULL)
		set_vars(cfg_list);
		
	return p;
} /* End of 'inp_init' function */

/* Free input plugin object */
void inp_free( in_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		free(p);
	}
} /* End of 'inp_free' function */

/* Start playing function */
bool inp_start( in_plugin_t *p, char *filename )
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

/* Get song length function */
int inp_get_len( in_plugin_t *p, char *filename )
{
	if (p != NULL && (p->m_fl.m_get_len != NULL))
		return p->m_fl.m_get_len(filename);
	return 0;
} /* End of 'inp_get_len' function */

/* Get song information function */
bool inp_get_info( in_plugin_t *p, char *file_name, song_info_t *info )
{
	if (p != NULL && (p->m_fl.m_get_info != NULL))
		return p->m_fl.m_get_info(file_name, info);
	return FALSE;
} /* End of 'inp_get_info' function */
	
/* Save song information function */
void inp_save_info( in_plugin_t *p, char *file_name, song_info_t *info )
{
	if (p != NULL && (p->m_fl.m_save_info != NULL))
		p->m_fl.m_save_info(file_name, info);
} /* End of 'inp_save_info' function */
	
/* Get supported file formats */
void inp_get_formats( in_plugin_t *p, char *buf )
{
	if (p != NULL && (p->m_fl.m_get_formats != NULL))
		p->m_fl.m_get_formats(buf);
} /* End of 'inp_get_formats' function */
	
/* Get stream function */
int inp_get_stream( in_plugin_t *p, void *buf, int size )
{
	if (p != NULL && (p->m_fl.m_get_stream != NULL))
		return p->m_fl.m_get_stream(buf, size);
	return 0;
} /* End of 'inp_get_stream' function */

/* Seek song */
void inp_seek( in_plugin_t *p, int shift )
{
	if (p != NULL && (p->m_fl.m_seek != NULL))
		p->m_fl.m_seek(shift);
} /* End of 'inp_seek' function */

/* Get song audio parameters */
void inp_get_audio_params( in_plugin_t *p, int *channels, 
							int *frequency, dword *fmt )
{
	if (p != NULL && (p->m_fl.m_get_audio_params != NULL))
		p->m_fl.m_get_audio_params(channels, frequency, fmt);
} /* End of 'inp_get_audio_params' function */

/* Set equalizer parameters */
void inp_set_eq( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_set_eq != NULL))
		p->m_fl.m_set_eq();
} /* End of 'inp_set_eq' function */

/* Get genre list */
genre_list_t *inp_get_glist( in_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_glist != NULL))
		return p->m_fl.m_glist;
	return NULL;
} /* End of 'inp_get_glist' function */

/* End of 'inp.c' file */

