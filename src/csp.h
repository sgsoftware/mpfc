/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : csp.h
 * PURPOSE     : SG MPFC. Interface for charset plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 7.02.2004
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

struct tag_pmng_t;

/* Plugin functions list structure */
typedef struct
{
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

	/* Information about plugin */
	char *m_about;
	
	/* Plugins manager */
	struct tag_pmng_t *m_pmng;

	/* Reserved data */
	byte m_reserved1[120];
} csp_func_list_t;

/* Charset plugin type */
typedef struct tag_cs_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin name */
	char *m_name;

	/* Functions list */
	csp_func_list_t m_fl;
} cs_plugin_t;

/* Charset flags */
#define CSP_AUTO 0x00000001

/* Initialize charset plugin */
cs_plugin_t *csp_init( char *name, struct tag_pmng_t *pmng );

/* Free charset plugin object */
void csp_free( cs_plugin_t *p );

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

/* Get information about plugin */
char *csp_get_about( cs_plugin_t *p );

#endif

/* End of 'csp.h' file */

