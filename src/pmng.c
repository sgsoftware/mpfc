/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.c
 * PURPOSE     : SG MPFC. Plugins manager functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.07.2003
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
	in_plugin_t *ip;
	out_plugin_t *op;
	effect_plugin_t *ep;
	FILE *fd;
	char str[256];
	
	/* Initialize input plugins */ 
	if (cfg_get_var_int(cfg_list, "plugins_search_subdirs"))
		sprintf(str, "find %s/input/ 2>/dev/null | grep \"\\.so$\"", 
				cfg_get_var(cfg_list, "lib_dir"));
	else
		sprintf(str, "ls %s/input/*.so 2>/dev/null", 
				cfg_get_var(cfg_list, "lib_dir"));
	fd = popen(str, "r");
	if (fd == NULL)
		return FALSE;
	while (!feof(fd))
	{
		char name[256];
		
		fgets(name, 256, fd);
		if (feof(fd))
			break;
		if (name[strlen(name) - 1] == '\n')
			name[strlen(name) - 1] = 0;
		ip = inp_init(name);
		if (ip != NULL)
			pmng_add_in(ip);
	}
	pclose(fd);

	/* Initialize effect plugins */ 
	if (cfg_get_var_int(cfg_list, "plugins_search_subdirs"))
		sprintf(str, "find %s/effect/ 2>/dev/null | grep \"\\.so$\"", 
				cfg_get_var(cfg_list, "lib_dir"));
	else
		sprintf(str, "ls %s/effect/*.so 2>/dev/null", 
				cfg_get_var(cfg_list, "lib_dir"));
	fd = popen(str, "r");
	if (fd == NULL)
		return FALSE;
	while (!feof(fd))
	{
		char name[256];
		
		fgets(name, 256, fd);
		if (feof(fd))
			break;
		if (name[strlen(name) - 1] == '\n')
			name[strlen(name) - 1] = 0;
		ep = ep_init(name);
		if (ep != NULL)
			pmng_add_effect(ep);
	}
	pclose(fd);

	/* Initialize output plugins */ 
	if (cfg_get_var_int(cfg_list, "plugins_search_subdirs"))
		sprintf(str, "find %s/output/ 2>/dev/null | grep \"\\.so$\"", 
				cfg_get_var(cfg_list, "lib_dir"));
	else
		sprintf(str, "ls %s/output/*.so 2>/dev/null", 
				cfg_get_var(cfg_list, "lib_dir"));
	fd = popen(str, "r");
	if (fd == NULL)
		return FALSE;
	while (!feof(fd))
	{
		char name[256];
		
		fgets(name, 256, fd);
		if (feof(fd))
			break;

		if (name[strlen(name) - 1] == '\n')
			name[strlen(name) - 1] = 0;
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
	pclose(fd);

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

/* End of 'pmng.c' file */

