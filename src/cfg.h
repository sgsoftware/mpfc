/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : cfg.h
 * PURPOSE     : SG MPFC. Interface for configuration handling
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.07.2004
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

/* Node flags */
typedef enum
{
	/* Whether node represents a variable instead of a list */
	CFG_NODE_VAR					= 1 << 0,

	/* Flags for list creation time - specify how big will be the list.
	 * They are used to choose a sensible hash table size.
	 * Note that they make sense if the size is not specified directly */
	CFG_NODE_SMALL_LIST				= 1 << 1,
	CFG_NODE_MEDIUM_LIST			= 1 << 2,
	CFG_NODE_BIG_LIST				= 1 << 3,

	/* Variable is run-time. This means that it is not saved when exiting */
	CFG_NODE_RUNTIME				= 1 << 4,

	/* Variable handler is called after changing the value */
	CFG_NODE_HANDLE_AFTER_CHANGE	= 1 << 5
} cfg_node_flags_t;

/* Default values for list hash table size */
#define CFG_HASH_SMALL_SIZE		5
#define CFG_HASH_MEDIUM_SIZE	20
#define CFG_HASH_BIG_SIZE		50
#define CFG_HASH_DEFAULT_SIZE	CFG_HASH_MEDIUM_SIZE

/* Handler for a variable (a function that is called whenever variable 
 * value is changed) type. 
 * If this function returns FALSE (and flag CFG_NODE_HANDLE_AFTER_CHANGE
 * is not set), change does not actually occur */
struct tag_cfg_node_t;
typedef bool_t (*cfg_var_handler_t)( struct tag_cfg_node_t *var, char *value );

/* Configuration tree node */
typedef struct tag_cfg_node_t
{
	/* Node name */
	char *m_name;

	/* Node flags */
	cfg_node_flags_t m_flags;

	/* Link to the parent list */
	struct tag_cfg_node_t *m_parent;

	/* Node type specific data */
	union
	{
		/* Data for variable (if flag CFG_NODE_VAR is set) */
		struct cfg_var_data_t
		{
			/* Variable value */
			char *m_value;

			/* Handler for this variable (a function that is called
			 * whenever variable value is changed) */
			cfg_var_handler_t m_handler;
		} m_var;

		/* Data for list */
		struct cfg_list_data_t
		{
			/* Child items hash */
			struct cfg_list_hash_item_t
			{
				struct tag_cfg_node_t *m_node;
				struct cfg_list_hash_item_t *m_next;
			} **m_children;
			int m_hash_size;
		} m_list;
	} m_data;
} cfg_node_t;

/* Alias for compatibility with some old code */
typedef cfg_node_t cfg_list_t;

/* Access node fields */
#define CFG_NODE_IS_VAR(node)		((node)->m_flags & CFG_NODE_VAR)
#define CFG_NODE_IS_LIST(node)		(!((node)->m_flags & CFG_NODE_VAR))
#define CFG_LIST(node)				(&((node)->m_data.m_list))
#define CFG_VAR(node)				(&((node)->m_data.m_var))
#define CFG_VAR_VALUE(node)			(CFG_VAR(node)->m_value)
#define CFG_VAR_HANDLER(node)		(CFG_VAR(node)->m_handler)

/* Create a new configuration list */
cfg_node_t *cfg_new_list( cfg_node_t *parent, char *name, dword flags,
		int hash_size );

/* Create a new variable */
cfg_node_t *cfg_new_var( cfg_node_t *parent, char *name, dword flags, 
		char *value, cfg_var_handler_t handler );

/* Create a new node and leave node type specific information unset 
 * (don't use this function directly; use previous two instead) */
cfg_node_t *cfg_new_node( cfg_node_t *parent, char *name, dword flags );

/* Insert a node into the list */
void cfg_insert_node( cfg_node_t *list, cfg_node_t *node );

/* Free node */
void cfg_free_node( cfg_node_t *node );

/* Search for the node */
cfg_node_t *cfg_search_node( cfg_node_t *parent, char *name );

/* Set variable value */
void cfg_set_var( cfg_node_t *parent, char *name, char *val );

/* Set variable integer value */
void cfg_set_var_int( cfg_node_t *parent, char *name, int val );

/* Set variable integer float */
void cfg_set_var_float( cfg_node_t *parent, char *name, float val );

/* Get variable value */
char *cfg_get_var( cfg_node_t *parent, char *name );

/* Get variable integer value */
int cfg_get_var_int( cfg_node_t *parent, char *name );

/* Get variable float value */
float cfg_get_var_float( cfg_node_t *parent, char *name );

/* Find the real parent of node */
cfg_node_t *cfg_find_real_parent( cfg_node_t *parent, char *name, 
		char **real_name );

/* Search list for a child node (name given is the exact name, without dots) */
cfg_node_t *cfg_search_list( cfg_node_t *list, char *name );

/* Call variable handler */
bool_t cfg_call_var_handler( bool_t after, cfg_node_t *node, char *value );

/* Calculate string hash value */
int cfg_calc_hash( char *str, int table_size );

#endif

/* End of 'cfg.h' file */

