/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for effect plugin management functions.
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

#ifndef __SG_MPFC_EP_H__
#define __SG_MPFC_EP_H__

#include "types.h"
#include "plugin.h"

struct tag_pmng_t;

/* Data for exchange */
typedef struct
{
	/* Common plugin data */
	plugin_data_t m_pd;

	/* Apply plugin function */
	int (*m_apply)( byte *data, int len, int fmt, int freq, int channels );

	/* Reserved */
	byte m_reserved[124];

	/* Reserved data */
	byte m_reserved1[64];
} ep_data_t;

/* Effect plugin type */
typedef struct tag_effect_plugin_t
{
	/* Common plugin data */
	plugin_t m_plugin;

	/* Data for exchange */
	ep_data_t m_pd;
} effect_plugin_t;

/* Helper macros */
#define EFFECT_PLUGIN(p) ((effect_plugin_t *)p)
#define EP_DATA(p) ((ep_data_t *)p)

/* Initialize effect plugin */
plugin_t *ep_init( char *name, struct tag_pmng_t *pmng );

/* Apply plugin to audio data */
int ep_apply( effect_plugin_t *plugin, byte *data, int len, 
	   			int fmt, int freq, int channels );

#endif

/* End of 'ep.h' file */


