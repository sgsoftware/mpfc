/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Player command message functions implementation.
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

#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "command.h"
#include "wnd.h"

/* Construct parameters list */
cmd_params_list_t *cmd_create_params_va( char *params_fmt, va_list ap )
{
	cmd_params_list_t *params;

	/* Allocate memory */
	params = (cmd_params_list_t *)malloc(sizeof(*params));
	if (params == NULL)
		return params;
	memset(params, 0, sizeof(*params));

	/* Get parameters */
	params->m_num_params = (params_fmt == 0 ? 0 : strlen(params_fmt));
	if (params->m_num_params)
	{
		int i;

		params->m_params = (cmd_param_t *)malloc(params->m_num_params *
				sizeof(cmd_param_t));
		for ( i = 0; i < params->m_num_params; i ++ )
		{
			char fmt = params_fmt[i];
			if (fmt == 'i')
			{
				params->m_params[i].m_type = CMD_PARAM_INT;
				params->m_params[i].m_value.m_int = va_arg(ap, int);
			}
			else if (fmt == 's')
			{
				params->m_params[i].m_type = CMD_PARAM_STRING;
				params->m_params[i].m_value.m_string = 
					strdup(va_arg(ap, char*));
			}
		}
	}
	return params;
} /* End of 'cmd_create_params_va' function */

/* Get next string parameter from the list */
char *cmd_next_string_param( cmd_params_list_t *params )
{
	cmd_param_t param;

	assert(params);
	if (params->m_iterator >= params->m_num_params)
		return NULL;

	param = params->m_params[params->m_iterator ++];
	if (param.m_type == CMD_PARAM_STRING)
		return strdup(param.m_value.m_string);
	else if (param.m_type == CMD_PARAM_INT)
	{
		char *res = (char *)malloc(10);
		snprintf(res, 10, "%d", param.m_value.m_int);
		return res;
	}
	return NULL;
} /* End of 'cmd_next_string_param' function */

/* Get next integer parameter from the list */
int cmd_next_int_param( cmd_params_list_t *params )
{
	cmd_param_t param;

	assert(params);
	if (params->m_iterator >= params->m_num_params)
		return 0;

	param = params->m_params[params->m_iterator ++];
	if (param.m_type == CMD_PARAM_INT)
		return param.m_value.m_int;
	else if (param.m_type == CMD_PARAM_STRING)
		return atoi(param.m_value.m_string);
	return 0;
} /* End of 'cmd_next_int_param' function */

/* Free parameters list */
void cmd_free_params( cmd_params_list_t *params )
{
	int i;
	assert(params);
	for ( i = 0; i < params->m_num_params; i ++ )
	{
		if (params->m_params[i].m_type == CMD_PARAM_STRING)
			free(params->m_params[i].m_value.m_string);
	}
	if (params->m_params != NULL)
		free(params->m_params);
	free(params);
} /* End of 'cmd_free_params' function */

/***
 * Message stuff
 ***/

/* Create data for command message */
wnd_msg_data_t player_msg_command_new( char *cmd, cmd_params_list_t *params )
{
	wnd_msg_data_t msg_data;
	player_msg_command_t *data;
	va_list ap;

	data = (player_msg_command_t *)malloc(sizeof(*data));
	data->m_command = strdup(cmd);
	data->m_params = params;
	msg_data.m_data = data;
	msg_data.m_destructor = player_msg_command_free;
	return msg_data;
} /* End of 'player_msg_command_new' function */

/* Free command message */
void player_msg_command_free( void *data )
{
	player_msg_command_t *cmd = (player_msg_command_t *)data;
	free(cmd->m_command);
	cmd_free_params(cmd->m_params);
} /* End of 'player_msg_command_free' function */

/* Callback function for command message */
wnd_msg_retcode_t player_callback_command( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	player_msg_command_t *cmd = (player_msg_command_t *)(msg_data->m_data);
	return PLAYER_MSG_COMMAND_HANDLER(handler)(wnd, cmd->m_command,
			cmd->m_params);
} /* End of 'player_callback_command' function */

/* End of 'command.c' file */

