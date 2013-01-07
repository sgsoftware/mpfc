/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for configuration handling.
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

#ifndef __SG_MPFC_CFG_H__
#define __SG_MPFC_CFG_H__

#include <stdio.h>
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
	CFG_NODE_HANDLE_AFTER_CHANGE	= 1 << 5,
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
typedef bool_t (*cfg_var_handler_t)( struct tag_cfg_node_t *var, char *value,
		void *data );

/* Variable operation type */
typedef enum
{
	CFG_VAR_OP_SET = 0,
	CFG_VAR_OP_ADD,
	CFG_VAR_OP_REM
} cfg_var_op_t;

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
			void *m_handler_data;

			/* Variable operations list */
			struct cfg_var_op_list_t
			{
				cfg_var_op_t m_op;
				char *m_arg;
				struct cfg_var_op_list_t *m_next;
			} *m_operations;
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

/* Set the list variables default values function */
typedef void (*cfg_set_default_values_t)( cfg_node_t *list );

/* Access node fields */
#define CFG_NODE_IS_VAR(node)		((node)->m_flags & CFG_NODE_VAR)
#define CFG_NODE_IS_LIST(node)		(!((node)->m_flags & CFG_NODE_VAR))
#define CFG_LIST(node)				(&((node)->m_data.m_list))
#define CFG_VAR(node)				(&((node)->m_data.m_var))
#define CFG_VAR_VALUE(node)			(CFG_VAR(node)->m_value)
#define CFG_VAR_HANDLER(node)		(CFG_VAR(node)->m_handler)

/* Create a new configuration list */
cfg_node_t *cfg_new_list( cfg_node_t *parent, const char *name, 
		cfg_set_default_values_t set_def, dword flags, int hash_size );

/* Create a new variable */
cfg_node_t *cfg_new_var_full( cfg_node_t *parent, const char *name, dword flags, 
		char *value, cfg_var_handler_t handler, void *handler_data );
#define cfg_new_var(parent, name, flags, value, handler) \
	cfg_new_var_full(parent, name, flags, value, handler, NULL)

/* Create a new node and leave node type specific information unset 
 * (don't use this function directly; use previous two instead) */
cfg_node_t *cfg_new_node( cfg_node_t *parent, const char *name, dword flags );

/* Insert a node into the list */
void cfg_insert_node( cfg_node_t *list, cfg_node_t *node );

/* Copy node contents to another node */
void cfg_copy_node( cfg_node_t *dest, cfg_node_t *src );

/* Free node */
void cfg_free_node( cfg_node_t *node, bool_t recursively );

/* Free variable operations list */
void cfg_var_free_operations( cfg_node_t *node );

/* Search for the node */
cfg_node_t *cfg_search_node( cfg_node_t *parent, const char *name );

/* Apply operation to variable value */
char *cfg_var_apply_op( cfg_node_t *node, const char *value, cfg_var_op_t op );

/* Full version of variable value setting */
void cfg_set_var_full( cfg_node_t *parent, const char *name, char const *val, 
		cfg_var_op_t op );

/* Set variable value */
void cfg_set_var( cfg_node_t *parent, const char *name, const char *val );

/* Set variable integer value */
void cfg_set_var_int( cfg_node_t *parent, const char *name, int val );

/* Set variable integer float */
void cfg_set_var_float( cfg_node_t *parent, const char *name, float val );

/* Set variable pointer value */
void cfg_set_var_ptr( cfg_node_t *parent, const char *name, void *val );

/* Get variable value */
char *cfg_get_var( cfg_node_t *parent, const char *name );

/* Get variable integer value */
int cfg_get_var_int( cfg_node_t *parent, const char *name );

/* Get variable float value */
float cfg_get_var_float( cfg_node_t *parent, const char *name );

/* Get variable pointer value */
void *cfg_get_var_ptr( cfg_node_t *parent, const char *name );

/* Set variable's handler */
void cfg_set_var_handler( cfg_node_t *parent, const char *name, 
		cfg_var_handler_t handler, void *handler_data );

/* Setters/getters for some other types */
#define cfg_set_var_bool(parent, name, val)	\
	(cfg_set_var_int(parent, name, (int)val))
#define cfg_get_var_bool(parent, name)	\
	((bool_t)cfg_get_var_int(parent, name))

/* Find the real parent of node */
cfg_node_t *cfg_find_real_parent( cfg_node_t *parent, const char *name, 
		const char **real_name );

/* Search list for a child node (name given is the exact name, without dots) */
cfg_node_t *cfg_search_list( cfg_node_t *list, const char *name );

/* Call variable handler */
bool_t cfg_call_var_handler( bool_t after, cfg_node_t *node, char *value );

/* Calculate string hash value */
int cfg_calc_hash( const char *str, int table_size );

/*
 * Configuration list iteration functions
 */

/* Iterator type */
typedef struct
{
	/* List being iterated */
	cfg_node_t *m_list;

	/* Current node */
	struct cfg_list_hash_item_t *m_cur_node;

	/* Current hash value */
	int m_hash_value;
} cfg_list_iterator_t;

/* Begin iteration */
cfg_list_iterator_t cfg_list_begin_iteration( cfg_node_t *list );

/* Make an iteration */
cfg_node_t *cfg_list_iterate( cfg_list_iterator_t *iter );

/*
 * Configuration files manipulation functions
 */

/* Read configuration file */
void cfg_rcfile_read( cfg_node_t *list, const char *name );

/* Read one line from the configuration file */
void cfg_rcfile_parse_line( cfg_node_t *list, char *str );

/* Save a node to file */
void cfg_rcfile_save_node( FILE *fd, cfg_node_t *node, const char *prefix );

#endif

/* End of 'cfg.h' file */

