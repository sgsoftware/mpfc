/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.h
 * PURPOSE     : SG MPFC. Interface for plugins manager 
 *               functions.
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

#ifndef __SG_MPFC_PMNG_H__
#define __SG_MPFC_PMNG_H__

#include "types.h"
#include "ep.h"
#include "inp.h"
#include "outp.h"

/* Plugin types */
#define PMNG_IN		0
#define PMNG_OUT	1
#define PMNG_EFFECT	2

/* Input plugins list */
extern int pmng_num_inp;
extern in_plugin_t **pmng_inp;

/* Output plugins list */
extern int pmng_num_outp;
extern out_plugin_t **pmng_outp;

/* Effect plugins list */
extern int pmng_num_ep;
extern effect_plugin_t **pmng_ep;

/* Current output plugin */
extern out_plugin_t *pmng_cur_out;

/* Initialize plugins */
bool_t pmng_init( void );

/* Unitialize plugins */
void pmng_free( void );

/* Load plugins */
void pmng_load_plugins( void );

/* Add an input plugin */
void pmng_add_in( in_plugin_t *p );

/* Add an output plugin */
void pmng_add_out( out_plugin_t *p );

/* Add an effect plugin */
void pmng_add_effect( effect_plugin_t *p );

/* Search for input plugin supporting given format */
in_plugin_t *pmng_search_format( char *format );

/* Apply effect plugins */
int pmng_apply_effects( byte *data, int len, int fmt, int freq, int channels );

/* Search for input plugin with specified name */
in_plugin_t *pmng_search_inp_by_name( char *name );

/* Plugin finder handler */
int pmng_find_handler( char *name, void *data );

/* Check if specified plugin is already loaded */
bool_t pmng_is_loaded( char *name, int type );

/* Search plugin for content-type */
in_plugin_t *pmng_search_content_type( char *content );

#endif

/* End of 'pmng.h' file */

