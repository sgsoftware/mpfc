/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.c
 * PURPOSE     : SG MPFC. Configuration handling functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.08.2003
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
#include "util.h"

/* Check if symbol is a white space */
#define cfg_is_whitespace(c) (c <= ' ')

/* Variables list */
cfg_list_t *cfg_list;

/* Initialize configuration */
void cfg_init( void )
{
	char str[256];
	int i;
	
	/* Initialize variables with initial values */
	cfg_list = (cfg_list_t *)malloc(sizeof(cfg_list_t));
	cfg_list->m_vars = NULL;
	cfg_list->m_num_vars = 0;

	/* Set default variables values */
	cfg_init_default();

	/* Read rc file from home directory and from current directory */
	sprintf(str, "%s/.mpfcrc", getenv("HOME"));
	cfg_read_rcfile(cfg_list, str);
	cfg_read_rcfile(cfg_list, "./.mpfcrc");
} /* End of 'cfg_init' function */

/* Uninitialize configuration */
void cfg_free( void )
{
	/* Free memory */
	cfg_free_list(cfg_list);
	cfg_list = NULL;
} /* End of 'cfg_free' function */

/* Initialize variables with default values */
void cfg_init_default( void )
{
	/* Set variables */
	cfg_set_var_int(cfg_list, "silent_mode", 0, 0);
	cfg_set_var(cfg_list, "output_plugin", "oss", 0);
	cfg_set_var_int(cfg_list, "update_song_len_on_play", 0, 0);
	cfg_set_var_int(cfg_list, "mp3_quick_get_len", 1, 0);
	cfg_set_var_int(cfg_list, "save_playlist_on_exit", 1, 0);
	cfg_set_var(cfg_list, "lib_dir", LIBDIR"/mpfc", 0);
	cfg_set_var_int(cfg_list, "echo_delay", 500, 0);
	cfg_set_var_int(cfg_list, "echo_volume", 50, 0);
	cfg_set_var_int(cfg_list, "echo_feedback", 50, 0);
} /* End of 'cfg_init_default' function */

/* Read configuration file */
void cfg_read_rcfile( cfg_list_t *list, char *name )
{
	FILE *fd;

	if (list == NULL)
		return;

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
		cfg_parse_line(list, str);
	}

	/* Close file */
	fclose(fd);
} /* End of 'cfg_read_rcfile' function */

/* Parse one line from configuration file */
void cfg_parse_line( cfg_list_t *list, char *str )
{
	int i, j, len;
	char name[80], val[256];

	if (list == NULL)
		return;
	
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
		cfg_set_var(list, name, "1", 0);
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
		cfg_set_var(list, name, val, 0);
	}
} /* End of 'cfg_parse_line' function */

/* Add variable */
void cfg_new_var( cfg_list_t *list, char *name, char *val, byte flags )
{
	if (list == NULL)
		return;
	
	if (list->m_vars == NULL)
		list->m_vars = (cfg_var_t *)malloc(sizeof(cfg_var_t));
	else
		list->m_vars = (cfg_var_t *)realloc(list->m_vars, sizeof(cfg_var_t) * 
				(list->m_num_vars + 1));
	strcpy(list->m_vars[list->m_num_vars].m_name, name);
	strcpy(list->m_vars[list->m_num_vars].m_val, val);
	list->m_vars[list->m_num_vars].m_flags = flags;
	list->m_num_vars ++;
} /* End of 'cfg_new_var' function */

/* Search for variable and return its index (or negative on failure) */
int cfg_search_var( cfg_list_t *list, char *name )
{
	int i;

	if (list == NULL)
		return -1;

	for ( i = 0; i < list->m_num_vars; i ++ )
		if (!strcmp(list->m_vars[i].m_name, name))
			return i;
	return -1;
} /* End of 'cfg_search_var' function */

/* Set variable value */
void cfg_set_var( cfg_list_t *list, char *name, char *val, byte flags )
{
	int i;

	if (list == NULL)
		return;
	
	/* Search for this variable */
	i = cfg_search_var(list, name);

	/* Variable exists - modify its value */
	if (i >= 0)
	{
		strcpy(list->m_vars[i].m_val, val);
		list->m_vars[i].m_flags = flags;
	}
	/* Create new variable */
	else
		cfg_new_var(list, name, val, flags);
} /* End of 'cfg_set_var' function */

/* Set variable integer value */
void cfg_set_var_int( cfg_list_t *list, char *name, int val, byte flags )
{
	char str[20];

	if (list == NULL)
		return;

	sprintf(str, "%i", val);
	cfg_set_var(list, name, str, flags);
} /* End of 'cfg_set_var_int' function */

/* Get variable value */
char *cfg_get_var( cfg_list_t *list, char *name )
{
	int i;

	if (list == NULL)
		return "0";

	/* Search for this variable */
	i = cfg_search_var(list, name);
	return (i >= 0) ? list->m_vars[i].m_val : "0";
} /* End of 'cfg_get_var' function */

/* Get variable integer value */
int cfg_get_var_int( cfg_list_t *list, char *name )
{
	return atoi(cfg_get_var(list, name));
} /* End of 'cfg_get_var_int' function */

/* Set variable integer float */
void cfg_set_var_float( cfg_list_t *list, char *name, float val, byte flags )
{
	char str[80];

	if (list == NULL)
		return;

	sprintf(str, "%f", val);
	cfg_set_var(list, name, str, flags);
} /* End of 'cfg_set_var_float' function */

/* Get variable float value */
float cfg_get_var_float( cfg_list_t *list, char *name )
{
	return atof(cfg_get_var(list, name));
} /* End of 'cfg_get_var_float' function */

/* Free configuration list */
void cfg_free_list( cfg_list_t *list )
{
	if (list != NULL)
	{
		if (list->m_vars != NULL)
			free(list->m_vars);
		free(list);
	}
} /* End of 'cfg_free_list' function */

/* Get variable flags */
byte cfg_get_var_flags( cfg_list_t *list, char *name )
{
	int i;
	
	if (list == NULL)
		return 0;

	i = cfg_search_var(list, name);
	if (i < 0)
		return 0;
	return list->m_vars[i].m_flags;
} /* End of 'cfg_get_var_flags' function */

/* End of 'cfg.c' file */

