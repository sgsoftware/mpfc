/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.h
 * PURPOSE     : SG MPFC. Interface for plugins manager 
 *               functions.
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

#ifndef __SG_MPFC_PMNG_H__
#define __SG_MPFC_PMNG_H__

#include "types.h"
#include "cfg.h"
#include "csp.h"
#include "ep.h"
#include "inp.h"
#include "outp.h"

/* Plugin types */
#define PMNG_IN		 0 
#define PMNG_OUT	 1
#define PMNG_EFFECT	 2
#define PMNG_CHARSET 3

/* Message printer function type */
typedef void (*pmng_print_msg_t)( char *fmt, ... );

/* Plugin manager type */
typedef struct tag_pmng_t
{
	/* Input plugins list */
	int m_num_inp;
	in_plugin_t **m_inp;

	/* Output plugins list */
	int m_num_outp;
	out_plugin_t **m_outp;

	/* Effect plugins list */
	int m_num_ep;
	effect_plugin_t **m_ep;

	/* Charset plugins list */
	int m_num_csp;
	cs_plugin_t **m_csp;

	/* Current output plugin */
	out_plugin_t *m_cur_out;

	/* Configuration variables list */
	cfg_node_t *m_cfg_list;

	/* Message printing function */
	pmng_print_msg_t m_printer;
} pmng_t;

/* Initialize plugins */
pmng_t *pmng_init( cfg_node_t *list, pmng_print_msg_t print_msg );

/* Unitialize plugins */
void pmng_free( pmng_t *pmng );

/* Load plugins */
bool_t pmng_load_plugins( pmng_t *pmng );

/* Add an input plugin */
void pmng_add_in( pmng_t *pmng, in_plugin_t *p );

/* Add an output plugin */
void pmng_add_out( pmng_t *pmng, out_plugin_t *p );

/* Add an effect plugin */
void pmng_add_effect( pmng_t *pmng, effect_plugin_t *p );

/* Add a charset plugin */
void pmng_add_charset( pmng_t *pmng, cs_plugin_t *p );

/* Search for input plugin supporting given format */
in_plugin_t *pmng_search_format( pmng_t *pmng, char *format );

/* Apply effect plugins */
int pmng_apply_effects( pmng_t *pmng, byte *data, int len, int fmt, 
		int freq, int channels );

/* Search for input plugin with specified name */
in_plugin_t *pmng_search_inp_by_name( pmng_t *pmng, char *name );

/* Plugin finder handler */
int pmng_find_handler( char *name, void *data );

/* Check if specified plugin is already loaded */
bool_t pmng_is_loaded( pmng_t *pmng, char *name, int type );

/* Search plugin for content-type */
in_plugin_t *pmng_search_content_type( pmng_t *pmng, char *content );

/* Find charset plugin which supports specified set */
cs_plugin_t *pmng_find_charset( pmng_t *pmng, char *name, int *index );

/* Get configuration variables list */
cfg_node_t *pmng_get_cfg( pmng_t *pmng );

/* Get message printer */
pmng_print_msg_t pmng_get_printer( pmng_t *pmng );

/* Create a plugin name */
char *pmng_create_plugin_name( char *filename );

#endif

/* End of 'pmng.h' file */

