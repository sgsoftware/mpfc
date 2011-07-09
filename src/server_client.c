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
#include <string.h>
#include <json/json.h>
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

/* Send a buffer */
bool_t server_conn_send_buf(server_conn_desc_t *d, const char *msg, int len)
{
	while (len > 0)
	{
		int sent = send(d->m_socket, msg, len, 0);
		if (sent < 0)
		{
			logger_debug(player_log, "Error sending response");
			return FALSE;
		}

		len -= sent;
		msg += sent;
	}
	return TRUE;
} /* End of 'server_conn_send_buf' function */

/* Send a response to client and free message memory */
void server_conn_response(server_conn_desc_t *d, const char *msg)
{
	int len = strlen(msg);
	char header[128];

	snprintf(header, sizeof(header), "Msg-Length: %d\nMsg-Type: response\n", len);
	if (!server_conn_send_buf(d, header, strlen(header)))
		return;
	server_conn_send_buf(d, msg, len);
} /* End of 'server_conn_response' function */

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
		logger_debug(player_log, "Error parsing command");
		return;
	}

	/* Execute */
	if (!strcmp(cmd_name, "play"))
	{
		int song = (param_kind == PARAM_INT ? param.num_param : 0);
		player_start_play(song, 0);
	}
	else if (!strcmp(cmd_name, "resume"))
	{
		player_pause_resume();
	}
	else if (!strcmp(cmd_name, "pause"))
	{
		player_pause_resume();
	}
	else if (!strcmp(cmd_name, "stop"))
	{
		player_stop();
	}
	else if (!strcmp(cmd_name, "next"))
	{
		player_skip_songs(1, TRUE);
	}
	else if (!strcmp(cmd_name, "prev"))
	{
		player_skip_songs(-1, TRUE);
	}
	else if (!strcmp(cmd_name, "time_back"))
	{
		player_time_back();
	}
	else if (!strcmp(cmd_name, "get_cur_song"))
	{
		struct json_object *js = json_object_new_object();
		int cur_song = player_plist->m_cur_song;

		json_object_object_add(js, "position", json_object_new_int(cur_song));
		if (cur_song >= 0)
		{
			const char *status = "";
			song_t *s = player_plist->m_list[cur_song];
			json_object_object_add(js, "title", json_object_new_string(
						STR_TO_CPTR(s->m_title)));
			json_object_object_add(js, "time", json_object_new_int(
						player_context->m_cur_time));
			json_object_object_add(js, "length", json_object_new_int(
						s->m_len));

			if (player_context->m_status == PLAYER_STATUS_PLAYING)
				status = "playing";
			else if (player_context->m_status == PLAYER_STATUS_PAUSED)
				status = "paused";
			else if (player_context->m_status == PLAYER_STATUS_STOPPED)
				status = "stopped";
			json_object_object_add(js, "play_status", json_object_new_string(status));
		}

		server_conn_response(d, json_object_get_string(js));
		json_object_put(js);
	}
	else if (!strcmp(cmd_name, "get_playlist"))
	{
		int i;
		struct json_object *js = json_object_new_array();

		for ( i = 0; i < player_plist->m_len; i++ )
		{
			struct json_object *js_child = json_object_new_object();
			song_t *s = player_plist->m_list[i];
			json_object_object_add(js_child, "title", json_object_new_string(
						STR_TO_CPTR(s->m_title)));
			json_object_object_add(js_child, "length", json_object_new_int(s->m_len));
			json_object_array_add(js, js_child);
		}

		server_conn_response(d, json_object_get_string(js));
		json_object_put(js);
	}
	wnd_invalidate(player_wnd);
} /* End of 'server_conn_exec_command' function */

/* End of 'server_client.c' file */

