/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : outp.c
 * PURPOSE     : SG Konsamp. Output plugin management functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
 * NOTE        : Module prefix 'outp'.
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
#include "error.h"
#include "inp.h"
#include "outp.h"
#include "song.h"

/* Initialize output plugin */
out_plugin_t *outp_init( char *name )
{
	out_plugin_t *p;
	void (*fl)( outp_func_list_t * );

	/* Try to allocate memory for plugin object */
	p = (out_plugin_t *)malloc(sizeof(out_plugin_t));
	if (p == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_LAZY);
	if (p->m_lib_handler == NULL)
	{
		error_set_code(ERROR_OUT_PLUGIN_ERROR);
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "outp_get_func_list");
	if (fl == NULL)
	{
		error_set_code(ERROR_OUT_PLUGIN_ERROR);
		outp_free(p);
		return NULL;
	}
	util_get_plugin_short_name(p->m_name, name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	fl(&p->m_fl);

	return p;
} /* End of 'outp_init' function */

/* Free output plugin object */
void outp_free( out_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		free(p);
	}
} /* End of 'outp_free' function */

/* Plugin start function */
bool outp_start( out_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_start != NULL))
		return p->m_fl.m_start();
	return FALSE;
} /* End of 'outp_start' function */

/* Plugin end function */
void outp_end( out_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_end != NULL))
		p->m_fl.m_end();
} /* End of 'outp_end' function */

/* Set channels number function */
void outp_set_channels( out_plugin_t *p, int ch )
{
	if (p != NULL && (p->m_fl.m_set_channels != NULL))
		p->m_fl.m_set_channels(ch);
} /* End of 'outp_set_channels' function */

/* Set frequency function */
void outp_set_freq( out_plugin_t *p, int freq )
{
	if (p != NULL && (p->m_fl.m_set_freq != NULL))
		p->m_fl.m_set_freq(freq);
} /* End of 'outp_set_freq' function */

/* Set format function */
void outp_set_fmt( out_plugin_t *p, dword fmt )
{
	if (p != NULL && (p->m_fl.m_set_fmt != NULL))
		p->m_fl.m_set_fmt(fmt);
} /* End of 'outp_set_fmt' function */
	
/* Play stream function */
void outp_play( out_plugin_t *p, void *buf, int size )
{
	if (p != NULL && (p->m_fl.m_play != NULL))
		p->m_fl.m_play(buf, size);
} /* End of 'outp_play' function */

/* Flush function */
void outp_flush( out_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_flush != NULL))
		p->m_fl.m_flush();
} /* End of 'outp_flush' function */

/* Set volume */
void outp_set_volume( out_plugin_t *p, int vol )
{
	if (p != NULL && (p->m_fl.m_set_volume != NULL))
		p->m_fl.m_set_volume(vol);
} /* End of 'outp_set_volume' function */

/* Get volume */
int outp_get_volume( out_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_get_volume != NULL))
		return p->m_fl.m_get_volume();
	return 0;
} /* End of 'outp_get_volume' function */

/* End of 'outp.c' file */

