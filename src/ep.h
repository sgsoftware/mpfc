/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : ep.h
 * PURPOSE     : SG Konsamp. Interface for effect plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
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

#ifndef __SG_KONSAMP_EP_H__
#define __SG_KONSAMP_EP_H__

#include "types.h"

/* Plugin functions list structure */
typedef struct
{
	/* Apply plugin function */
	int (*m_apply)( byte *data, int len, int fmt, int freq, int channels );
} ep_func_list_t;

/* Effect plugin type */
typedef struct tag_effect_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin short name */
	char m_name[256];

	/* Functions list */
	ep_func_list_t m_fl;
} effect_plugin_t;

/* Initialize effect plugin */
effect_plugin_t *ep_init( char *name );

/* Free effect plugin object */
void ep_free( effect_plugin_t *plugin );

/* Apply plugin to audio data */
int ep_apply( effect_plugin_t *plugin, byte *data, int len, 
	   			int fmt, int freq, int channels );

#endif

/* End of 'ep.h' file */


