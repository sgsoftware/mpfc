/******************************************************************
 * Copyright (C) 2003 - 2011 by SG Software.
 *
 * SG MPFC. Interface for playlist plugin management functions.
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

#ifndef __SG_MPFC_PLP_H__
#define __SG_MPFC_PLP_H__

#include "types.h"
#include "plugin.h"

struct tag_pmng_t;
struct tag_song_metadata_t;

/* Callback function which is called for each play list item */
typedef void (*plp_func_t)( void *ctx, char *name, struct tag_song_metadata_t *metadata );

/* Status code */
typedef int plp_status_t;
#define PLP_STATUS_OK 0
#define PLP_STATUS_FAILED 1

/* Data for exchange */
typedef struct
{
	/* Common plugin data */
	plugin_data_t m_pd;

	/* Iterate over playlist contents */
	plp_status_t (*m_for_each_item)( char *pl_name, void *ctx, plp_func_t f );

	/* Get supported file formats */
	void (*m_get_formats)( char *extensions, char *content_type );

	/* Rank used when comparing against other play list types
	 * Specified with the cfg system (plugins.plist.<name>.rank)
	 * Used for smart directory adding */
	int m_rank;
	
	/* Reserved */
	byte m_reserved[116];

	/* Reserved data */
	byte m_reserved1[64];
} plp_data_t;

/* Charset plugin type */
typedef struct tag_plist_plugin_t
{
	/* Common plugin data */
	plugin_t m_plugin;

	/* Data for exchange */
	plp_data_t m_pd;
} plist_plugin_t;

/* Helper macros */
#define PLIST_PLUGIN(p) ((plist_plugin_t *)p)
#define PLIST_DATA(pd) ((plp_data_t *)pd)
#define PLIST_RANK(plp) (plp->m_pd.m_rank)

/* Initialize playlist plugin */
plugin_t *plp_init( char *name, struct tag_pmng_t *pmng );

/* Iterate over playlist contents */
plp_status_t plp_for_each_item( plist_plugin_t *p, char *pl_name, void *ctx, plp_func_t f );

/* Get supported file formats */
void plp_get_formats( plist_plugin_t *p, char *extensions, char *content_type );
	
#endif

/* End of 'plp.h' file */

