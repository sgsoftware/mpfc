/******************************************************************
 * Copyright (C) 2011 by SG Software.
 *
 * SG MPFC. Server implementation.
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

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "cfg.h"
#include "pmng.h"
#include "player.h"
#include "rd_with_notify.h"
#include "server_client.h"

int server_socket = -1;
pthread_t server_tid = -1;

rd_with_notify_t *server_rdwn = NULL;

server_conn_desc_t *server_conns = NULL;

int server_hook_id = -1;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *server_thread( void * );
static void *server_conn_thread( void * );

static void server_conn_notify( server_conn_desc_t *conn, char nv );

static void server_hook_handler( char *hook );

/* Create a new connection descriptor */
server_conn_desc_t *server_conn_desc_new( int sock )
{
	server_conn_desc_t *conn_desc =
		(server_conn_desc_t *)malloc(sizeof(server_conn_desc_t));
	if (!conn_desc)
	{
		logger_error(player_log, 0, _("No enough memory!"));
		return NULL;
	}

	conn_desc->m_socket = sock;
	conn_desc->m_buf[0] = 0;
	conn_desc->m_cur_cmd = str_new("");
	if (!conn_desc->m_cur_cmd)
	{
		free(conn_desc);
		logger_error(player_log, 0, _("No enough memory!"));
		return NULL;
	}

	conn_desc->m_rdwn = rd_with_notify_new(sock);
	if (!conn_desc->m_rdwn)
	{
		logger_error(player_log, 0,
				_("Connection notification pipe create failed: %s"),
				strerror(errno));
		str_free(conn_desc->m_cur_cmd);
		free(conn_desc);
		return NULL;
	}

	/* List management */
	pthread_mutex_lock(&server_mutex);
	conn_desc->m_prev = NULL;
	conn_desc->m_next = server_conns;
	if (server_conns)
		server_conns->m_prev = conn_desc;
	server_conns = conn_desc;
	pthread_mutex_unlock(&server_mutex);
	return conn_desc;
} /* End of 'server_conn_desc_new' function */

/* Free connection descriptor */
void server_conn_desc_free( server_conn_desc_t *conn_desc )
{
	pthread_mutex_lock(&server_mutex);
	close(conn_desc->m_socket);
	str_free(conn_desc->m_cur_cmd);
	rd_with_notify_free(conn_desc->m_rdwn);

	/* List management */
	if (conn_desc->m_prev)
		conn_desc->m_prev->m_next = conn_desc->m_next;
	if (conn_desc->m_next)
		conn_desc->m_next->m_prev = conn_desc->m_prev;
	if (conn_desc == server_conns)
		server_conns = conn_desc->m_next;

	free(conn_desc);
	pthread_mutex_unlock(&server_mutex);
} /* End of 'server_conn_desc_free' function */

/* Start the server */
bool_t server_start( void )
{
	struct sockaddr_in addr;
	int err, i;

	int server_port = cfg_get_var_int(cfg_list, "server-port");
	if (!server_port)
		server_port = 0x4D50; /* 'MP' / 19792 */

	int server_port_pool_size = cfg_get_var_int(cfg_list, "server-port-pool-size");
	if (!server_port_pool_size)
		server_port_pool_size = 10;

	logger_message(player_log, 0, _("Starting the server at port %d"), server_port);

	/* Create socket */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		logger_error(player_log, 0,
				_("Server socket creation failed: %s"),
				strerror(errno));
		goto failed;
	}

	/* Bind */
	for ( int i = 0; i < server_port_pool_size; i++, server_port++ )
	{
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(server_port);
		addr.sin_family = AF_INET;
		if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) != -1)
			goto bind_succeeded;
		else
			logger_error(player_log, 0,
					_("Server socket bind at port %d failed: %s"),
					server_port, strerror(errno));
	}

bind_succeeded:

	logger_message(player_log, 0, _("Server listening at port %d"), server_port);

	/* Listen */
	if (listen(server_socket, 5) == -1)
	{
		logger_error(player_log, 0,
				_("Server socket listen failed: %s"),
				strerror(errno));
		goto failed;
	}

	/* Create rdwn */
	server_rdwn = rd_with_notify_new(server_socket);
	if (!server_rdwn)
	{
		logger_error(player_log, 0,
				_("Server notification pipe create failed: %s"),
				strerror(errno));
		goto failed;
	}

	/* Start the main thread */
	err = pthread_create(&server_tid, NULL, server_thread, NULL);
	if (err)
	{
		logger_error(player_log, 0,
				_("Server thread create failed: %s"),
				strerror(err));
		goto failed;
	}

	/* Install hook handler */
	server_hook_id = pmng_add_hook_handler(player_pmng, server_hook_handler);

	return TRUE;

failed:
	if (server_socket != -1)
	{
		close(server_socket);
		server_socket = -1;
	}
	if (server_rdwn)
	{
		rd_with_notify_free(server_rdwn);
		server_rdwn = NULL;
	}
	return FALSE;
} /* End of 'server_start' function */

