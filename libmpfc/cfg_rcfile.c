/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC Library. Configuration files manipulation functions 
 * implementation.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "mystring.h"
#include "util.h"

/* Stack */
#define CFG_RCFILE_STACK_SIZE 32
static struct
{
	char *m_name;
	bool_t m_fresh;
	int m_block_count;
} cfg_rcfile_stack[CFG_RCFILE_STACK_SIZE];
static int cfg_rcfile_stack_pos = -1;

/* Skip whitespaces in configuration file line */
static void cfg_rcfile_skip_ws( char **str )
{
	while (isspace(**str))
		(*str) ++;
} /* End of 'cfg_rcfile_skip_ws' function */

/* Read a string entry from configuration file line */
char *cfg_rcfile_read_str( char **str, char *token, char (*skipper)(char**) )
{
	char *ret;

	if (token != NULL)
		(*token) = 0;

	/* Skip initial white space */
	cfg_rcfile_skip_ws(str);
	if ((**str) == 0)
		return strdup("");

	/* Read quoted string */
	if ((**str) == '\"')
	{
		char *s;
		int i = 0, len;

		/* Determine result string length */
		(*str) ++;
		for ( len = 0, s = *str;; len ++, s ++ )
		{
			if ((*s) == 0)
				return strdup("");
			else if ((*s) == '\"')
				break;
			else if ((*s) == '\\')
			{
				s ++;
				if ((*s) == 0)
					return strdup("");
			}
		}

		/* Build string */
		ret = (char *)malloc(len + 1);
		if (ret == NULL)
			return strdup("");
		for ( ; (**str) != '\"'; (*str) ++ )
		{
			if ((**str) != '\\')
				ret[i ++] = **str;
			else
			{
				(*str) ++;
				switch (**str)
				{
				case 'n':
					ret[i ++] = '\n';
					break;
				case 't':
					ret[i ++] = '\t';
					break;
				case '\"':
					ret[i ++] = '\"';
					break;
				case '\\':
					ret[i ++] = '\\';
					break;
				case 'e':
					ret[i ++] = '\033';
					break;
				default:
					ret[i ++] = (**str);
					break;
				}
			}
		}
		ret[i] = 0;
		(*str) ++;
	}
	/* Read normal string */
	else
	{
		int right;
		char *s;

		/* Determine string right border */
		for ( right = 0, s = *str;; right ++, s ++ )
		{
			char c = 0;
			if ((*s) == 0)
				break;
			else if (isspace(*s))
			{
				s ++;
				break;
			}
			else if (skipper != NULL && ((c = skipper(&s)) != 0))
			{
				if (token != NULL)
					(*token) = c;
				break;
			}
		}

		/* Create string */
		ret = (char *)malloc(right + 1);
		if (ret == NULL)
			return strdup("");
		util_strncpy(ret, *str, right + 1);
		(*str) = s;
	}
	cfg_rcfile_skip_ws(str);
	if (skipper != NULL)
	{
		char c = skipper(str);
		if (c != 0)
		{
			if (token != NULL)
				(*token) = c;
			cfg_rcfile_skip_ws(str);
		}
	}
	return ret;
} /* End of 'cfg_rcfile_read_str' function */

/* Skipper function for parsing list operators */
static char cfg_rcfile_list_skipper( char **str )
{
	if ((**str) == ']')
	{
		(*str) ++;
		return ']';
	}
	return 0;
} /* End of 'cfg_rcfile_list_skipper' function */

/* Skipper function for parsing variable names */
static char cfg_rcfile_var_skipper( char **str )
{
	if ((**str) == '=')
	{
		(*str) ++;
		return '=';
	}
	else if (((**str) == '+' || (**str) == '-') && (*((*str) + 1) == '='))
	{
		char ret = **str;
		(*str) += 2;
		return ret;
	}
	return 0;
} /* End of 'cfg_rcfile_var_skipper' function */

/* Full version of reading configuration file function */
static void _cfg_rcfile_read( cfg_node_t *list, const char *name, bool_t nonrec )
{
	assert(list);
	assert(name);

	/* Try to open file */
	FILE *fd = fopen(name, "rt");
	if (fd == NULL)
		return;

	/* Read */
	while (!feof(fd))
	{
		/* Read line */
		str_t *str = util_fgets(fd);
		if (str == NULL)
			break;

		/* Parse this line */
		cfg_rcfile_parse_line(list, STR_TO_CPTR(str));
		str_free(str);

		/* Remove old items from stack */
		while (cfg_rcfile_stack_pos >= 0 &&
				(!cfg_rcfile_stack[cfg_rcfile_stack_pos].m_fresh) &&
				(cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count == 0))
			free(cfg_rcfile_stack[cfg_rcfile_stack_pos --].m_name);
		if (cfg_rcfile_stack_pos >= 0)
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_fresh = FALSE;
	}

	/* Close file */
	fclose(fd);

	/* Free stack */
	if (nonrec)
	{
		while (cfg_rcfile_stack_pos >= 0)
			free(cfg_rcfile_stack[cfg_rcfile_stack_pos --].m_name);
	}
} /* End of '_cfg_rcfile_read' function */

