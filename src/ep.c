/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : ep.c
 * PURPOSE     : SG MPFC. Fffect plugin management functions
 * 	             implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
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
#include "types.h"
#include "cfg.h"
#include "ep.h"
#include "error.h"

/* Initialize effect plugin */
effect_plugin_t *ep_init( char *name )
{
	effect_plugin_t *p;
	void (*fl)( ep_func_list_t * );
	void (*set_vars)( cfg_list_t * );

	/* Try to allocate memory for plugin object */
	p = (effect_plugin_t *)malloc(sizeof(effect_plugin_t));
	if (p == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_NOW);
	if (p->m_lib_handler == NULL)
	{
		error_set_code(ERROR_EFFECT_PLUGIN_ERROR);
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "ep_get_func_list");
	if (fl == NULL)
	{
		error_set_code(ERROR_EFFECT_PLUGIN_ERROR);
		inp_free(p);
		return NULL;
	}
	util_get_plugin_short_name(p->m_name, name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	fl(&p->m_fl);

	if ((set_vars = dlsym(p->m_lib_handler, "ep_set_vars")) != NULL)
		set_vars(cfg_list);
		
	return p;
} /* End of 'ep_init' function */

/* Free effect plugin object */
void ep_free( effect_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
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

/* End of 'ep.c' file */

