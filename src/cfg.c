/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.c
 * PURPOSE     : SG MPFC. High-level configuration handling 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 25.10.2003
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
#include "player.h"
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
	cfg_list->m_db = NULL;

	/* Initialize data base */
	cfg_init_db();

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
	cfg_set_var_int(cfg_list, "silent-mode", 0);
	cfg_set_var(cfg_list, "output-plugin", "oss");
	cfg_set_var_int(cfg_list, "mp3-quick-get-len", 1);
	cfg_set_var_int(cfg_list, "save-playlist-on-exit", 1);
	cfg_set_var_int(cfg_list, "search-nocase", 0);
	cfg_set_var_int(cfg_list, "play-from-stop", 1);
	cfg_set_var(cfg_list, "lib-dir", LIBDIR"/mpfc");
	cfg_set_var_int(cfg_list, "echo-delay", 500);
	cfg_set_var_int(cfg_list, "echo-volume", 50);
	cfg_set_var_int(cfg_list, "echo-feedback", 50);
} /* End of 'cfg_init_default' function */

/* Read configuration file */
void cfg_read_rcfile( cfg_list_t *list, char *name )
{
	FILE *fd;

	if (list == NULL)
		return;

	/* Try to open file */
	fd = util_fopen(name, "rt");
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
		cfg_set_var(list, name, "1");
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
		cfg_set_var(list, name, val);
	}
} /* End of 'cfg_parse_line' function */

/* Initialize data base */
void cfg_init_db( void )
{
	cfg_set_to_db(cfg_list, "cur-song", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "cur-song_name", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "cur-time", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "player-status", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "player-start", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "player-end", NULL, CFG_RUNTIME);
	cfg_set_to_db(cfg_list, "title-format", 
			player_handle_var_title_format, 0);
	cfg_set_to_db(cfg_list, "output-plugin", player_handle_var_outp, 0);
	cfg_set_to_db(cfg_list, "color-scheme", player_handle_color_scheme, 0);
	cfg_set_to_db(cfg_list, "kbind-scheme", player_handle_kbind_scheme, 0);
} /* End of 'cfg_init_db' function */

/* End of 'cfg.c' file */

