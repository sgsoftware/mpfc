/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for general plugin management functions.
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

#ifndef __SG_MPFC_GENP_H__
#define __SG_MPFC_GENP_H__

#include <sys/stat.h>
#include "types.h"
#include "plugin.h"

/* Forward declarations */
struct tag_pmng_t;

/* Data for exchange with plugin */
typedef struct
{
	/* Common plugin data */
	plugin_data_t m_common_data;

	/*
	 * Functions
	 */

	/* Start plugin function */
	bool_t (*m_start)( void );

	/* End plugin function */
	void (*m_end)( void );

	/* Check if plugin is started */
	bool_t (*m_is_started)( void );

	/* Reserved data */
	byte m_reserved1[116];
} genp_data_t;

/* General plugin type */
typedef struct tag_general_plugin_t
{
	/* Plugin object */
	plugin_t m_plugin;

	/* Data for exchange */
	genp_data_t m_pd;
} general_plugin_t;

/* Helper macros */
#define GENERAL_PLUGIN(p) ((general_plugin_t *)p)
#define GENP_DATA(pd) ((genp_data_t *)pd)

/* Initialize general plugin */
plugin_t *genp_init( char *name, struct tag_pmng_t *pmng );

/* Start plugin function */
bool_t genp_start( general_plugin_t *p );

/* End plugin function */
void genp_end( general_plugin_t *p );

/* Check if plugin is started */
bool_t genp_is_started( general_plugin_t *p );

#endif

/* End of 'genp.h' file */

