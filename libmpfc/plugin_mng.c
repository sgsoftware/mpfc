/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : plugin_mng.c
 * PURPOSE     : SG MPFC. Plugins manager functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2004
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
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "csp.h"
#include "ep.h"
#include "finder.h"
#include "inp.h"
#include "outp.h"
#include "pmng.h"
#include "util.h"

/* Initialize plugins */
pmng_t *pmng_init( cfg_list_t *list, pmng_print_msg_t print_msg )
{
	pmng_t *pmng;

	/* Allocate memory */
	pmng = (pmng_t *)malloc(sizeof(pmng_t));
	if (pmng == NULL)
		return NULL;
	memset(pmng, 0, sizeof(pmng_t));
	pmng->m_cfg_list = list;
	pmng->m_printer = print_msg;
	
	/* Load plugins */
	if (!pmng_load_plugins(pmng))
	{
		pmng_free(pmng);
		return NULL;
	}
	return pmng;
} /* End of 'pmng_init' function */

/* Unitialize plugins */
void pmng_free( pmng_t *pmng )
{
	int i;

	if (pmng == NULL)
		return;
	
	for ( i = 0; i < pmng->m_num_inp; i ++ )
		inp_free(pmng->m_inp[i]);
	free(pmng->m_inp);
	for ( i = 0; i < pmng->m_num_outp; i ++ )
		outp_free(pmng->m_outp[i]);
	free(pmng->m_outp);
	for ( i = 0; i < pmng->m_num_ep; i ++ )
		ep_free(pmng->m_ep[i]);
	free(pmng->m_ep);
	for ( i = 0; i < pmng->m_num_csp; i ++ )
		csp_free(pmng->m_csp[i]);
	free(pmng->m_csp);
	free(pmng);
} /* End of 'pmng_free' function */

/* Add an input plugin */
void pmng_add_in( pmng_t *pmng, in_plugin_t *p )
{
	if (pmng == NULL)
		return;
		
	pmng->m_inp = (in_plugin_t **)realloc(pmng->m_inp, 
			sizeof(in_plugin_t *) * (pmng->m_num_inp + 1));
	pmng->m_inp[pmng->m_num_inp ++] = p;
} /* End of 'pmng_add_in' function */

/* Add an output plugin */
void pmng_add_out( pmng_t *pmng, out_plugin_t *p )
{
	if (pmng == NULL)
		return;

	pmng->m_outp = (out_plugin_t **)realloc(pmng->m_outp, 
			sizeof(out_plugin_t *) * (pmng->m_num_outp + 1));
	pmng->m_outp[pmng->m_num_outp ++] = p;

	if (pmng->m_cur_out == NULL)
		pmng->m_cur_out = p;
} /* End of 'pmng_add_out' function */

/* Add an effect plugin */
void pmng_add_effect( pmng_t *pmng, effect_plugin_t *p )
{
	if (pmng == NULL)
		return;

	pmng->m_ep = (effect_plugin_t **)realloc(pmng->m_ep, 
			sizeof(effect_plugin_t *) * (pmng->m_num_ep + 1));
	pmng->m_ep[pmng->m_num_ep ++] = p;
} /* End of 'pmng_add_effect' function */

/* Add a charset plugin */
void pmng_add_charset( pmng_t *pmng, cs_plugin_t *p )
{
	if (pmng == NULL)
		return;

	pmng->m_csp = (cs_plugin_t **)realloc(pmng->m_csp, 
			sizeof(cs_plugin_t *) * (pmng->m_num_csp + 1));
	pmng->m_csp[pmng->m_num_csp ++] = p;
} /* End of 'pmng_add_charset' function */

/* Search for input plugin supporting given format */
in_plugin_t *pmng_search_format( pmng_t *pmng, char *format )
{
	int i;

	if (pmng == NULL)
		return NULL;

	for ( i = 0; i < pmng->m_num_inp; i ++ )
	{
		char formats[128], ext[10];
		int j, k = 0;
	   
		inp_get_formats(pmng->m_inp[i], formats, NULL);
		for ( j = 0;; ext[k ++] = formats[j ++] )
		{
			if (formats[j] == 0 || formats[j] == ';')
			{
				ext[k] = 0;
				if (!strcasecmp(ext, format))
					return pmng->m_inp[i];
				k = 0;
			}
			if (!formats[j])
				break;
		}
	}
	return NULL;
} /* End of 'pmng_search_format' function */

