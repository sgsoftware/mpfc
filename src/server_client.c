/******************************************************************
 * Copyright (C) 2011 by SG Software.
 *
 * SG MPFC. Server client interaction.
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
#include <errno.h>
#include <stdlib.h>
#include "player.h"
#include "server_client.h"

typedef union
{
	long num_param;
	char *str_param;
} param_t;

typedef enum
{
	PARAM_NONE,
	PARAM_INT,
	PARAM_STRING
} param_kind_t;

/* Parse command */
static bool_t server_client_parse_cmd( char *cmd, char **cmd_name,
	  	                               param_kind_t *param_kind, param_t *param )
{
	(*cmd_name) = cmd;

	/* Skip command name */
	for ( ;; cmd++ )
	{
		 if (!(isalnum(*cmd) || (*cmd) == '_'))
			 break;
	}

	if (!(*cmd))
	{
		(*param_kind) = PARAM_NONE;
		return TRUE;
	}

	/* Must be a space */
	if ((*cmd) != ' ')
		return FALSE;

	/* Make command name null-terminated */
	(*cmd++) = 0;

	/* Starting a number */
	if ((*cmd) == '-' || isdigit(*cmd))
	{
		char *endptr;
		long v;
		
		errno = 0;
		v = strtol(cmd, &endptr, 10);
		if (errno)
			return FALSE;

		/* Something left: it's an error */
		if (*endptr)
			return FALSE;

		(*param_kind) = PARAM_INT;
		param->num_param = v;
		return TRUE;
	}

	/* Starting a string */
	if ((*cmd) == '"')
	{
		char *p = ++cmd;

		/* Skip to the closing '"'
		 * TODO: handle escaping */
		for ( ; *cmd && (*cmd) != '"'; cmd++ )
			;
		if (!(*cmd))
			return FALSE;
		(*cmd) = 0;
		(*param_kind) = PARAM_STRING;
		param->str_param = p;
		return TRUE;
	}

	return FALSE;
} /* End of 'server_client_parse_cmd' function */

/* Send a notification to client */
void server_conn_client_notify(server_conn_desc_t *d, char nv)
{
} /* End of 'server_conn_client_notify' function */

/* Execute a command received from client */
void server_conn_exec_command(server_conn_desc_t *d)
{
	char *cmd_name;
	param_kind_t param_kind;
	param_t param;
	char *cmd = d->m_cur_cmd->m_data;

	logger_debug(player_log, "Received command '%s'", cmd);

	if (!server_client_parse_cmd(cmd, &cmd_name, &param_kind, &param))
	{
		logger_message(player_log, 0, "Error parsing command");
		return;
	}

	switch (param_kind)
	{
		case PARAM_NONE:
			logger_debug(player_log, "Received a no-param command '%s'", cmd_name);
			break;
		case PARAM_INT:
			logger_debug(player_log, "Received an int-param command '%s' with param %ld",
					cmd_name, param.num_param);
			break;
		case PARAM_STRING:
			logger_debug(player_log, "Received a string-param command '%s' with param '%s'",
					cmd_name, param.str_param);
			break;
	}
} /* End of 'server_conn_exec_command' function */

/* End of 'server_client.c' file */

