/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Output plugin management functions implementation.
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
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "inp.h"
#include "outp.h"
#include "pmng.h"
#include "song.h"
#include "util.h"

/* Initialize output plugin */
plugin_t *outp_init( char *name, pmng_t *pmng )
{
	outp_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_OUTPUT, sizeof(out_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	OUTPUT_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&OUTPUT_PLUGIN(p)->m_pd);
	return p;
} /* End of 'outp_init' function */

/* Plugin start function */
bool_t outp_start( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_start != NULL))
		return p->m_pd.m_start();
	return FALSE;
} /* End of 'outp_start' function */

/* Plugin end function */
void outp_end( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_end != NULL))
		p->m_pd.m_end();
} /* End of 'outp_end' function */

/* Set channels number function */
void outp_set_channels( out_plugin_t *p, int ch )
{
	if (p != NULL && (p->m_pd.m_set_channels != NULL))
		p->m_pd.m_set_channels(ch);
} /* End of 'outp_set_channels' function */

/* Set frequency function */
void outp_set_freq( out_plugin_t *p, int freq )
{
	if (p != NULL && (p->m_pd.m_set_freq != NULL))
		p->m_pd.m_set_freq(freq);
} /* End of 'outp_set_freq' function */

/* Set format function */
void outp_set_fmt( out_plugin_t *p, dword fmt )
{
	if (p != NULL && (p->m_pd.m_set_fmt != NULL))
		p->m_pd.m_set_fmt(fmt);
} /* End of 'outp_set_fmt' function */
	
/* Get real channels number */
int outp_get_channels( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_get_channels != NULL))
		return p->m_pd.m_get_channels();
	else
		return -1;
} /* End of 'outp_get_channels' function */

/* Get real frequenry */
int outp_get_freq ( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_get_channels != NULL))
		return p->m_pd.m_get_freq();
	else
		return -1;
} /* End of 'outp_get_freq ' function */

/* Get real format */
dword outp_get_fmt( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_get_fmt != NULL))
		return p->m_pd.m_get_fmt();
	else
		return 0xFFFFFFFF;
} /* End of 'outp_get_fmt' function */

/* Play stream function */
void outp_play( out_plugin_t *p, void *buf, int size )
{
	if (p != NULL && (p->m_pd.m_play != NULL))
		p->m_pd.m_play(buf, size);
} /* End of 'outp_play' function */

/* Flush function */
void outp_flush( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_flush != NULL))
		p->m_pd.m_flush();
} /* End of 'outp_flush' function */

/* Pause playing */
void outp_pause( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_pause != NULL))
		p->m_pd.m_pause();
} /* End of 'outp_pause' function */

/* Resume playing */
void outp_resume( out_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_resume != NULL))
		p->m_pd.m_resume();
} /* End of 'outp_resume' function */

/* Set volume */
void outp_set_volume( out_plugin_t *p, int left, int right )
{
	if (p != NULL && (p->m_pd.m_set_volume != NULL))
		p->m_pd.m_set_volume(left, right);
} /* End of 'outp_set_volume' function */

/* Get volume */
void outp_get_volume( out_plugin_t *p, int *left, int *right )
{
	if (p != NULL && (p->m_pd.m_get_volume != NULL))
		p->m_pd.m_get_volume(left, right);
	else
		*left = *right = 0;
} /* End of 'outp_get_volume' function */

/* Set mixer type */
void outp_set_mixer_type( out_plugin_t *p, plugin_mixer_type_t type )
{
	if (p != NULL && (p->m_pd.m_set_mixer_type != NULL))
		p->m_pd.m_set_mixer_type(type);
} /* End of 'out_set_mixer_type' function */

/* Get plugin flags */
dword outp_get_flags( out_plugin_t *p )
{
	if (p != NULL)
		return p->m_pd.m_flags;
	else
		return 0;
} /* End of 'outp_get_flags' function */

/* End of 'outp.c' file */

