/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : plugin.h
 * PURPOSE     : MPFC Library. Interface for common plugins data.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.10.2004
 * NOTE        : Module prefix 'plugin'.
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

#ifndef __SG_MPFC_PLUGIN_H__
#define __SG_MPFC_PLUGIN_H__

#include "types.h"
#include "cfg.h"
#include "logger.h"
#include "wnd.h"

/* Forward declarations */
struct tag_pmng_t;

/* Plugin types */
typedef enum
{
	PLUGIN_TYPE_INPUT = 1 << 0,
	PLUGIN_TYPE_OUTPUT = 1 << 1,
	PLUGIN_TYPE_EFFECT = 1 << 2,
	PLUGIN_TYPE_CHARSET = 1 << 3,
	PLUGIN_TYPE_ALL = 0xFFFFFF
} plugin_type_t;

/* Common plugin data for exchange with plugin */
typedef struct
{
	/* Plugin manager */
	struct tag_pmng_t *m_pmng;

	/* Plugin configuration list */
	cfg_node_t *m_cfg;

	/* Root configuration list */
	cfg_node_t *m_root_cfg;

	/* Root window */
	wnd_t *m_root_wnd;

	/* Logger object */
	logger_t *m_logger;

	/* Plugin description */
	char *m_desc;

	/* Plugin author string */
	char *m_author;

	/* Launch configuration dialog function */
	void (*m_configure)( wnd_t *parent );

	/* Reserved data */
	byte m_reserved[32];
} plugin_data_t;

/* Common plugin object */
typedef struct tag_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin name */
	char *m_name;

	/* Plugin type */
	plugin_type_t m_type;

	/* Plugin configuration list */
	cfg_node_t *m_cfg;

	/* Destructor function */
	void (*m_destructor)( struct tag_plugin_t *p );

	/* Pointer to plugin exchange data */
	plugin_data_t *m_pd;
} plugin_t;

/* Helper macros */
#define PLUGIN(p) ((plugin_t *)p)
#define PLUGIN_DATA(pd) ((plugin_data_t *)pd)

/* Initialize plugin */
plugin_t *plugin_init( struct tag_pmng_t *pmng, char *name, plugin_type_t type, 
		int size, plugin_data_t *pd );

/* Free plugin */
void plugin_free( plugin_t *p );

/* Get plugin description string */
char *plugin_get_desc( plugin_t *p );

/* Get plugin author string */
char *plugin_get_author( plugin_t *p );

/* Launch configuration dialog */
void plugin_configure( plugin_t *p, wnd_t *parent );

/* Get plugin manager */
struct tag_pmng_t *plugin_get_pmng( plugin_t *p );

/* Get root configuration list */
cfg_node_t *plugin_get_root_cfg( plugin_t *p );

/* Get plugin configuration list */
cfg_node_t *plugin_get_cfg( plugin_t *p );

/* Get root iwndow */
wnd_t *plugin_get_root_wnd( plugin_t *p );

#endif

/* End of 'plugin.h' file */

