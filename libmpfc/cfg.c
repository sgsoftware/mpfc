/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Configuration handling functions implementation.
 * $Id$
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
#include <inttypes.h>
#define __USE_GNU
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "util.h"

/* Create a new configuration list */
cfg_node_t *cfg_new_list( cfg_node_t *parent, const char *name, 
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
cfg_node_t *cfg_new_var_full( cfg_node_t *parent, const char *name, 
		dword flags, char *value, cfg_var_handler_t handler, 
		void *handler_data )
{
	cfg_node_t *node;

	/* Create node */
	flags |= CFG_NODE_VAR;
	node = cfg_new_node(parent, name, flags);
	if (node == NULL)
		return NULL;

	/* Set variable data */
	CFG_VAR(node)->m_value = value;
	CFG_VAR(node)->m_handler = handler;
	CFG_VAR(node)->m_handler_data = handler_data;

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
cfg_node_t *cfg_new_node( cfg_node_t *parent, const char *name, dword flags )
{
	cfg_node_t *node;
	cfg_node_t *real_parent;
	const char *real_name;

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
	memset(node, 0, sizeof(*node));

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
		cfg_var_free_operations(node);
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
				free(item);
				item = next;
			}
		}
		free(CFG_LIST(node)->m_children);
	}

	/* Free node itself */
	free(node);
} /* End of 'cfg_free_node' function */

/* Free variable operations list */
void cfg_var_free_operations( cfg_node_t *node )
{
	struct cfg_var_op_list_t *opl, *next;
	assert(CFG_NODE_IS_VAR(node));

	for ( opl = CFG_VAR(node)->m_operations; opl != NULL; )
	{
		next = opl->m_next;
		free(opl->m_arg);
		free(opl);
		opl = next;
	}
	CFG_VAR(node)->m_operations = NULL;
} /* End of 'cfg_var_free_operations' function */