/* Apply effect plugins */
int pmng_apply_effects( pmng_t *pmng, byte *data, int len, int fmt, 
		int freq, int channels )
{
	int l = len, i;

	if (pmng == NULL)
		return 0;

	for ( i = 0; i < pmng->m_num_ep; i ++ )
	{
		char name[256];
		
		/* Apply effect plugin if it is enabled */
		sprintf(name, "enable-effect-%s", pmng->m_ep[i]->m_name);
		if (cfg_get_var_int(pmng->m_cfg_list, name))
			l = ep_apply(pmng->m_ep[i], data, l, fmt, freq, channels);
	}
	return l;
} /* End of 'pmng_apply_effects' function */

/* Search for input plugin with specified name */
in_plugin_t *pmng_search_inp_by_name( pmng_t *pmng, char *name )
{
	int i;

	if (pmng == NULL)
		return NULL;

	for ( i = 0; i < pmng->m_num_inp; i ++ )
		if (!strcmp(name, pmng->m_inp[i]->m_name))
			return pmng->m_inp[i];
	return NULL;
} /* End of 'pmng_search_inp_by_name' function */

/* Load plugins */
bool_t pmng_load_plugins( pmng_t *pmng )
{
	char path[MAX_FILE_NAME];
	struct 
	{
		int m_type;
		pmng_t *pmng;
	} data = {0, pmng};

	if (pmng == NULL)
		return FALSE;

	/* Load input plugins */
	data.m_type = PMNG_IN;
	sprintf(path, "%s/input", cfg_get_var(pmng->m_cfg_list, "lib-dir"));
	find_do(path, "*.so", pmng_find_handler, &data);

	/* Load output plugins */
	data.m_type = PMNG_OUT;
	sprintf(path, "%s/output", cfg_get_var(pmng->m_cfg_list, "lib-dir"));
	find_do(path, "*.so", pmng_find_handler, &data);

	/* Load effect plugins */
	data.m_type = PMNG_EFFECT;
	sprintf(path, "%s/effect", cfg_get_var(pmng->m_cfg_list, "lib-dir"));
	find_do(path, "*.so", pmng_find_handler, &data);

	/* Load charset plugins */
	data.m_type = PMNG_CHARSET;
	sprintf(path, "%s/charset", cfg_get_var(pmng->m_cfg_list, "lib-dir"));
	find_do(path, "*.so", pmng_find_handler, &data);
	return TRUE;
} /* End of 'pmng_load_plugins' function */

/* Plugin finder handler */
int pmng_find_handler( char *name, void *data )
{
	struct data_t
	{
		int m_type;
		pmng_t *m_pmng;
	} *pmng_data;
	pmng_t *pmng;
	int type;

	/* Get data */
	pmng_data = (struct data_t *)data;
	pmng = pmng_data->m_pmng;
	type = pmng_data->m_type;

	/* Check if this plugin is not loaded already */
	if (pmng_is_loaded(pmng, name, type))
		return 0;

	/* Input plugin */
	if (type == PMNG_IN)
	{
		in_plugin_t *ip;

		ip = inp_init(name, pmng);
		if (ip != NULL)
			pmng_add_in(pmng, ip);
	}
	/* Output plugin */
	else if (type == PMNG_OUT)
	{
		out_plugin_t *op;
		
		op = outp_init(name, pmng);
		if (op != NULL)
		{
			char *out_plugin;
			
			pmng_add_out(pmng, op);

			/* Choose this plugin if it is set in options */
			out_plugin = cfg_get_var(pmng->m_cfg_list, "output-plugin");
			if (out_plugin != NULL && !strcmp(op->m_name, out_plugin))
				pmng->m_cur_out = op;
		}
	}
	/* Effect plugin */
	else if (type == PMNG_EFFECT)
	{
		effect_plugin_t *ep;
		
		ep = ep_init(name, pmng);
		if (ep != NULL)
			pmng_add_effect(pmng, ep);
	}
	/* Charset plugin */
	else if (type == PMNG_CHARSET)
	{
		cs_plugin_t *csp;
		
		csp = csp_init(name, pmng);
		if (csp != NULL)
			pmng_add_charset(pmng, csp);
	}
	return 0;
} /* End of 'pmng_find_handler' function */

