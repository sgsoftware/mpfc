/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.c
 * PURPOSE     : SG MPFC. Configuration handling functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.09.2004
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"

/* Create a new configuration list */
cfg_node_t *cfg_new_list( cfg_node_t *parent, char *name, 
		cfg_set_default_values_t set_def, dword flags, int hash_size )
{
	cfg_node_t *node;

	/* Create the node */
	node = cfg_new_node(parent, name, flags);
	if (node == NULL)
		return NULL;
	
	/* Make children hash */
	if (hash_size == 0)
	{
		if (flags & CFG_NODE_SMALL_LIST)
			hash_size = CFG_HASH_SMALL_SIZE;
		else if (flags & CFG_NODE_MEDIUM_LIST)
			hash_size = CFG_HASH_MEDIUM_SIZE;
		else if (flags & CFG_NODE_BIG_LIST)
			hash_size = CFG_HASH_BIG_SIZE;
		else
			hash_size = CFG_HASH_DEFAULT_SIZE;
	}
	CFG_LIST(node)->m_hash_size = hash_size;
	CFG_LIST(node)->m_children = (struct cfg_list_hash_item_t **)
		malloc(hash_size * sizeof(struct cfg_list_hash_item_t *));
	memset(CFG_LIST(node)->m_children, 0, 
			hash_size * sizeof(struct cfg_list_hash_item_t *));

	/* Fill list with default values */
	if (set_def != NULL)
		set_def(node);

	/* Insert node into the parent */
	if (node->m_parent != NULL)
		cfg_insert_node(node->m_parent, node);
	return node;
} /* End of 'cfg_new_list' function */

/* Create a new variable */
cfg_node_t *cfg_new_var( cfg_node_t *parent, char *name, dword flags, 
		char *value, cfg_var_handler_t handler )
{
	cfg_node_t *node;

	/* Create node */
	flags |= CFG_NODE_VAR;
	node = cfg_new_node(parent, name, flags);
	if (node == NULL)
		return NULL;

	/* Set variable data */
	CFG_VAR(node)->m_value = (value == NULL ? NULL : strdup(value));
	CFG_VAR(node)->m_handler = handler;

	/* Call handler */
	if (!cfg_call_var_handler(TRUE, node, value))
	{
		cfg_free_node(node, TRUE);
		return NULL;
	}
	cfg_insert_node(node->m_parent, node);
	return node;
} /* End of 'cfg_new_var' function */

/* Create a new node and leave node type specific information unset 
 * (don't use this function directly; use previous two instead) */
cfg_node_t *cfg_new_node( cfg_node_t *parent, char *name, dword flags )
{
	cfg_node_t *node;
	cfg_node_t *real_parent;
	char *real_name;

	assert(name);

	/* Find real parent */
	if (parent != NULL)
	{
		real_parent = cfg_find_real_parent(parent, name, &real_name);
		if (real_parent == NULL)
			return NULL;
	}
	else
	{
		real_parent = parent;
		real_name = name;
	}

	/* Allocate memory */
	node = (cfg_node_t *)malloc(sizeof(cfg_node_t));
	if (node == NULL)
		return node;

	/* Set fields */
	node->m_name = strdup(real_name);
	node->m_flags = flags;
	node->m_parent = real_parent;
	return node;
} /* End of 'cfg_new_node' function */

/* Free node */
void cfg_free_node( cfg_node_t *node, bool_t recursively )
{
	assert(node);

	/* Free common data */
	free(node->m_name);

	/* Free variable data */
	if (CFG_NODE_IS_VAR(node))
	{
		if (CFG_VAR(node)->m_value != NULL)
			free(CFG_VAR(node)->m_value);
	}
	/* Free list data */
	else
	{
		int i;

		for ( i = 0; i < CFG_LIST(node)->m_hash_size; i ++ )
		{
			struct cfg_list_hash_item_t *item, *next;
			for ( item = CFG_LIST(node)->m_children[i]; item != NULL; )
			{
				next = item->m_next;
				if (recursively)
					cfg_free_node(item->m_node, recursively);
				item = next;
			}
		}
		free(CFG_LIST(node)->m_children);
	}

	/* Free node itself */
	free(node);
} /* End of 'cfg_free_node' function */

/* Insert a node into the list */
void cfg_insert_node( cfg_node_t *list, cfg_node_t *node )
{
	int hash;
	struct cfg_list_hash_item_t *item;

	assert(list);
	assert(node);
	assert(node->m_name);
	assert(CFG_NODE_IS_LIST(list));
	
	/* Calculate hash for this node */
	hash = cfg_calc_hash(node->m_name, CFG_LIST(list)->m_hash_size);

	/* Check whether this node is also created */
	for ( item = CFG_LIST(list)->m_children[hash]; item != NULL; 
			item = item->m_next )
	{
		/* Replace node */
		if (!strcmp(item->m_node->m_name, node->m_name))
		{
			struct cfg_list_data_t *l = CFG_LIST(item->m_node);
			struct cfg_list_hash_item_t *i;
			int h;

			/* Copy nodes */
			for ( h = 0; h < l->m_hash_size; h ++ )
			{
				for ( i = l->m_children[h]; i != NULL; i = i->m_next )
					cfg_insert_node(node, i->m_node);
			}
			cfg_free_node(item->m_node, FALSE);
			item->m_node = node;
			return;
		}
	}

	/* Create item and append to the list */
	item = (struct cfg_list_hash_item_t *)malloc(
			sizeof(struct cfg_list_hash_item_t));
	if (item == NULL)
		return;
	item->m_node = node;
	item->m_next = CFG_LIST(list)->m_children[hash];
	CFG_LIST(list)->m_children[hash] = item;
} /* End of 'cfg_insert_node' function */

