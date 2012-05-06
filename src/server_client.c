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
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <json/json.h>
#include "file_utils.h"
#include "player.h"
#include "server_client.h"
#include "util.h"

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

/* Build notification message */
void server_conn_notification_msg(char nv, char *msg, int buf_size)
{
	switch (nv)
	{
		case SERVER_NOTIFY_PLAYLIST:
			strncpy(msg, "playlist", buf_size);
			break;
		case SERVER_NOTIFY_STATUS:
			strncpy(msg, "status", buf_size);
			break;
		default:
			strncpy(msg, "", buf_size);
			break;
	}
} /* End of 'server_conn_notification_msg' function */

/* Send a notification to client */
void server_conn_client_notify(server_conn_desc_t *d, char nv)
{
	char msg[128];
	char header[128];
	int len;

	server_conn_notification_msg(nv, msg, sizeof(msg));
	len = strlen(msg);

	snprintf(header, sizeof(header), "Msg-Length: %d\nMsg-Type: n\n", len);
	if (!server_conn_send_buf(d, header, strlen(header)))
		return;
	server_conn_send_buf(d, msg, len);
} /* End of 'server_conn_client_notify' function */

/* Send a response to client and free message memory */
void server_conn_response(server_conn_desc_t *d, const char *msg)
{
	int len = strlen(msg);
	char header[128];

	snprintf(header, sizeof(header), "Msg-Length: %d\nMsg-Type: r\n", len);
	if (!server_conn_send_buf(d, header, strlen(header)))
		return;
	server_conn_send_buf(d, msg, len);
} /* End of 'server_conn_response' function */

/* Validate file name. It shall not contain '..' */
static bool_t is_valid_file_name(char *name)
{
	if (name[0] != '/')
		return FALSE;

	for ( ;; )
	{
		assert((*name) == '/');

		name++;
		if (name[0] == '.' && name[1] == '.' &&
				(name[2] == 0 || name[2] == '/'))
			return FALSE;
		name = strchr(name, '/');
		if (!name)
			break;
	}

	return TRUE;
} /* End of 'is_valid_file_name' function */

/* Translate virtual file name */
static char *translate_file_name(char *name)
{
	char *r = cfg_get_var(cfg_list, "remote-dir-root");
	if (!r)
		return NULL;

	if (!is_valid_file_name(name))
		return NULL;

	return util_strcat(r, name, NULL);
} /* End of 'translate_file_name' function */

/* Execute 'list_dir' command */
static void server_conn_list_dir(char *name, struct json_object *js)
{
	char *real_name = NULL;
	fu_dir_t *dir = NULL;

	/* Translate virtual directory name */
	real_name = translate_file_name(name);
	if (!real_name)
		goto finally;

	/* Open directory */
	dir = fu_opendir(real_name);
	if (!dir)
		goto finally;

	for ( ;; )
	{
		struct dirent *de = fu_readdir(dir);
		if (!de)
			break;

		/* Skip special dirs */
		if (fu_is_special_dir(de->d_name))
			continue;

		/* Determine file type */
		bool_t is_dir;
		{
			char *full_name = util_strcat(real_name, "/", de->d_name, NULL);
			bool_t ok = fu_file_type(full_name, &is_dir);
			free(full_name);
			if (!ok)
				continue;
		}

		struct json_object *js_child = json_object_new_object();
		json_object_object_add(js_child, "name", json_object_new_string(de->d_name));
		json_object_object_add(js_child, "type",
				json_object_new_string(is_dir ? "d" : "f"));
		json_object_array_add(js, js_child);
	}

finally:
	if (dir)
		fu_closedir(dir);
	if (real_name)
		free(real_name);
} /* End of 'server_conn_list_dir' function */

/* Execute a command received from client */
bool_t server_conn_exec_command(server_conn_desc_t *d)
{
	char *cmd_name;
	param_kind_t param_kind;
	param_t param;
	char *cmd = d->m_cur_cmd->m_data;

	logger_debug(player_log, "Received command '%s'", cmd);

	if (!server_client_parse_cmd(cmd, &cmd_name, &param_kind, &param))
	{
		logger_debug(player_log, "Error parsing command");
		return TRUE;
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
	else if (!strcmp(cmd_name, "list_dir"))
	{
		struct json_object *js = json_object_new_array();
		if (param_kind == PARAM_STRING)
			server_conn_list_dir(param.str_param, js);
		server_conn_response(d, json_object_get_string(js));
		json_object_put(js);
	}
	else if (!strcmp(cmd_name, "add"))
	{
		if (param_kind == PARAM_STRING)
		{
			char *real_name = translate_file_name(param.str_param);
			plist_add(player_plist, real_name);
			free(real_name);
		}
					
	}
	else if (!strcmp(cmd_name, "remove"))
	{
		if (param_kind == PARAM_INT)
		{
			int pos = param.num_param;
			plist_move(player_plist, pos, FALSE);
			plist_rem(player_plist);
		}
	}
	else if (!strcmp(cmd_name, "queue"))
	{
		if (param_kind == PARAM_INT)
		{
			int pos = param.num_param;
			plist_move(player_plist, pos, FALSE);
			player_queue_song();
		}
	}
	else if (!strcmp(cmd_name, "seek"))
	{
		if (param_kind == PARAM_INT)
		{
			int t = param.num_param;
			player_seek(t, FALSE);
		}
	}
	else if (!strcmp(cmd_name, "clear_playlist"))
	{
		plist_clear(player_plist);
	}
	else if (!strcmp(cmd_name, "bye"))
	{
		return FALSE;
	}
	wnd_invalidate(player_wnd);
	return TRUE;
} /* End of 'server_conn_exec_command' function */

/* End of 'server_client.c' file */