/* Insert a node into the list */
void cfg_insert_node( cfg_node_t *list, cfg_node_t *node )
{
	int hash;
	struct cfg_list_hash_item_t *item;

	assert(list);
	assert(node);
	assert(node->m_name);
	assert(CFG_NODE_IS_LIST(list));
	
	/* Search for this node in the list */
	hash = cfg_calc_hash(node->m_name, CFG_LIST(list)->m_hash_size);
	for ( item = CFG_LIST(list)->m_children[hash]; item != NULL; 
			item = item->m_next )
	{
		/* Copy existing node to the new node and free it */
		if (!strcmp(node->m_name, item->m_node->m_name))
		{
			cfg_copy_node(node, item->m_node);
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
cfg_node_t *cfg_search_node( cfg_node_t *parent, const char *name )
{
	cfg_node_t *real_parent;
	cfg_node_t *node;
	const char *real_name;

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

/* Apply operation to variable value */
char *cfg_var_apply_op( cfg_node_t *node, const char *value, cfg_var_op_t op )
{
	/* Simply set variable value */
	if (op == CFG_VAR_OP_SET)
		return strdup(value);
	/* Append new value */
	else if (op == CFG_VAR_OP_ADD)
	{
		char *existing_value = (node == NULL ? NULL : CFG_VAR(node)->m_value);
		if (existing_value != NULL)
			return util_strcat(existing_value, ";", value, NULL);
		else
			return strdup(value);
	}
	/* Remove some value */
	else if (op == CFG_VAR_OP_REM)
	{
		char *existing_value = (node == NULL ? NULL : CFG_VAR(node)->m_value);
		if (existing_value != NULL)
		{
			char *ret = strdup(existing_value);
			int len = strlen(value);
			char *sub = strstr(ret, value);
			if (sub != NULL)
			{
				if (sub[len] == ';')
					len ++;
				memmove(sub, &sub[len], strlen(sub) - len + 1);
			}
			return ret;
		}
		else
			return strdup("");
	}
	return NULL;
} /* End of 'cfg_var_apply_op' function */

/* Full version of variable value setting */
void cfg_set_var_full( cfg_node_t *parent, const char *name, const char *value, 
		cfg_var_op_t op )
{
	cfg_node_t *node;
	const char *orig_value;
	struct cfg_var_op_list_t *opl, *last;
	
	/* Search for this node */
	node = cfg_search_node(parent, name);

	/* Construct new value */
	orig_value = value;
	char *new_value = cfg_var_apply_op(node, value, op);

	/* Change value */
	if (node != NULL && CFG_NODE_IS_VAR(node))
	{
		if (!cfg_call_var_handler(FALSE, node, new_value))
			return;
		free(CFG_VAR(node)->m_value);
		if (value == NULL)
			CFG_VAR(node)->m_value = NULL;
		else
			CFG_VAR(node)->m_value = new_value;
		cfg_call_var_handler(TRUE, node, new_value);
	}
	/* Create node if not found */
	else if (node == NULL)
		node = cfg_new_var(parent, name, 0, new_value, NULL);

	/* Create operations list item */
	opl = (struct cfg_var_op_list_t *)malloc(sizeof(*opl));
	opl->m_arg = strdup(orig_value);
	opl->m_op = op;
	opl->m_next = NULL;
	if (opl == NULL)
		return;

	/* Insert list item */
	if (op == CFG_VAR_OP_ADD || op == CFG_VAR_OP_REM)
	{
		if (CFG_VAR(node)->m_operations == NULL)
			CFG_VAR(node)->m_operations = opl;
		else
		{
			for ( last = CFG_VAR(node)->m_operations; last->m_next != NULL;
					last = last->m_next );
			last->m_next = opl;
		}
	}
	else
	{
		cfg_var_free_operations(node);
		CFG_VAR(node)->m_operations = opl;
	}
} /* End of 'cfg_set_var_full' function */

/* Set variable value */
void cfg_set_var( cfg_node_t *parent, const char *name, const char *value )
{
	cfg_set_var_full(parent, name, value, CFG_VAR_OP_SET);
} /* End of 'cfg_set_var' function */

/* Set variable integer value */
void cfg_set_var_int( cfg_node_t *parent, const char *name, int val )
{
	char str[32];
	snprintf(str, sizeof(str), "%d", val);
	cfg_set_var(parent, name, str);
} /* End of 'cfg_set_var_int' function */

/* Set variable pointer value */
void cfg_set_var_ptr( cfg_node_t *parent, const char *name, void *val )
{
	char str[32];
	snprintf(str, sizeof(str), "%p", val);
	cfg_set_var(parent, name, str);
} /* End of 'cfg_set_var_ptr' function */

/* Set variable integer float */
void cfg_set_var_float( cfg_node_t *parent, const char *name, float val )
{
	char str[32];
	snprintf(str, sizeof(str), "%f", val);
	cfg_set_var(parent, name, str);
} /* End of 'cfg_set_var_float' function */

/* Get variable value */
char *cfg_get_var( cfg_node_t *parent, const char *name )
{
	cfg_node_t *node;

	/* Get node */
	node = cfg_search_node(parent, name);
	if (node == NULL || !CFG_NODE_IS_VAR(node))
		return NULL;
	return CFG_VAR(node)->m_value;
} /* End of 'cfg_get_var' function */

/* Get variable integer value */
int cfg_get_var_int( cfg_node_t *parent, const char *name )
{
	char *str = cfg_get_var(parent, name);
	return (str == NULL) ? 0 : atoi(str);
} /* End of 'cfg_get_var_int' function */

/* Get variable pointer value */
void *cfg_get_var_ptr( cfg_node_t *parent, const char *name )
{
	void *value;
	char *str = cfg_get_var(parent, name);
	if (str == NULL)
		return NULL;
	sscanf(str, "%p", &value);
	return value;
} /* End of 'cfg_get_var_ptr' function */

/* Get variable float value */
float cfg_get_var_float( cfg_node_t *parent, const char *name )
{
	char *str = cfg_get_var(parent, name);
	return (str == NULL) ? 0. : atof(str);
} /* End of 'cfg_get_var_float' function */

/* Find the real parent of node */
cfg_node_t *cfg_find_real_parent( cfg_node_t *parent, const char *name, 
		const char **real_name )
{
	cfg_node_t *real_parent;
	char *item_name;

	assert(parent);
	assert(name);
	assert(CFG_NODE_IS_LIST(parent));

	/* Walk through parents */
	for ( real_parent = parent;; )
	{
		const char *next_name = strchr(name, '.');
		cfg_node_t *was_real = real_parent;

		/* If no '.' found - stop (we've found last parent in hierarchy) */
		if (next_name == NULL)
			break;

		/* Extract child list name */
		item_name = strndup(name, next_name - name);
		real_parent = cfg_search_list(real_parent, item_name);

		/* Create temporary list if it does not exist */
		if (real_parent == NULL)
			real_parent = cfg_new_list(was_real, item_name, NULL, 0, 0);

		/* Move to next child name */
		free(item_name);
		name = next_name + 1;
	}

	/* Set real name */
	if (real_name != NULL)
		(*real_name) = name;
	return real_parent;
} /* End of 'cfg_find_real_parent' function */

/* Search list for a child node (name given is the exact name, without dots) */
cfg_node_t *cfg_search_list( cfg_node_t *list, const char *name )
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
	return CFG_VAR_HANDLER(node)(node, value, CFG_VAR(node)->m_handler_data);
} /* End of 'cfg_call_var_handler' function */

/* Calculate string hash value */
int cfg_calc_hash( const char *str, int table_size )
{
	int val = 0;

	while (*str)
	{
		val += *str;
		str ++;
	}
	return val % table_size;
} /* End of 'cfg_calc_hash' function */

/* Set variable's handler */
void cfg_set_var_handler( cfg_node_t *parent, const char *name, 
		cfg_var_handler_t handler, void *handler_data )
{
	cfg_node_t *node;
	
	/* Search for this node */
	node = cfg_search_node(parent, name);

	/* Set handler of an existing variable */
	if (node != NULL && CFG_NODE_IS_VAR(node))
	{
		CFG_VAR(node)->m_handler = handler;
		CFG_VAR(node)->m_handler_data = handler_data;
	}
	/* Create node if not found */
	else if (node == NULL)
		cfg_new_var_full(parent, name, 0, NULL, handler, handler_data);
} /* End of 'cfg_set_var_handler' function */

/* Begin iteration */
cfg_list_iterator_t cfg_list_begin_iteration( cfg_node_t *list )
{
	cfg_list_iterator_t iter;
	iter.m_list = list;
	iter.m_cur_node = NULL;
	iter.m_hash_value = -1;
	return iter;
} /* End of 'cfg_list_begin_iteration' function */

/* Make an iteration */
cfg_node_t *cfg_list_iterate( cfg_list_iterator_t *iter )
{
	struct cfg_list_data_t *list;
	cfg_node_t *node;

	if (iter->m_list == NULL)
		return NULL;
	list = CFG_LIST(iter->m_list);

	/* Move to next hash item */
	if (iter->m_cur_node == NULL)
	{
		do
		{
			iter->m_hash_value ++;
		} while (iter->m_hash_value < list->m_hash_size && 
				(iter->m_cur_node = 
						 list->m_children[iter->m_hash_value]) == NULL);

		/* No record found */
		if (iter->m_hash_value >= list->m_hash_size)
			return NULL;
	}

	node = iter->m_cur_node->m_node;
	iter->m_cur_node = iter->m_cur_node->m_next;
	return node;
} /* End of 'cfg_list_iterate' function */

/* Copy node contents to another node */
void cfg_copy_node( cfg_node_t *dest, cfg_node_t *src )
{
	struct cfg_list_data_t *sl, *dl;
	int hash;
	struct cfg_list_hash_item_t *item;
	assert(dest);
	assert(src);

	/* Variables case is trivial */
	if (CFG_NODE_IS_VAR(src))
	{
		struct cfg_var_data_t *dv, *sv;
		struct cfg_var_op_list_t *opl;

		/* Copy data */
		assert(CFG_NODE_IS_VAR(dest));
		dv = CFG_VAR(dest);
		sv = CFG_VAR(src);
		dv->m_handler = sv->m_handler;
		dv->m_handler_data = sv->m_handler_data;

		/* Construct value */
		for ( opl = sv->m_operations; opl != NULL; opl = opl->m_next )
		{
			char *new_value = cfg_var_apply_op(dest, opl->m_arg, opl->m_op);
			if (dv->m_value != NULL)
				free(dv->m_value);
			dv->m_value = new_value;
		}
		cfg_free_node(src, FALSE);
		return;
	}

	/* Go through every source list item */
	assert(CFG_NODE_IS_LIST(src));
	sl = CFG_LIST(src);
	dl = CFG_LIST(dest);
	for ( hash = 0; hash < sl->m_hash_size; hash ++ )
	{
		for ( item = sl->m_children[hash]; item != NULL; item = item->m_next )
		{
			cfg_node_t *node = item->m_node;
			struct cfg_list_hash_item_t *i, *prev = NULL;

			/* Find corresponding destination item */
			int h = cfg_calc_hash(node->m_name, dl->m_hash_size);
			for ( i = dl->m_children[h]; i != NULL; prev = i, i = i->m_next )
			{
				/* Copy this node recursively */
				if (!strcmp(node->m_name, i->m_node->m_name))
				{
					cfg_copy_node(i->m_node, node);
					break;
				}
			}

			/* If not found simply insert */
			if (i == NULL)
			{
				struct cfg_list_hash_item_t *new_item;

				/* Create hash list item */
				new_item = (struct cfg_list_hash_item_t *)malloc(
						sizeof(*new_item));
				if (new_item == NULL)
					continue;
				new_item->m_node = node;
				new_item->m_next = NULL;

				/* Attach it */
				if (prev == NULL)
					dl->m_children[h] = new_item;
				else
					prev->m_next = new_item;
			}
		}
	}
	cfg_free_node(src, FALSE);
} /* End of 'cfg_copy_node' function */

/* End of 'cfg.c' file */