/* Search for the node */
cfg_node_t *cfg_search_node( cfg_node_t *parent, char *name )
{
	cfg_node_t *real_parent;
	cfg_node_t *node;
	char *real_name;

	assert(parent);
	assert(name);

	/* Find the real node parent and real name */
	real_parent = cfg_find_real_parent(parent, name, &real_name);
	if (real_parent == NULL)
		return NULL;

	/* Search for node */
	node = cfg_search_list(real_parent, real_name);
	return node;
} /* End of 'cfg_search_node' function */

/* Set variable value */
void cfg_set_var( cfg_node_t *parent, char *name, char *value )
{
	cfg_node_t *node;
	
	/* Search for this node */
	node = cfg_search_node(parent, name);

	/* Change value */
	if (node != NULL && CFG_NODE_IS_VAR(node))
	{
		if (!cfg_call_var_handler(FALSE, node, value))
			return;
		free(CFG_VAR(node)->m_value);
		CFG_VAR(node)->m_value = (value == NULL ? NULL : strdup(value));
		cfg_call_var_handler(TRUE, node, value);
	}
	/* Create node if not found */
	else if (node == NULL)
		cfg_new_var(parent, name, 0, value, NULL);
} /* End of 'cfg_set_var' function */

/* Set variable integer value */
void cfg_set_var_int( cfg_node_t *parent, char *name, int val )
{
	char str[32];
	snprintf(str, sizeof(str), "%d", val);
	cfg_set_var(parent, name, str);
} /* End of 'cfg_set_var_int' function */

/* Set variable integer float */
void cfg_set_var_float( cfg_node_t *parent, char *name, float val )
{
	char str[32];
	snprintf(str, sizeof(str), "%f", val);
	cfg_set_var(parent, name, str);
} /* End of 'cfg_set_var_float' function */

/* Get variable value */
char *cfg_get_var( cfg_node_t *parent, char *name )
{
	cfg_node_t *node;

	/* Get node */
	node = cfg_search_node(parent, name);
	if (node == NULL || !CFG_NODE_IS_VAR(node))
		return NULL;
	return CFG_VAR(node)->m_value;
} /* End of 'cfg_get_var' function */

/* Get variable integer value */
int cfg_get_var_int( cfg_node_t *parent, char *name )
{
	char *str = cfg_get_var(parent, name);
	return (str == NULL) ? 0 : atoi(str);
} /* End of 'cfg_get_var_int' function */

/* Get variable float value */
float cfg_get_var_float( cfg_node_t *parent, char *name )
{
	char *str = cfg_get_var(parent, name);
	return (str == NULL) ? 0. : atof(str);
} /* End of 'cfg_get_var_float' function */

/* Find the real parent of node */
cfg_node_t *cfg_find_real_parent( cfg_node_t *parent, char *name, 
		char **real_name )
{
	cfg_node_t *real_parent;

	assert(parent);
	assert(name);
	assert(CFG_NODE_IS_LIST(parent));

	/* Walk through parents */
	for ( real_parent = parent;; )
	{
		char *next_name = strchr(name, '.');
		cfg_node_t *was_real = real_parent;

		/* If no '.' found - stop (we've found last parent in hierarchy) */
		if (next_name == NULL)
			break;

		/* Extract child list name */
		*next_name = 0;
		real_parent = cfg_search_list(real_parent, name);

		/* Create temporary list if it does not exist */
		if (real_parent == NULL)
			real_parent = cfg_new_list(was_real, name, NULL, 0, 0);

		/* Move to next child name */
		*next_name = '.';
		name = next_name + 1;
	}

	/* Set real name */
	if (real_name != NULL)
		(*real_name) = name;
	return real_parent;
} /* End of 'cfg_find_real_parent' function */

/* Search list for a child node (name given is the exact name, without dots) */
cfg_node_t *cfg_search_list( cfg_node_t *list, char *name )
{
	struct cfg_list_hash_item_t *item;
	int hash;
	cfg_node_t *node = NULL;

	/* Check that this is a list */
	if (!CFG_NODE_IS_LIST(list))
		return NULL;

	/* Calculate hash */
	hash = cfg_calc_hash(name, CFG_LIST(list)->m_hash_size);

	/* Search for node */
	for ( item = CFG_LIST(list)->m_children[hash]; item != NULL; 
			item = item->m_next )
	{
		if (!strcmp(item->m_node->m_name, name))
		{
			node = item->m_node;
			break;
		}
	}
	return node;
} /* End of 'cfg_search_list' function */

/* Call variable handler */
bool_t cfg_call_var_handler( bool_t after, cfg_node_t *node, char *value )
{
	assert(node);
	assert(CFG_NODE_IS_VAR(node));

	if (value == NULL)
		return TRUE;

	if (after && (node->m_flags & CFG_NODE_HANDLE_AFTER_CHANGE) ||
			CFG_VAR_HANDLER(node) == NULL)
		return TRUE;
	return CFG_VAR_HANDLER(node)(node, value);
} /* End of 'cfg_call_var_handler' function */

/* Calculate string hash value */
int cfg_calc_hash( char *str, int table_size )
{
	int val = 0;

	while (*str)
	{
		val += *str;
		str ++;
	}
	return val % table_size;
} /* End of 'cfg_calc_hash' function */

/* End of 'cfg.c' file */