/* Read configuration file */
void cfg_rcfile_read( cfg_node_t *list, const char *name )
{
	/* Call a low-level version of this function */
	_cfg_rcfile_read(list, name, TRUE);
} /* End of 'cfg_rcfile_read' function */

/* Read one line from the configuration file */
void cfg_rcfile_parse_line( cfg_node_t *list, char *str )
{
	int i, j, len;
	char *var_name, *var_value, *real_name;
	char ch, token;
	cfg_var_op_t op;

	assert(list);
	assert(str);
	
	/* Skip initial whitespace and look at the first symbol */
	cfg_rcfile_skip_ws(&str);
	ch = *str;

	/* Comment or end of line */
	if (ch == '#' || (ch == 0))
		return;
	/* List operator */
	else if (ch == '[')
	{
		char *list_name;

		/* Read list name */
		cfg_rcfile_skip_ws(&str);
		str ++;
		list_name = cfg_rcfile_read_str(&str, NULL, cfg_rcfile_list_skipper);

		/* Push to the stack */
		if (cfg_rcfile_stack_pos < CFG_RCFILE_STACK_SIZE - 1)
		{
			cfg_rcfile_stack_pos ++;

			if (cfg_rcfile_stack_pos > 0)
			{
				char *real_name, *parent_name;
				parent_name = cfg_rcfile_stack[cfg_rcfile_stack_pos - 1].m_name;
				real_name = util_strcat(parent_name, ".", list_name, NULL);
				free(list_name);
				cfg_rcfile_stack[cfg_rcfile_stack_pos].m_name = real_name;
			}
			else
				cfg_rcfile_stack[cfg_rcfile_stack_pos].m_name = list_name;
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_fresh = TRUE;
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count = 0;
		}
		else
			free(list_name);
		return;
	}
	/* Block start/end */
	else if (ch == '{')
	{
		if (cfg_rcfile_stack_pos >= 0)
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count ++;
		return;
	}
	else if (ch == '}')
	{
		if (cfg_rcfile_stack_pos >= 0)
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count --;
		return;
	}
	/* Include a file */
	else if (!strncmp(str, "include", 7))
	{
		char *fname;

		str += 7;
		cfg_rcfile_skip_ws(&str);
		fname = cfg_rcfile_read_str(&str, NULL, NULL);
		if (cfg_rcfile_stack_pos >= 0)
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count ++;
		_cfg_rcfile_read(list, fname, FALSE);
		if (cfg_rcfile_stack_pos >= 0)
			cfg_rcfile_stack[cfg_rcfile_stack_pos].m_block_count --;
		free(fname);
		return;
	}

	/* Else - parse variable */
	var_name = cfg_rcfile_read_str(&str, &token, cfg_rcfile_var_skipper);
	if (token != 0)
		var_value = cfg_rcfile_read_str(&str, NULL, NULL);
	else
		var_value = strdup("1");

	/* Get variable full name */
	if (cfg_rcfile_stack_pos >= 0)
	{
		char *list_name = cfg_rcfile_stack[cfg_rcfile_stack_pos].m_name;
		real_name = util_strcat(list_name, ".", var_name, NULL);
	}
	else
		real_name = var_name;

	/* Set variable */
	if (token == '+')
		op = CFG_VAR_OP_ADD;
	else if (token == '-')
		op = CFG_VAR_OP_REM;
	else
		op = CFG_VAR_OP_SET;
	cfg_set_var_full(list, real_name, var_value, op);

	/* Free memory */
	if (real_name != var_name)
		free(real_name);
	free(var_name);
	free(var_value);
} /* End of 'cfg_rcfile_parse_line' function */

/* Save a node to file */
void cfg_rcfile_save_node( FILE *fd, cfg_node_t *node, const char *prefix )
{
	/* Save a variable */
	if (CFG_NODE_IS_VAR(node))
	{
		char *value = CFG_VAR_VALUE(node);
		if (value != NULL)
		{
			fprintf(fd, "%s%s = ", prefix == NULL ? "" : prefix, 
					node->m_name);
			/* Output a quoted value */
			fprintf(fd, "\"");
			for ( ; *value; value ++ )
			{
				char ch = *value;
				if (ch == '\n')
					fprintf(fd, "\\n");
				else if (ch == '\"')
					fprintf(fd, "\\\"");
				else if (ch == '\\')
					fprintf(fd, "\\\\");
				else if (ch == 27)
					fprintf(fd, "\\e");
				else
					fprintf(fd, "%c", ch);
			}
			fprintf(fd, "\"\n");
		}
	}
	/* Save a list */
	else
	{
		char *this_prefix;
		
		this_prefix = util_strcat(prefix == NULL ? "" : prefix,
				node->m_name, ".", NULL);
		cfg_list_iterator_t iter = cfg_list_begin_iteration(node);
		for ( ;; )
		{
			cfg_node_t *child = cfg_list_iterate(&iter);
			if (child == NULL)
				break;
			cfg_rcfile_save_node(fd, child, this_prefix);
		}
		free(this_prefix);
	}
} /* End of 'cfg_rcfile_save_node' function */

/* End of 'cfg_rcfile.c' file */

