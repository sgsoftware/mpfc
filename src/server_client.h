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

#ifndef __SG_MPFC_SERVER_CLIENT_H__
#define __SG_MPFC_SERVER_CLIENT_H__

#include <pthread.h>
#include "mystring.h"
#include "rd_with_notify.h"

/* Connection descriptor */
typedef struct tag_server_conn_desc_t
{
	int m_socket;
	pthread_t m_tid;
	rd_with_notify_t *m_rdwn;

	char m_buf[1024];
	int m_buf_pos;
	str_t *m_cur_cmd;

	struct tag_server_conn_desc_t *m_next, *m_prev;
} server_conn_desc_t;

/* Notification codes */
enum
{
	SERVER_NOTIFY_EXIT = 0,
	SERVER_NOTIFY_PLAYLIST,
	SERVER_NOTIFY_STATUS,
};

/* Send a notification to client */
void server_conn_client_notify(server_conn_desc_t *d, char nv);

/* Execute a command received from client */
bool_t server_conn_exec_command(server_conn_desc_t *d);

#endif

/* End of 'server_client.h' file */

