/******************************************************************
 * Copyright (C) 2003 - 2011 by SG Software.
 *
 * SG MPFC. Playlist plugin management functions.
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
#include "plp.h"
#include "pmng.h"
#include "util.h"

/* Initialize playlist plugin */
plugin_t *plp_init( char *name, pmng_t *pmng )
{
	plp_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_PLIST, sizeof(plist_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	pd.m_rank = cfg_get_var_int(p->m_cfg, "rank");
	PLIST_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&PLIST_PLUGIN(p)->m_pd);
	return p;
} /* End of 'ep_init' function */

/* Iterate over playlist contents */
plp_status_t plp_for_each_item( plist_plugin_t *p, char *pl_name, void *ctx, plp_func_t f )
{
	if (p != NULL && (p->m_pd.m_for_each_item != NULL))
		return p->m_pd.m_for_each_item(pl_name, ctx, f);
	return PLP_STATUS_FAILED;
} /* End of 'plp_for_each_item' function */

/* Get supported file formats */
void plp_get_formats( plist_plugin_t *p, char *extensions, char *content_type )
{
	if (p != NULL && (p->m_pd.m_get_formats != NULL))
		p->m_pd.m_get_formats(extensions, content_type);
	else
	{
		if (extensions != NULL)
			strcpy(extensions, "");
		if (content_type != NULL)
			strcpy(content_type, "");
	}
} /* End of 'plp_get_formats' function */
	
/* End of 'plugin_plist.c' file */

