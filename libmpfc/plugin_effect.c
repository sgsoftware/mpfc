/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : ep.c
 * PURPOSE     : SG MPFC. Fffect plugin management functions
 * 	             implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.02.2004
 * NOTE        : Module prefix 'ep'.
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
#include "ep.h"
#include "pmng.h"
#include "util.h"

/* Initialize effect plugin */
effect_plugin_t *ep_init( char *name, pmng_t *pmng )
{
	effect_plugin_t *p;
	void (*fl)( ep_func_list_t * );

	/* Try to allocate memory for plugin object */
	p = (effect_plugin_t *)malloc(sizeof(effect_plugin_t));
	if (p == NULL)
	{
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_NOW);
	if (p->m_lib_handler == NULL)
	{
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "ep_get_func_list");
	if (fl == NULL)
	{
		ep_free(p);
		return NULL;
	}
	p->m_name = pmng_create_plugin_name(name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	p->m_fl.m_pmng = pmng;
	fl(&p->m_fl);
	return p;
} /* End of 'ep_init' function */

/* Free effect plugin object */
void ep_free( effect_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		if (p->m_name != NULL)
			free(p->m_name);
		free(p);
	}
} /* End of 'ep_free' function */

/* Apply plugin to audio data */
int ep_apply( effect_plugin_t *p, byte *data, int len, 
	   			int fmt, int freq, int channels )
{
	if (p != NULL && (p->m_fl.m_apply != NULL))
		return p->m_fl.m_apply(data, len, fmt, freq, channels);
	return len;
} /* End of 'ep_apply' function */

/* Get information about plugin */
char *ep_get_about( effect_plugin_t *p )
{
	if (p != NULL && (p->m_fl.m_about != NULL))
		return p->m_fl.m_about;
	else
		return NULL;
} /* End of 'ep_get_about' function */

/* End of 'ep.c' file */

