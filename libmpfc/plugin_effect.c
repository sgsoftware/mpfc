/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : ep.c
 * PURPOSE     : SG MPFC. Fffect plugin management functions
 * 	             implementation.
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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "ep.h"
#include "pmng.h"
#include "util.h"

/* Initialize effect plugin */
plugin_t *ep_init( char *name, pmng_t *pmng )
{
	ep_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_EFFECT, sizeof(effect_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	EFFECT_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&EFFECT_PLUGIN(p)->m_pd);
	return p;
} /* End of 'ep_init' function */

/* Apply plugin to audio data */
int ep_apply( effect_plugin_t *p, byte *data, int len, 
	   			int fmt, int freq, int channels )
{
	if (p != NULL && (p->m_pd.m_apply != NULL))
		return p->m_pd.m_apply(data, len, fmt, freq, channels);
	return len;
} /* End of 'ep_apply' function */

/* End of 'ep.c' file */

