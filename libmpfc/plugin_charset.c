/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : csp.c
 * PURPOSE     : SG MPFC. Charset plugin management functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.02.2004
 * NOTE        : Module prefix 'csp'.
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
#include "csp.h"
#include "pmng.h"
#include "util.h"

/* Initialize charset plugin */
cs_plugin_t *csp_init( char *name, struct tag_pmng_t *pmng )
{
	cs_plugin_t *p;
	void (*fl)( csp_func_list_t * );

	/* Try to allocate memory for plugin object */
	p = (cs_plugin_t *)malloc(sizeof(cs_plugin_t));
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
	fl = dlsym(p->m_lib_handler, "csp_get_func_list");
	if (fl == NULL)
	{
		csp_free(p);
		return NULL;
	}
	p->m_name = pmng_create_plugin_name(name);
	memset(&p->m_fl, 0, sizeof(p->m_fl));
	p->m_fl.m_pmng = pmng;
	fl(&p->m_fl);
	return p;
} /* End of 'csp_init' function */

/* Free charset plugin object */
void csp_free( cs_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		if (p->m_name != NULL)
			free(p->m_name);
		free(p);
	}
} /* End of 'csp_free' function */

/* Get number of supported character sets */
int csp_get_num_sets( cs_plugin_t *p )
{
	if (p != NULL && p->m_fl.m_get_num_sets != NULL)
		return p->m_fl.m_get_num_sets();
	else
		return 0;
} /* End of 'csp_get_num_sets' function */

/* Get charset name */
char *csp_get_cs_name( cs_plugin_t *p, int index )
{
	if (p != NULL && p->m_fl.m_get_cs_name != NULL)
		return p->m_fl.m_get_cs_name(index);
	else
		return NULL;
} /* End of 'csp_get_cs_name' function */

/* Get symbol code */
dword csp_get_code( cs_plugin_t *p, byte ch, int index )
{
	if (p != NULL && p->m_fl.m_get_code != NULL)
		return p->m_fl.m_get_code(ch, index);
	else
		return 0;
} /* End of 'csp_get_code' function */

/* Get information about plugin */
char *csp_get_about( cs_plugin_t *p )
{
	if (p != NULL && p->m_fl.m_about != NULL)
		return p->m_fl.m_about;
	else
		return NULL;
} /* End of 'csp_get_about' function */

/* Expand automatic charset */
char *csp_expand_auto( cs_plugin_t *p, char *sample_str, int index )
{
	if (p != NULL && p->m_fl.m_expand_auto != NULL)
		return p->m_fl.m_expand_auto(sample_str, index);
	else
		return NULL;
} /* End of 'csp_expand_auto' function */

/* Get charset flags */
dword csp_get_flags( cs_plugin_t *p, int index )
{
	if (p != NULL && p->m_fl.m_get_flags != NULL)
		return p->m_fl.m_get_flags(index);
	else
		return 0;
} /* End of 'csp_get_flags' function */

/* End of 'csp.c' file */