/* Check if specified plugin is already loaded */
bool_t pmng_is_loaded( pmng_t *pmng, char *name, int type )
{
	char *sn;
	int i;

	if (pmng == NULL)
		return FALSE;
	
	/* Get plugin short name */
	sn = util_short_name(name);
	
	/* Search */
	switch (type)
	{
	case PMNG_IN:
		for ( i = 0; i < pmng->m_num_inp; i ++ )
			if (!strcmp(sn, pmng->m_inp[i]->m_name))
				return TRUE;
		break;
	case PMNG_OUT:
		for ( i = 0; i < pmng->m_num_outp; i ++ )
			if (!strcmp(sn, pmng->m_outp[i]->m_name))
				return TRUE;
		break;
	case PMNG_EFFECT:
		for ( i = 0; i < pmng->m_num_ep; i ++ )
			if (!strcmp(sn, pmng->m_ep[i]->m_name))
				return TRUE;
		break;
	case PMNG_CHARSET:
		for ( i = 0; i < pmng->m_num_csp; i ++ )
			if (!strcmp(sn, pmng->m_csp[i]->m_name))
				return TRUE;
		break;
	}
	return FALSE;
} /* End of 'pmng_is_loaded' function */

/* Search plugin for content-type */
in_plugin_t *pmng_search_content_type( pmng_t *pmng, char *content )
{
	int i;

	if (content == NULL || pmng == NULL)
		return NULL;

	for ( i = 0; i < pmng->m_num_inp; i ++ )
	{
		char types[256], type[80];
		int j, k = 0;
	   
		inp_get_formats(pmng->m_inp[i], NULL, types);
		if (types == NULL)
			continue;
		for ( j = 0;; type[k ++] = types[j ++] )
		{
			if (types[j] == 0 || types[j] == ';')
			{
				type[k] = 0;
				if (!strcasecmp(type, content))
					return pmng->m_inp[i];
				k = 0;
			}
			if (!types[j])
				break;
		}
	}
	return NULL;
} /* End of 'pmng_search_content_type' function */

/* Find charset plugin which supports specified set */
cs_plugin_t *pmng_find_charset( pmng_t *pmng, char *name, int *index )
{
	int i;
	
	if (pmng == NULL)
		return NULL;

	for ( i = 0; i < pmng->m_num_csp; i ++ )
	{
		int num = csp_get_num_sets(pmng->m_csp[i]), j;

		for ( j = 0; j < num; j ++ )
		{
			char *sn;

			if ((sn = csp_get_cs_name(pmng->m_csp[i], j)) != NULL)
			{
				if (!strcmp(sn, name))
				{
					*index = j;
					return pmng->m_csp[i];
				}
			}
		}
	}
	return NULL;
} /* End of 'pmng_find_charset' function */

/* Get configuration variables list */
cfg_list_t *pmng_get_cfg( pmng_t *pmng )
{
	if (pmng == NULL)
		return NULL;
	else
		return pmng->m_cfg_list;
} /* End of 'pmng_get_cfg' function */

/* Get message printer */
pmng_print_msg_t pmng_get_printer( pmng_t *pmng )
{
	if (pmng != NULL)
		return pmng->m_printer;
	else
		return NULL;
} /* End of 'pmng_get_printer' function */

/* Create a plugin name */
char *pmng_create_plugin_name( char *filename )
{
	char *sn = util_short_name(filename), *name, *ext;

	if (strncmp(sn, "lib", 3))
		return strdup(sn);
	sn += 3;
	name = strdup(sn);
	ext = util_extension(name);
	if (ext != NULL)
	{
		ext --;
		*ext = 0;
	}
	return name;
} /* End of 'pmng_create_plugin_name' function */

/* End of 'pmng.c' file */

