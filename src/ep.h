/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : ep.h
 * PURPOSE     : SG MPFC. Interface for effect plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.02.2004
 * NOTE        : Module prefix 'ep'.
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

#ifndef __SG_MPFC_EP_H__
#define __SG_MPFC_EP_H__

#include "types.h"

struct tag_pmng_t;

/* Plugin functions list structure */
typedef struct
{
	/* Apply plugin function */
	int (*m_apply)( byte *data, int len, int fmt, int freq, int channels );

	/* Reserved */
	byte m_reserved[124];

	/* Information about plugin */
	char *m_about;

	/* Plugins manager */
	struct tag_pmng_t *m_pmng;

	/* Reserved data */
	byte m_reserved1[120];
} ep_func_list_t;

/* Effect plugin type */
typedef struct tag_effect_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin short name */
	char *m_name;

	/* Functions list */
	ep_func_list_t m_fl;
} effect_plugin_t;

/* Initialize effect plugin */
effect_plugin_t *ep_init( char *name, struct tag_pmng_t *pmng );

/* Free effect plugin object */
void ep_free( effect_plugin_t *plugin );

/* Apply plugin to audio data */
int ep_apply( effect_plugin_t *plugin, byte *data, int len, 
	   			int fmt, int freq, int channels );

/* Get information about plugin */
char *ep_get_about( effect_plugin_t *p );

#endif

/* End of 'ep.h' file */


