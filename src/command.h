/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for player command message functions.
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

#ifndef __SG_MPFC_COMMAND_H__
#define __SG_MPFC_COMMAND_H__

#include <stdarg.h>
#include "types.h"
#include "wnd_types.h"

/* Command parameter */
typedef struct
{
	/* The value */
	union
	{
		char *m_string;
		int m_int;
	} m_value;

	/* Value type */
	enum 
	{
		CMD_PARAM_STRING,
		CMD_PARAM_INT
	} m_type;
} cmd_param_t;

/* List of parameters */
typedef struct
{
	/* Parameters */
	cmd_param_t *m_params;
	int m_num_params;

	/* List iterator */
	int m_iterator;
} cmd_params_list_t;

/* Construct parameters list */
cmd_params_list_t *cmd_create_params_va( char *params_fmt, va_list ap );

/* Get next string parameter from the list */
char *cmd_next_string_param( cmd_params_list_t *params );

/* Get next integer parameter from the list */
int cmd_next_int_param( cmd_params_list_t *params );

/* Check that there is the next parameter */
bool_t cmd_check_next_param( cmd_params_list_t *params );

/* Free parameters list */
void cmd_free_params( cmd_params_list_t *params );

/***
 * Message stuff
 ***/

/* Command message data */
typedef struct
{
	/* Command name */
	char *m_command;

	/* Command parameters */
	cmd_params_list_t *m_params;
} player_msg_command_t;

/* Create data for command message */
wnd_msg_data_t player_msg_command_new( char *cmd, cmd_params_list_t *params );

/* Free command message */
void player_msg_command_free( void *data );

/* Callback function for command message */
wnd_msg_retcode_t player_callback_command( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define PLAYER_MSG_COMMAND_HANDLER(h)	\
	((wnd_msg_retcode_t (*)(wnd_t *, char *, \
							cmd_params_list_t *params))(h->m_func))

#endif

/* End of 'command.h' file */

