/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Charset plugin management functions implementation.
 * $Id$
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
plugin_t *csp_init( char *name, struct tag_pmng_t *pmng )
{
	csp_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_CHARSET, sizeof(cs_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	CHARSET_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&CHARSET_PLUGIN(p)->m_pd);
	return p;
} /* End of 'csp_init' function */

/* Get number of supported character sets */
int csp_get_num_sets( cs_plugin_t *p )
{
	if (p != NULL && p->m_pd.m_get_num_sets != NULL)
		return p->m_pd.m_get_num_sets();
	else
		return 0;
} /* End of 'csp_get_num_sets' function */

/* Get charset name */
char *csp_get_cs_name( cs_plugin_t *p, int index )
{
	if (p != NULL && p->m_pd.m_get_cs_name != NULL)
		return p->m_pd.m_get_cs_name(index);
	else
		return NULL;
} /* End of 'csp_get_cs_name' function */

/* Get symbol code */
dword csp_get_code( cs_plugin_t *p, byte ch, int index )
{
	if (p != NULL && p->m_pd.m_get_code != NULL)
		return p->m_pd.m_get_code(ch, index);
	else
		return 0;
} /* End of 'csp_get_code' function */

/* Expand automatic charset */
char *csp_expand_auto( cs_plugin_t *p, char *sample_str, int index )
{
	if (p != NULL && p->m_pd.m_expand_auto != NULL)
		return p->m_pd.m_expand_auto(sample_str, index);
	else
		return NULL;
} /* End of 'csp_expand_auto' function */

/* Get charset flags */
dword csp_get_flags( cs_plugin_t *p, int index )
{
	if (p != NULL && p->m_pd.m_get_flags != NULL)
		return p->m_pd.m_get_flags(index);
	else
		return 0;
} /* End of 'csp_get_flags' function */

/* End of 'csp.c' file */

