/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.h
 * PURPOSE     : SG MPFC. Interface for configuration handling
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.09.2003
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

/* Variable flags */
#define CFG_RUNTIME 0x01

/* Variables data base type */
typedef struct tag_cfg_db_t
{
	/* Variable name */
	char m_name[80];

	/* Variable handler (is called when variable is set) */
	void (*m_handler)( char *name );

	/* Variable flags */
	dword m_flags;

	/* Next list element */
	struct tag_cfg_db_t *m_next;
} cfg_db_t;

/* Variable type */
typedef struct tag_cfg_var_t
{
	/* Variable name */
	char m_name[80];

	/* Variable value */
	char m_val[256];
} cfg_var_t;

/* Variables list type */
typedef struct tag_cfg_var_list_t
{
	/* Variables list */
	cfg_var_t *m_vars;
	int m_num_vars;

	/* Data base */
	cfg_db_t *m_db;
} cfg_list_t;

/* Global variables list */
extern cfg_list_t *cfg_list;

/* Initialize configuration */
void cfg_init( void );

/* Uninitialize configuration */
void cfg_free( void );

/* Initialize data base */
void cfg_init_db( void );

/* Initialize variables with default values */
void cfg_init_default( void );

/* Read configuration file */
void cfg_read_rcfile( cfg_list_t *list, char *name );

/* Parse one line from configuration file */
void cfg_parse_line( cfg_list_t *list, char *str );

/* Free configuration list */
void cfg_free_list( cfg_list_t *list );

/* Add variable */
void cfg_new_var( cfg_list_t *list, char *name, char *val );

/* Search for variable and return its index (or negative on failure) */
int cfg_search_var( cfg_list_t *list, char *name );

/* Set variable value */
void cfg_set_var( cfg_list_t *list, char *name, char *val );

/* Set variable integer value */
void cfg_set_var_int( cfg_list_t *list, char *name, int val );

/* Set variable integer float */
void cfg_set_var_float( cfg_list_t *list, char *name, float val );

/* Get variable value */
char *cfg_get_var( cfg_list_t *list, char *name );

/* Get variable integer value */
int cfg_get_var_int( cfg_list_t *list, char *name );

/* Get variable float value */
float cfg_get_var_float( cfg_list_t *list, char *name );

/* Get variable flags */
byte cfg_get_var_flags( cfg_list_t *list, char *name );

/* Handle variable setting */
void cfg_handle_var( cfg_list_t *list, char *name );

/* Set variable information to the data base */
void cfg_set_to_db( cfg_list_t *list, char *name, 
		void (*handler)( char * ), dword flags );

#endif

/* End of 'cfg.h' file */

