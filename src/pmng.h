/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : pmng.h
 * PURPOSE     : SG Konsamp. Interface for plugins manager 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 15.02.2003
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

#ifndef __SG_KONSAMP_PMNG_H__
#define __SG_KONSAMP_PMNG_H__

#include "types.h"
#include "inp.h"
#include "outp.h"

/* Input plugins list */
extern int pmng_num_inp;
extern in_plugin_t **pmng_inp;

/* Output plugins list */
extern int pmng_num_outp;
extern out_plugin_t **pmng_outp;

/* Current output plugin */
extern out_plugin_t *pmng_cur_out;

/* Initialize plugins */
bool pmng_init( void );

/* Unitialize plugins */
void pmng_free( void );

/* Add an input plugin */
void pmng_add_in( in_plugin_t *p );

/* Add an output plugin */
void pmng_add_out( out_plugin_t *p );

/* Search for input plugin supporting given format */
in_plugin_t *pmng_search_format( char *format );

#endif

/* End of 'pmng.h' file */

