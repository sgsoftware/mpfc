/******************************************************************
 * Copyright (C) 2003 - 2013 by SG Software.
 *
 * MPFC Library. Common plugins functions implementation.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "plugin.h"
#include "pmng.h"
#include "util.h"

/* Initialize plugin */
plugin_t *plugin_init( pmng_t *pmng, char *name, plugin_type_t type, int size, 
		plugin_data_t *pd )
{
	plugin_t *p;
	void (*plugin_exchange_data)( plugin_data_t *pd );
	void (*plugin_set_cfg_default)( cfg_node_t *list );
	char *prefix, *cfg_list_name;

	/* Allocate memory for plugin */
	p = (plugin_t *)malloc(size);
	if (p == NULL)
		return NULL;
	memset(p, 0, size);

	/* Set plugin fields */
	p->m_type = type;
	p->m_name = pmng_create_plugin_name(name);

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_NOW);
	if (p->m_lib_handler == NULL)
	{
		logger_error(pmng->m_log, 1, "%s", dlerror());
		plugin_free(p);
		return NULL;
	}

	/* Create plugin configuration list */
	if (type == PLUGIN_TYPE_PLIST)
		prefix = "plugins.plist";
	else if (type == PLUGIN_TYPE_GENERAL)
		prefix = "plugins.general";
	else
	{
		plugin_free(p);
		return NULL;
	}
	plugin_set_cfg_default = dlsym(p->m_lib_handler, "plugin_set_cfg_default");
	cfg_list_name = util_strcat(prefix, ".", p->m_name, NULL);
	pd->m_cfg = cfg_new_list(pmng->m_cfg_list, cfg_list_name, 
			plugin_set_cfg_default, CFG_NODE_SMALL_LIST, 0);
	p->m_cfg = pd->m_cfg;
	free(cfg_list_name);

	/* Exchange data */
	plugin_exchange_data = dlsym(p->m_lib_handler, "plugin_exchange_data");
	if (plugin_exchange_data == NULL)
	{
		logger_error(pmng->m_log, 1,
				_("Plugin %s has no 'plugin_exchange_data'"), p->m_name);
		plugin_free(p);
		return NULL;
	}
	pd->m_pmng = pmng;
	pd->m_root_cfg = pmng->m_cfg_list;
	pd->m_root_wnd = pmng->m_root_wnd;
	pd->m_logger = pmng->m_log;
	plugin_exchange_data(pd);
	return p;
} /* End of 'plugin_init' function */

/* Free plugin */
void plugin_free( plugin_t *p )
{
	assert(p);

	/* Call destructor first */
	if (p->m_destructor != NULL)
		p->m_destructor(p);

	/* Free memory */
	if (p->m_lib_handler != NULL)
		dlclose(p->m_lib_handler);
	if (p->m_name != NULL)
		free(p->m_name);
	free(p);
} /* End of 'plugin_free' function */

/* Get plugin description string */
char *plugin_get_desc( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_desc;
	else
		return NULL;
} /* End of 'plugin_get_desc' function */

/* Get plugin author string */
char *plugin_get_author( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_author;
	else
		return NULL;
} /* End of 'plugin_get_author' function */

/* Get plugin manager */
struct tag_pmng_t *plugin_get_pmng( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_pmng;
	else
		return NULL;
} /* End of 'plugin_get_pmng' function */

/* Get root configuration list */
cfg_node_t *plugin_get_root_cfg( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_root_cfg;
	else
		return NULL;
} /* End of 'plugin_get_root_cfg' function */

/* Get plugin configuration list */
cfg_node_t *plugin_get_cfg( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_cfg;
	else
		return NULL;
} /* End of 'plugin_get_cfg' function */

/* Get root iwndow */
wnd_t *plugin_get_root_wnd( plugin_t *p )
{
	if (p != NULL)
		return p->m_pd->m_root_wnd;
	else
		return NULL;
} /* End of 'plugin_get_root_wnd' function */

/* Launch configuration dialog */
void plugin_configure( plugin_t *p, wnd_t *parent )
{
	if (p != NULL && p->m_pd->m_configure != NULL)
		(p->m_pd->m_configure)(parent);
} /* End of 'plugin_configure' function */

/* End of 'plugin.c' file */

