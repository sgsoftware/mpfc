/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.c
 * PURPOSE     : SG MPFC. Basic configuration handling functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.10.2003
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"

/* Add variable */
void cfg_new_var( cfg_list_t *list, char *name, char *val )
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
void cfg_set_var( cfg_list_t *list, char *name, char *val )
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
	}
	/* Create new variable */
	else
		cfg_new_var(list, name, val);

	/* Handle variable */
	cfg_handle_var(list, name);
} /* End of 'cfg_set_var' function */

/* Set variable integer value */
void cfg_set_var_int( cfg_list_t *list, char *name, int val )
{
	char str[20];

	if (list == NULL)
		return;

	sprintf(str, "%i", val);
	cfg_set_var(list, name, str);
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
void cfg_set_var_float( cfg_list_t *list, char *name, float val )
{
	char str[80];

	if (list == NULL)
		return;

	sprintf(str, "%f", val);
	cfg_set_var(list, name, str);
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
		cfg_db_t *t, *t1;
		
		for ( t = list->m_db; t != NULL; )
		{
			t1 = t->m_next;
			free(t);
			t = t1;
		}
		
		if (list->m_vars != NULL)
			free(list->m_vars);
		free(list);
	}
} /* End of 'cfg_free_list' function */

/* Get variable flags */
byte cfg_get_var_flags( cfg_list_t *list, char *name )
{
	cfg_db_t *t;
	
	if (list == NULL)
		return 0;

	for ( t = list->m_db; t != NULL; t = t->m_next )
		if (!strcmp(name, t->m_name))
			return t->m_flags;
	return 0;
} /* End of 'cfg_get_var_flags' function */

/* Handle variable setting */
void cfg_handle_var( cfg_list_t *list, char *name )
{
	cfg_db_t *t;
	
	/* Search for entry in data base */
	if (list == NULL || list->m_db == NULL)
		return;
	for ( t = list->m_db; t != NULL; t = t->m_next )
		if (!strcmp(name, t->m_name))
			break;

	/* Call handler */
	if (t != NULL && t->m_handler != NULL)
		(t->m_handler)(name);
} /* End of 'cfg_handle_var' function */

/* Set variable information to the data base */
void cfg_set_to_db( cfg_list_t *list, char *name, 
		void (*handler)( char * ), dword flags )
{
	cfg_db_t *t, *p;
	
	if (list == NULL)
		return;

	/* Search for existing entry */
	for ( t = list->m_db, p = NULL; t != NULL; p = t, t = t->m_next )
		if (!strcmp(name, t->m_name))
			break;

	/* Add entry */
	if (t == NULL)
	{
		if (p == NULL)
			t = list->m_db = (cfg_db_t *)malloc(sizeof(cfg_db_t));
		else
			t = p->m_next = (cfg_db_t *)malloc(sizeof(cfg_db_t));
		t->m_next = NULL;
	}

	/* Set data */
	strcpy(t->m_name, name);
	t->m_handler = handler;
	t->m_flags = flags;
} /* End of 'cfg_set_to_db' function */

/* End of 'cfg.c' file */

