/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.c
 * PURPOSE     : SG MPFC. Plugins manager functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.10.2003
 * NOTE        : Module prefix 'pmng'.
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

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "cfg.h"
#include "ep.h"
#include "finder.h"
#include "inp.h"
#include "outp.h"
#include "pmng.h"
#include "util.h"

/* Input plugins list */
int pmng_num_inp = 0;
in_plugin_t **pmng_inp = NULL;

/* Output plugins list */
int pmng_num_outp = 0;
out_plugin_t **pmng_outp = NULL;

/* Effect plugins list */
int pmng_num_ep = 0;
effect_plugin_t **pmng_ep = NULL;

/* Current output plugin */
out_plugin_t *pmng_cur_out = NULL;

/* Initialize plugins */
bool_t pmng_init( void )
{
	pmng_load_plugins();
	return TRUE;
} /* End of 'pmng_init' function */

/* Unitialize plugins */
void pmng_free( void )
{
	int i;
	
	for ( i = 0; i < pmng_num_inp; i ++ )
		inp_free(pmng_inp[i]);
	free(pmng_inp);
	for ( i = 0; i < pmng_num_outp; i ++ )
		outp_free(pmng_outp[i]);
	free(pmng_outp);
	for ( i = 0; i < pmng_num_ep; i ++ )
		ep_free(pmng_ep[i]);
	free(pmng_ep);
	pmng_cur_out = NULL;
} /* End of 'pmng_free' function */

/* Add an input plugin */
void pmng_add_in( in_plugin_t *p )
{
	if (pmng_inp == NULL)
		pmng_inp = (in_plugin_t **)malloc(sizeof(in_plugin_t *));
	else
		pmng_inp = (in_plugin_t **)realloc(pmng_inp, 
				sizeof(in_plugin_t *) * (pmng_num_inp + 1));
	pmng_inp[pmng_num_inp ++] = p;
} /* End of 'pmng_add_in' function */

/* Add an output plugin */
void pmng_add_out( out_plugin_t *p )
{
	if (pmng_outp == NULL)
		pmng_outp = (out_plugin_t **)malloc(sizeof(out_plugin_t *));
	else
		pmng_outp = (out_plugin_t **)realloc(pmng_outp, 
				sizeof(out_plugin_t *) * (pmng_num_outp + 1));
	pmng_outp[pmng_num_outp ++] = p;

	if (pmng_cur_out == NULL)
		pmng_cur_out = p;
} /* End of 'pmng_add_out' function */

/* Search for input plugin supporting given format */
in_plugin_t *pmng_search_format( char *format )
{
	int i;

	for ( i = 0; i < pmng_num_inp; i ++ )
	{
		char formats[128], ext[10];
		int j, k = 0;
	   
		inp_get_formats(pmng_inp[i], formats);
		for ( j = 0;; ext[k ++] = formats[j ++] )
		{
			if (formats[j] == 0 || formats[j] == ';')
			{
				ext[k] = 0;
				if (!strcasecmp(ext, format))
					return pmng_inp[i];
				k = 0;
			}
			if (!formats[j])
				break;
		}
	}
	return NULL;
} /* End of 'pmng_search_format' function */

/* Add an effect plugin */
void pmng_add_effect( effect_plugin_t *p )
{
	if (pmng_ep == NULL)
		pmng_ep = (effect_plugin_t **)malloc(sizeof(effect_plugin_t *));
	else
		pmng_ep = (effect_plugin_t **)realloc(pmng_ep, 
				sizeof(effect_plugin_t *) * (pmng_num_ep + 1));
	pmng_ep[pmng_num_ep ++] = p;
} /* End of 'pmng_add_effect' function */

/* Apply effect plugins */
int pmng_apply_effects( byte *data, int len, int fmt, int freq, int channels )
{
	int l = len, i;

	for ( i = 0; i < pmng_num_ep; i ++ )
	{
		char name[256];
		
		/* Apply effect plugin if it is enabled */
		sprintf(name, "enable_effect_%s", pmng_ep[i]->m_name);
		if (cfg_get_var_int(cfg_list, name))
			l = ep_apply(pmng_ep[i], data, l, fmt, freq, channels);
	}
	return l;
} /* End of 'pmng_apply_effects' function */

/* Search for input plugin with specified name */
in_plugin_t *pmng_search_inp_by_name( char *name )
{
	int i;

	for ( i = 0; i < pmng_num_inp; i ++ )
		if (!strcmp(name, pmng_inp[i]->m_name))
			return pmng_inp[i];
	return NULL;
} /* End of 'pmng_search_inp_by_name' function */

/* Load plugins */
void pmng_load_plugins( void )
{
	char path[256];
	int type;

	/* Load input plugins */
	type = PMNG_IN;
	sprintf(path, "%s/input", cfg_get_var(cfg_list, "lib_dir"));
	find_do(path, "*.so", pmng_find_handler, &type);

	/* Load output plugins */
	type = PMNG_OUT;
	sprintf(path, "%s/output", cfg_get_var(cfg_list, "lib_dir"));
	find_do(path, "*.so", pmng_find_handler, &type);

	/* Load effect plugins */
	type = PMNG_EFFECT;
	sprintf(path, "%s/effect", cfg_get_var(cfg_list, "lib_dir"));
	find_do(path, "*.so", pmng_find_handler, &type);
} /* End of 'pmng_load_plugins' function */

/* Plugin finder handler */
int pmng_find_handler( char *name, void *data )
{
	int type = *(int *)data;

	/* Check if this plugin is not loaded already */
	if (pmng_is_loaded(name, type))
		return 0;

	/* Input plugin */
	if (type == PMNG_IN)
	{
		in_plugin_t *ip;

		ip = inp_init(name);
		if (ip != NULL)
			pmng_add_in(ip);
	}
	/* Output plugin */
	else if (type == PMNG_OUT)
	{
		out_plugin_t *op;
		
		op = outp_init(name);
		if (op != NULL)
		{
			int i, j;
			char str1[256], str2[256];
			
			pmng_add_out(op);

			/* Choose this plugin if it is set in options */
			if (!strcmp(op->m_name, cfg_get_var(cfg_list, "output_plugin")))
				pmng_cur_out = op;
		}
	}
	/* Effect plugin */
	else if (type == PMNG_EFFECT)
	{
		effect_plugin_t *ep;
		
		ep = ep_init(name);
		if (ep != NULL)
			pmng_add_effect(ep);
	}
	return 0;
} /* End of 'pmng_find_handler' function */

/* Check if specified plugin is already loaded */
bool_t pmng_is_loaded( char *name, int type )
{
	char sn[256];
	int i;
	
	/* Get plugin short name */
	util_get_plugin_short_name(sn, name);
	
	/* Search */
	switch (type)
	{
	case PMNG_IN:
		for ( i = 0; i < pmng_num_inp; i ++ )
			if (!strcmp(sn, pmng_inp[i]->m_name))
				return TRUE;
		break;
	case PMNG_OUT:
		for ( i = 0; i < pmng_num_outp; i ++ )
			if (!strcmp(sn, pmng_outp[i]->m_name))
				return TRUE;
		break;
	case PMNG_EFFECT:
		for ( i = 0; i < pmng_num_ep; i ++ )
			if (!strcmp(sn, pmng_ep[i]->m_name))
				return TRUE;
		break;
	}
	return FALSE;
} /* End of 'pmng_is_loaded' function */

/* End of 'pmng.c' file */

