/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.c
 * PURPOSE     : SG Konsamp. Plugins manager functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
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

/* Current output plugin */
out_plugin_t *pmng_cur_out = NULL;

/* Initialize plugins */
bool pmng_init( void )
{
	in_plugin_t *ip;
	out_plugin_t *op;
	FILE *fd;
	
	/* Initialize input plugins */ 
	fd = popen("ls /usr/local/lib/mpfc/input/*.so", "r");
	if (fd == NULL)
		return FALSE;
	while (!feof(fd))
	{
		char name[256];
		
		fgets(name, 256, fd);
		name[strlen(name) - 1] = 0;
		ip = inp_init(name);
		if (ip != NULL)
			pmng_add_in(ip);
	}
	pclose(fd);

	/* Initialize output plugins */ 
	fd = popen("ls /usr/local/lib/mpfc/output/*.so", "r");
	if (fd == NULL)
		return FALSE;
	while (!feof(fd))
	{
		char name[256];
		
		fgets(name, 256, fd);
		name[strlen(name) - 1] = 0;
		op = outp_init(name);
		if (op != NULL)
			pmng_add_out(op);
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
	   
		pmng_inp[i]->m_fl.m_get_formats(formats);
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

/* End of 'pmng.c' file */