/* Stop server */
void server_stop( void )
{
	if (server_socket == -1)
		return;

	/* Uninstall hook handler */
	pmng_remove_hook_handler(player_pmng, server_hook_id);

	/* Notify the thread about exit */
	char val = 0;
	write(RDWN_NOTIFY_WRITE_FD(server_rdwn), &val, 1);
	pthread_join(server_tid, NULL);

	/* Close connections */
	for ( ;; )
	{
		pthread_mutex_lock(&server_mutex);
		if (server_conns)
		{
			pthread_t tid = server_conns->m_tid;
			server_conn_notify(server_conns, SERVER_NOTIFY_EXIT);
			pthread_mutex_unlock(&server_mutex);

			pthread_join(tid, NULL);
		}
		else
		{
			pthread_mutex_unlock(&server_mutex);
			break;
		}
	}

	/* Close pipe */
	rd_with_notify_free(server_rdwn);
	server_rdwn = NULL;

	/* Close socket */
	close(server_socket);
	server_socket = -1;

	pthread_mutex_destroy(&server_mutex);
} /* End of 'server_stop' function */

/* The main server thread function
 * It is responsible for accepting connections and launching 
 * per-connection threads */
static void *server_thread( void *p )
{
	for ( ;; )
	{
		/* Wait for input */
		int retv = rd_with_notify_wait(server_rdwn);
		if (!retv)
		{
			logger_error(player_log, 0,
					_("Server select failed: %s"),
					strerror(errno));
			return NULL;
		}

		/* Exit */
		if (retv & RDWN_NOTIFY_READY)
			break;

		/* Try to accept connection */
		if (retv & RDWN_READ_READY)
		{
			server_conn_desc_t *conn_desc = NULL;
			int conn_socket = -1;
			int err;

			conn_socket = accept(server_socket, NULL, NULL);
			if (conn_socket == -1)
			{
				logger_error(player_log, 0,
						_("Server socket accept failed: %s"),
						strerror(errno));
				goto failure;
			}

			logger_message(player_log, 0, _("Received a connection"));

			/* Start the connection thread */
			conn_desc = server_conn_desc_new(conn_socket);
			if (!conn_desc)
			{
				close(conn_socket);
				goto failure;
			}
			err = pthread_create(&conn_desc->m_tid, NULL,
					server_conn_thread, conn_desc);
			if (err)
			{
				logger_error(player_log, 0,
						_("Server connection thread create failed: %s"),
						strerror(err));
				goto failure;
			}

			continue;
		failure:
			if (conn_desc)
				server_conn_desc_free(conn_desc);
			continue;
		}
	}

	return NULL;
} /* End of 'server_thread' function */

/* Hook handler to send notifications */
static void server_hook_handler( char *hook )
{
	char nv;
	server_conn_desc_t *conn;

	/* Determine notification code */
	if (!strcmp(hook, "playlist"))
		nv = SERVER_NOTIFY_PLAYLIST;
	else if (!strcmp(hook, "player-status"))
		nv = SERVER_NOTIFY_STATUS;
	else
		return;

	/* Notify all clients */
	pthread_mutex_lock(&server_mutex);
	for ( conn = server_conns; conn; conn = conn->m_next )
	{
		server_conn_notify(conn, nv);
	}
	pthread_mutex_unlock(&server_mutex);
} /* End of 'server_conn_hook_handler' function */

/* Send exit notification to a connection */
static void server_conn_notify( server_conn_desc_t *conn, char nv )
{
	write(RDWN_NOTIFY_WRITE_FD(conn->m_rdwn), &nv, 1);
} /* End of 'server_conn_notify_exit' function */

/* Parse and execute input from client */
bool_t server_conn_parse_input(server_conn_desc_t *d)
{
	int i;
	char *p;
	bool_t res = TRUE;

	/* Extract command */
	for ( i = 0, p = d->m_buf; *p && i < sizeof(d->m_buf); i++, p++ )
	{
		if ((*p) == '\r')
			continue;
		if ((*p) == '\n')
		{
			res = server_conn_exec_command(d);
			str_clear(d->m_cur_cmd);
		}
		else
			str_insert_char(d->m_cur_cmd, *p, d->m_cur_cmd->m_len);
	}
	return res;
} /* End of 'server_conn_parse_input' function */

/* Connection management thread */
static void *server_conn_thread( void *p )
{
	server_conn_desc_t *conn_desc = (server_conn_desc_t *)p;

	for ( ;; )
	{
		/* Wait for some activity */
		int retv = rd_with_notify_wait(conn_desc->m_rdwn);
		if (!retv)
		{
			logger_error(player_log, 0,
					_("Connection select failed: %s"),
					strerror(errno));
			break;
		}

		/* Notification */
		if (retv & RDWN_NOTIFY_READY)
		{
			char nv;
			if (read(RDWN_NOTIFY_READ_FD(conn_desc->m_rdwn), &nv, 1) <= 0)
			{
				/* Some error */
				break;
			}

			/* Exit */
			if (nv == SERVER_NOTIFY_EXIT)
				break;

			/* Handle notification */
			server_conn_client_notify(conn_desc, nv);
		}

		/* Input from client */
		if (retv & RDWN_READ_READY)
		{
			ssize_t sz = recv(conn_desc->m_socket, 
					&conn_desc->m_buf, sizeof(conn_desc->m_buf) - 1, 0);
			if (sz == -1)
				continue;
			else if (sz == 0)
				break;
			conn_desc->m_buf[sz] = 0;

			if (!server_conn_parse_input(conn_desc))
				break;
		}
	}

	logger_message(player_log, 0, _("Closing connection"));

	/* Destroy the connection */
	server_conn_desc_free(conn_desc);

	return NULL;
} /* End of 'server_conn_thread' function */

/* End of 'server.c' file */

