/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.c
 * PURPOSE     : SG MPFC. Configuration handling functions
 *               implementation.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"

/* Check if symbol is a white space */
#define cfg_is_whitespace(c) (c <= ' ')

/* Variables list */
cfg_var_t *cfg_vars = NULL;
int cfg_num_vars = 0;

/* Initialize configuration */
void cfg_init( void )
{
	char str[256], *home_dir;
	int i;
	
	/* Initialize variables with initial values */
	cfg_vars = NULL;
	cfg_num_vars = 0;

	/* Set default variables values */
	cfg_init_default();

	/* Read rc file from home directory and from current directory */
	sprintf(str, "%s/.mpfcrc", getenv("HOME"));
	cfg_read_rcfile(str);
	cfg_read_rcfile("./.mpfcrc");
} /* End of 'cfg_init' function */

/* Uninitialize configuration */
void cfg_free( void )
{
	/* Free memory */
	if (cfg_vars != NULL)
	{
		free(cfg_vars);
		cfg_vars = NULL;
	}
	cfg_num_vars = 0;
} /* End of 'cfg_free' function */

/* Initialize variables with default values */
void cfg_init_default( void )
{
	/* Set variables */
	cfg_set_var_int("silent_mode", 0);
	//cfg_set_var("lib_dir", "/usr/local/lib/mpfc");
	cfg_set_var("output_plugin", "oss");
} /* End of 'cfg_init_default' function */

/* Read configuration file */
void cfg_read_rcfile( char *name )
{
	FILE *fd;

	/* Try to open file */
	fd = fopen(name, "rt");
	if (fd == NULL)
		return;

	/* Read */
	while (!feof(fd))
	{
		char str[1024];

		/* Read line */
		fgets(str, sizeof(str), fd);

		/* Parse this line */
		cfg_parse_line(str);
	}

	/* Close file */
	fclose(fd);
} /* End of 'cfg_read_rcfile' function */

/* Parse one line from configuration file */
void cfg_parse_line( char *str )
{
	int i, j, len;
	char name[80], val[256];
	
	/* If string begins with '#' - it is comment */
	if (str[0] == '#')
		return;

	/* Skip white space */
	len = strlen(str);
	for ( i = 0; i < len && cfg_is_whitespace(str[i]); i ++ );

	/* Read until next white space */
	for ( j = i; j < len && str[j] != '=' && !cfg_is_whitespace(str[j]); j ++ );
	if (cfg_is_whitespace(str[j]) || str[j] == '=')
		j --;

	/* Extract variable name */
	memcpy(name, &str[i], j - i + 1);
	name[j - i + 1] = 0;

	/* Read '=' sign */
	for ( ; j < len && str[j] != '='; j ++ );

	/* Variable has no value - let it be "1" */
	if (j == len)
	{
		cfg_set_var(name, "1");
	}
	/* Read value */
	else
	{
		/* Get value begin */
		for ( i = j + 1; i < len && cfg_is_whitespace(str[i]); i ++ );

		/* Get value end */
		for ( j = i; j < len && !cfg_is_whitespace(str[j]); j ++ );
		if (cfg_is_whitespace(str[j]))
			j --;

		/* Extract value and set it */
		memcpy(val, &str[i], j - i + 1);
		val[j - i + 1] = 0;
		cfg_set_var(name, val);
	}
} /* End of 'cfg_parse_line' function */

/* Add variable */
void cfg_new_var( char *name, char *val )
{
	if (cfg_vars == NULL)
		cfg_vars = (cfg_var_t *)malloc(sizeof(cfg_var_t));
	else
		cfg_vars = (cfg_var_t *)realloc(cfg_vars, sizeof(cfg_var_t) * 
				(cfg_num_vars + 1));
	strcpy(cfg_vars[cfg_num_vars].m_name, name);
	strcpy(cfg_vars[cfg_num_vars].m_val, val);
	cfg_num_vars ++;
} /* End of 'cfg_new_var' function */

/* Search for variable and return its index (or negative on failure) */
int cfg_search_var( char *name )
{
	int i;

	for ( i = 0; i < cfg_num_vars; i ++ )
		if (!strcmp(cfg_vars[i].m_name, name))
			return i;
	return -1;
} /* End of 'cfg_search_var' function */

/* Set variable value */
void cfg_set_var( char *name, char *val )
{
	int i;
	
	/* Search for this variable */
	i = cfg_search_var(name);

	/* Variable exists - modify its value */
	if (i >= 0)
		strcpy(cfg_vars[i].m_val, val);
	/* Create new variable */
	else
		cfg_new_var(name, val);
} /* End of 'cfg_set_var' function */

/* Set variable integer value */
void cfg_set_var_int( char *name, int val )
{
	char str[20];

	sprintf(str, "%i", val);
	cfg_set_var(name, str);
} /* End of 'cfg_set_var_int' function */

/* Get variable value */
char *cfg_get_var( char *name )
{
	int i;

	/* Search for this variable */
	i = cfg_search_var(name);
	return (i >= 0) ? cfg_vars[i].m_val : "0";
} /* End of 'cfg_get_var' function */

/* Get variable integer value */
int cfg_get_var_int( char *name )
{
	return atoi(cfg_get_var(name));
} /* End of 'cfg_get_var_int' function */

/* End of 'cfg.c' file */

