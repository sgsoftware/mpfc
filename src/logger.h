/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : logger.h
 * PURPOSE     : SG MPFC. Interface for logger functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 21.09.2004
 * NOTE        : Module prefix 'logger'.
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

#ifndef __SG_MPFC_LOGGER_H__
#define __SG_MPFC_LOGGER_H__

#include <pthread.h>
#include <stdio.h>
#include "types.h"
#include "cfg.h"

/* Log message types */
typedef enum
{
	LOGGER_MSG_STATUS = 0,
	LOGGER_MSG_NORMAL,
	LOGGER_MSG_WARNING,
	LOGGER_MSG_ERROR,
	LOGGER_MSG_FATAL,
} logger_msg_type_t;

/* Log level */
enum
{
	LOGGER_LEVEL_NONE = 0,
	LOGGER_LEVEL_LOW,
	LOGGER_LEVEL_DEFAULT,
	LOGGER_LEVEL_HIGH,
};

/* Logger data type */
typedef struct tag_logger_t
{
	/* The messages list */
	struct logger_message_t
	{
		/* Message type */
		logger_msg_type_t m_type;

		/* Log level */
		int m_level;

		/* Message text */
		char *m_message;

		/* Links to the previous and next messages */
		struct logger_message_t *m_next, *m_prev;
	} *m_head, *m_tail;

	/* Number of messages */
	int m_num_messages;

	/* Configuration list */
	cfg_node_t *m_cfg;

	/* Mutex */
	pthread_mutex_t m_mutex;

	/* Log file name descriptor */
	FILE *m_fd;

	/* Handlers list */
	struct logger_handler_t
	{
		/* Function and pointer to spcific data passed to it */
		void *m_data;
		void (*m_function)( struct tag_logger_t *log, void *data, 
				struct logger_message_t *msg );

		/* Next handler */
		struct logger_handler_t *m_next;
	} *m_handlers, *m_handlers_tail;
} logger_t;

/* Initialize logger */
logger_t *logger_new( cfg_node_t *cfg_list, char *file_name );

/* Free logger */
void logger_free( logger_t *log );

/* Add message to the log */
void logger_message( logger_t *log, logger_msg_type_t type, int level,
		char *format, ... );

/* Attach a handler function */
void logger_attach_handler( logger_t *log, 
		void (*fn)( logger_t *, void *, struct logger_message_t * ), 
		void *data );

/* Get prefix of messages of some type */
char *logger_get_type_prefix( logger_msg_type_t type );

/* Lock logger */
void logger_lock( logger_t *log );

/* Unlock logger */
void logger_unlock( logger_t *log );

#endif

/* End of 'logger.h' file */

