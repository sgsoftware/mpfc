/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for logger functions.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "cfg.h"
#include "logger.h"

int logger_get_level( logger_t *log );

static void *logger_stderr_thread( void *arg )
{
	logger_t *log = (logger_t *)arg;

	for ( ;; )
	{
		int retv = rd_with_notify_wait(log->m_stderr_rdwn);
		if (!retv)
			break;

		/* Exit notification */
		if (retv & RDWN_NOTIFY_READY)
			break;

		/* Read from stderr */
		char buf[1024];
		int sz = read(RDWN_FD(log->m_stderr_rdwn), buf, sizeof(buf) - 1);
		if (sz < 0)
			continue;
		buf[sz] = 0;

		/* Add to the log */
		logger_error(log, 0, "%s", buf);
	}

	return NULL;
}

static bool_t logger_redirect_stderr( logger_t *log )
{
	/* Create a pipe */
	if (pipe(log->m_stderr_pipe) < 0)
	{
		log->m_stderr_pipe[0] = -1;
		log->m_stderr_pipe[1] = -1;
		return FALSE;
	}

	/* Replace stderr with the write side of the pipe */
	if (dup2(log->m_stderr_pipe[1], STDERR_FILENO) < 0)
		goto failed;
	log->m_stderr_rdwn = rd_with_notify_new(log->m_stderr_pipe[0]);
	if (!log->m_stderr_rdwn)
		goto failed;

	/* Create thread listening on the read side */
	if (pthread_create(&log->m_stderr_tid, NULL, logger_stderr_thread, log))
		goto failed;

	return TRUE;
failed:
	if (log->m_stderr_rdwn)
	{
		rd_with_notify_free(log->m_stderr_rdwn);
		log->m_stderr_rdwn = NULL;
	}
	if (log->m_stderr_pipe[0] >= 0)
	{
		close(log->m_stderr_pipe[0]);
		log->m_stderr_pipe[0] = -1;
	}
	if (log->m_stderr_pipe[1] >= 0)
	{
		close(log->m_stderr_pipe[1]);
		log->m_stderr_pipe[1] = -1;
	}
	return FALSE;
}

/* Initialize logger */
logger_t *logger_new( cfg_node_t *cfg_list, char *file_name )
{
	logger_t *log;

	/* Allocate memory */
	log = (logger_t *)malloc(sizeof(*log));
	if (log == NULL)
		return NULL;
	memset(log, 0, sizeof(*log));
	log->m_stderr_tid = -1;
	log->m_cfg = cfg_list;
	log->m_level = logger_get_level(log);
	cfg_set_var_handler(log->m_cfg, "log-level", logger_on_change_level, log);

	/* Open file */
	if (file_name != NULL)
		log->m_fd = fopen(file_name, "wt");

	/* Create logger mutex */
	pthread_mutex_init(&log->m_mutex, NULL);

	/* We will redirect stuff written to stderr to the log */
	logger_redirect_stderr(log);

	return log;
} /* End of 'logger_new' function */

/* Free logger */
void logger_free( logger_t *log )
{
	struct logger_message_t *msg, *next;
	struct logger_handler_t *h;

	assert(log);

	/* Close stderr redirection */
	if (log->m_stderr_tid >= 0)
	{
		char msg = 0;
		assert(log->m_stderr_rdwn);
		write(RDWN_NOTIFY_WRITE_FD(log->m_stderr_rdwn), &msg, 1);
		pthread_join(log->m_stderr_tid, NULL);
	}
	if (log->m_stderr_rdwn)
		rd_with_notify_free(log->m_stderr_rdwn);
	if (log->m_stderr_pipe[0] >= 0)
		close(log->m_stderr_pipe[0]);
	if (log->m_stderr_pipe[1] >= 0)
		close(log->m_stderr_pipe[1]);

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

/* Common message adding function */
void logger_add_message( logger_t *log, logger_msg_type_t type, int level,
		char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, type, level, format, ap);
	va_end(ap);
} /* End of 'logger_add_message' function */

/* Version of 'logger_add_message' with vararg list specified */
void logger_add_message_vararg( logger_t *log, logger_msg_type_t type, 
		int level, char *format, va_list ap )
{
	struct logger_message_t *msg;
	struct logger_handler_t *h;
	int n, size = 100;
	char *text;
	va_list ap_orig;

	if (log == NULL)
		return;
	assert(format);

	/* Filter by log level */
	if (level > log->m_level || 
			(type == LOGGER_MSG_DEBUG && log->m_level < 0x100))
		return;

	/* Build message text */
	text = (char *)malloc(size);
	if (text == NULL)
		return;
	va_copy(ap_orig, ap);
	for ( ;; )
	{
		va_copy(ap, ap_orig);
		n = vsnprintf(text, size, format, ap);
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
	log->m_num_messages ++;

	/* Write message to the file */
	if (log->m_fd != NULL)
	{
		fprintf(log->m_fd, "%s%s\n", logger_get_type_prefix(type, level), 
				msg->m_message);
		fflush(log->m_fd);
	}

	/* Call handlers */
	for ( h = log->m_handlers; h != NULL; h = h->m_next )
		(h->m_function)(log, h->m_data, msg);
	logger_unlock(log);
} /* End of 'logger_add_message' function */

/* Add a status message */
void logger_status_msg( logger_t *log, int level, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_STATUS, level, format, ap);
	va_end(ap);
} /* End of 'logger_status_msg' function */

/* Add a normal message */
void logger_message( logger_t *log, int level, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_NORMAL, level, format, ap);
	va_end(ap);
} /* End of 'logger_message' function */

/* Add a warning message */
void logger_warning( logger_t *log, int level, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_WARNING, level, format, ap);
	va_end(ap);
} /* End of 'logger_warning' function */

/* Add an error message */
void logger_error( logger_t *log, int level, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_ERROR, level, format, ap);
	va_end(ap);
} /* End of 'logger_error' function */

/* Add a fatal message message */
void logger_fatal( logger_t *log, int level, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_FATAL, level, format, ap);
	va_end(ap);
} /* End of 'logger_fatal' function */

/* Add a debug message */
void logger_debug( logger_t *log, char *format, ... )
{
	va_list ap;
	va_start(ap, format);
	logger_add_message_vararg(log, LOGGER_MSG_DEBUG, -1, format, ap);
	va_end(ap);
} /* End of 'logger_debug' function */

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
char *logger_get_type_prefix( logger_msg_type_t type, int level )
{
	static char *prefixes[] = { "", "(==) ", "(WW) ", "(EE) ", "(FF) ", 
		"(DD) " };
	if (type < 0 || type >= (sizeof(prefixes) / sizeof(*prefixes)))
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

/* Get logger level from the configuration */
int logger_get_level( logger_t *log )
{
	char *s = cfg_get_var(log->m_cfg, "log-level");
	if (s == NULL)
		return 1;
	else if (!strcmp(s, "none"))
		return -1;
	else if (!strcmp(s, "low"))
		return 0;
	else if (!strcmp(s, "high"))
		return 2;
	else if (!strcmp(s, "debug"))
		return 0x100;
	else 
		return 1;
} /* End of 'logger_get_level' function */

/* Handler for setting log level */
bool_t logger_on_change_level( cfg_node_t *node, char *value, void *data )
{
	logger_t *log = (logger_t *)data;
	log->m_level = logger_get_level(log);
	return TRUE;
} /* End of 'logger_on_change_level' function */

/* End of 'logger.c' file */

