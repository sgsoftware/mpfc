/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : csp.h
 * PURPOSE     : SG MPFC. Interface for charset plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.10.2004
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

#ifndef __SG_MPFC_CSP_H__
#define __SG_MPFC_CSP_H__

#include "types.h"
#include "plugin.h"

struct tag_pmng_t;

/* Data for exchange */
typedef struct
{
	/* Common plugin data */
	plugin_data_t m_pd;

	/* Get number of supported character sets */
	int (*m_get_num_sets)( void );

	/* Get charset name */
	char *(*m_get_cs_name)( int index );

	/* Get symbol code */
	dword (*m_get_code)( byte ch, int index );

	/* Expand automatic charset */
	char *(*m_expand_auto)( char *sample_str, int index );

	/* Get charset flags */
	dword (*m_get_flags)( int index );

	/* Reserved data */
	byte m_reserved[116];

	/* Reserved data */
	byte m_reserved1[64];
} csp_data_t;

/* Charset plugin type */
typedef struct tag_cs_plugin_t
{
	/* Common plugin data */
	plugin_t m_plugin;

	/* Data for exchange */
	csp_data_t m_pd;
} cs_plugin_t;

/* Helper macros */
#define CHARSET_PLUGIN(p) ((cs_plugin_t *)p)
#define CSP_DATA(pd) ((csp_data_t *)pd)

/* Charset flags */
#define CSP_AUTO 0x00000001

/* Initialize charset plugin */
plugin_t *csp_init( char *name, struct tag_pmng_t *pmng );

/* Get number of supported character sets */
int csp_get_num_sets( cs_plugin_t *p );

/* Get charset name */
char *csp_get_cs_name( cs_plugin_t *p, int index );

/* Get symbol code */
dword csp_get_code( cs_plugin_t *p, byte ch, int index );

/* Expand automatic charset */
char *csp_expand_auto( cs_plugin_t *p, char *sample_str, int index );

/* Get charset flags */
dword csp_get_flags( cs_plugin_t *p, int index );

#endif

/* End of 'csp.h' file */

