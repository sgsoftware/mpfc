/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : logger.c
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "types.h"
#include "cfg.h"
#include "logger.h"

/* Initialize logger */
logger_t *logger_new( cfg_node_t *cfg_list, char *file_name )
{
	logger_t *log;

	/* Allocate memory */
	log = (logger_t *)malloc(sizeof(*log));
	if (log == NULL)
		return NULL;
	memset(log, 0, sizeof(*log));
	log->m_cfg = cfg_list;

	/* Open file */
	if (file_name != NULL)
		log->m_fd = fopen(file_name, "wt");

	/* Create logger mutex */
	pthread_mutex_init(&log->m_mutex, NULL);
	return log;
} /* End of 'logger_new' function */

/* Free logger */
void logger_free( logger_t *log )
{
	struct logger_message_t *msg, *next;
	struct logger_handler_t *h;

	assert(log);

	/* Free mutex */
	pthread_mutex_destroy(&log->m_mutex);

	/* Free messages */
	for ( msg = log->m_head; msg != NULL; msg = next )
	{
		next = msg->m_next;
		free(msg->m_message);
		free(msg);
	}

	/* Free handlers */
	for ( h = log->m_handlers; h != NULL; )
	{
		struct logger_handler_t *next = h->m_next;
		free(h);
		h = next;
	}
	
	/* Close file */
	if (log->m_fd != NULL)
		fclose(log->m_fd);
	free(log);
} /* End of 'logger_free' function */

/* Add message to the log */
void logger_message( logger_t *log, logger_msg_type_t type, int level,
		char *format, ... )
{
	struct logger_message_t *msg;
	struct logger_handler_t *h;
	int n, size = 100;
	va_list ap;
	char *text;

	if (log == NULL)
		return;
	assert(format);

	/* Filter by log level */
//	if (level > cfg_get_var_int(log->m_cfg, "log-level"))
//		return;

	/* Build message text */
	text = (char *)malloc(size);
	if (text == NULL)
		return;
	for ( ;; )
	{
		va_start(ap, format);
		n = vsnprintf(text, size, format, ap);
		va_end(ap);
		if (n > -1 && n < size)
			break;
		else if (n > -1)
			size = n + 1;
		else 
			size *= 2;

		text = (char *)realloc(text, size);
		if (text == NULL)
			return;
	}

	/* Create message */
	msg = (struct logger_message_t *)malloc(sizeof(*msg));
	if (msg == NULL)
	{
		free(text);
		return;
	}
	msg->m_type = type;
	msg->m_level = level;
	msg->m_message = text;
	msg->m_prev = NULL;
	msg->m_next = NULL;

	/* Attach message to the list */
	logger_lock(log);
	if (log->m_tail == NULL)
		log->m_head = log->m_tail = msg;
	else
	{
		log->m_tail->m_next = msg;
		msg->m_prev = log->m_tail;
		log->m_tail = msg;
	}

	/* Write message to the file */
	if (log->m_fd != NULL)
	{
		fprintf(log->m_fd, "%s%s\n", logger_get_type_prefix(type), 
				msg->m_message);
		fflush(log->m_fd);
	}

	/* Call handlers */
	for ( h = log->m_handlers; h != NULL; h = h->m_next )
		(h->m_function)(log, h->m_data, msg);
	logger_unlock(log);
} /* End of 'logger_message' function */

/* Attach a handler function */
void logger_attach_handler( logger_t *log, 
		void (*fn)( logger_t *, void *, struct logger_message_t * ), 
		void *data )
{
	struct logger_handler_t *h;
	assert(log);

	/* Create handler structure */
	h = (struct logger_handler_t *)malloc(sizeof(*h));
	if (h == NULL)
		return;
	h->m_function = fn;
	h->m_data = data;
	h->m_next = NULL;
	
	/* Attach handler */
	logger_lock(log);
	if (log->m_handlers_tail == NULL)
		log->m_handlers = log->m_handlers_tail = h;
	else
	{
		log->m_handlers_tail->m_next = h;
		log->m_handlers_tail = h;
	}
	logger_unlock(log);
} /* End of 'logger_attach_handler' function */

/* Get prefix of messages of some type */
char *logger_get_type_prefix( logger_msg_type_t type )
{
	static char *prefixes[] = { "", "(==) ", "(WW) ", "(EE) ", "(FF) " };
	if (type < 0 || type >= sizeof(prefixes) / sizeof(*prefixes))
		return NULL;
	return prefixes[type];
} /* End of 'logger_get_type_prefix' function */

/* Lock logger */
void logger_lock( logger_t *log )
{
	assert(log);
	pthread_mutex_lock(&log->m_mutex);
} /* End of 'logger_lock' function */

/* Unlock logger */
void logger_unlock( logger_t *log )
{
	assert(log);
	pthread_mutex_unlock(&log->m_mutex);
} /* End of 'logger_unlock' function */

/* End of 'logger.c' file */

