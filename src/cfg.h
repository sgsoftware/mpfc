/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.h
 * PURPOSE     : SG MPFC. Interface for configuration handling
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.04.2003
 * NOTE        : Module prefix 'cfg'.
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

#ifndef __SG_MPFC_CFG_H__
#define __SG_MPFC_CFG_H__

#include "types.h"

/* Variable type */
typedef struct tag_cfg_var_t
{
	/* Variable name */
	char m_name[80];

	/* Variable value */
	char m_val[256];
} cfg_var_t;

/* Variables list */
extern cfg_var_t *cfg_vars;
extern int cfg_num_vars;

/* Initialize configuration */
void cfg_init( void );

/* Uninitialize configuration */
void cfg_free( void );

/* Initialize variables with default values */
void cfg_init_default( void );

/* Read configuration file */
void cfg_read_rcfile( char *name );

/* Parse one line from configuration file */
void cfg_parse_line( char *str );

/* Add variable */
void cfg_new_var( char *name, char *val );

/* Search for variable and return its index (or negative on failure) */
int cfg_search_var( char *name );

/* Set variable value */
void cfg_set_var( char *name, char *val );

/* Set variable integer value */
void cfg_set_var_int( char *name, int val );

/* Get variable value */
char *cfg_get_var( char *name );

/* Get variable integer value */
int cfg_get_var_int( char *name );

#endif

/* End of 'cfg.h' file */

