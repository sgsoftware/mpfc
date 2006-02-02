/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. General plugin management functions implementation.
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

#include <stdlib.h>
#include "types.h"
#include "genp.h"
#include "plugin.h"
#include "pmng.h"

/* Initialize general plugin */
plugin_t *genp_init( char *name, struct tag_pmng_t *pmng )
{
	genp_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_GENERAL, sizeof(general_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	GENERAL_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&GENERAL_PLUGIN(p)->m_pd);
	return p;
} /* End of 'genp_init' function */

/* Start plugin function */
bool_t genp_start( general_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_start != NULL))
		return p->m_pd.m_start();
	return FALSE;
} /* End of 'genp_start' function */

/* End plugin function */
void genp_end( general_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_end != NULL))
		p->m_pd.m_end();
} /* End of 'genp_end' function */

/* Check if plugin is started */
bool_t genp_is_started( general_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_is_started != NULL))
		return p->m_pd.m_is_started();
	return FALSE;
} /* End of 'genp_is_started' function */

/* hooks callback */
void genp_hooks_callback( general_plugin_t *p, char *hook )
{
	if (p != NULL && (p->m_pd.m_hooks_callback != NULL))
		p->m_pd.m_hooks_callback(hook);
} /* End of 'genp_hooks_callback' function */

/* End of 'plugin_general.c' file */

