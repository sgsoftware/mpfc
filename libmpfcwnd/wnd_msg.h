/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_msg.h
 * PURPOSE     : MPFC Window Library. Interface for message 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.08.2004
 * NOTE        : Module prefix 'wnd_msg'.
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

#ifndef __SG_MPFC_WND_MSG_H__
#define __SG_MPFC_WND_MSG_H__

#include <pthread.h>
#include "types.h"
#include "wnd_types.h"

/* Message data type */
struct tag_wnd_msg_data_t
{
	/* The message data */
	void *m_data;

	/* Destructor for freeing data type specific fields */
	void (*m_destructor)( void *data );
};

/* Message type */
struct tag_wnd_msg_t 
{
	/* Message target window */
	wnd_t *m_wnd;
	
	/* Message name */
	char *m_name;

	/* Message data */
	wnd_msg_data_t m_data;
};

/* Message handler return code */
enum tag_wnd_msg_retcode_t
{
	WND_MSG_RETCODE_OK = 0,
	WND_MSG_RETCODE_STOP,
	WND_MSG_RETCODE_EXIT,
};

/* Message handler type.
 * Message handler is actually a chain of functions. So we implement
 * it as linked list */
struct tag_wnd_msg_handler_t
{
	/* Pointer to handler function (should be casted to proper type) */
	void *m_func;

	/* Pointer to the next handler */
	struct tag_wnd_msg_handler_t *m_next;
};

/* Message callback function type */
typedef wnd_msg_retcode_t (*wnd_msg_callback_t)( struct tag_wnd_t *wnd,
		wnd_msg_handler_t *h, wnd_msg_data_t *data );

/* Message queue type */
typedef struct tag_wnd_msg_queue_t
{
	/* Queue head and tail */
	struct wnd_msg_queue_item_t 
	{
		/* Message parameters */
		wnd_msg_t m_msg;

		/* Next and previous messages in queue */
		struct wnd_msg_queue_item_t *m_next, *m_prev;
	} *m_base, *m_last;

	/* Queue mutex */
	pthread_mutex_t m_mutex;
} wnd_msg_queue_t;

/* Initialize message queue */
wnd_msg_queue_t *wnd_msg_queue_init( void );

/* Get a message from queue */
bool_t wnd_msg_get( wnd_msg_queue_t *queue, wnd_msg_t *msg );

/* Send a message */
void wnd_msg_send( struct tag_wnd_t *wnd, char *name, wnd_msg_data_t data );

/* Remove a given item from the queue */
void wnd_msg_rem( wnd_msg_queue_t *queue, struct wnd_msg_queue_item_t *item );

/* Lock message queue */
void wnd_msg_lock_queue( wnd_msg_queue_t *queue );

/* Unlock message queue */
void wnd_msg_unlock_queue( wnd_msg_queue_t *queue );

/* Free message queue */
void wnd_msg_queue_free( wnd_msg_queue_t *queue );

/* Remove messages with specified target window */
void wnd_msg_queue_remove_by_window( wnd_msg_queue_t *queue, 
		struct tag_wnd_t *wnd, bool with_descendants );

/* Free message data */
void wnd_msg_free( wnd_msg_t *msg );

/* Add a handler to the handlers chain */
void wnd_msg_add_handler( wnd_t *wnd, char *msg_name, void *handler );

/* Add a handler to the handlers chain end */
void wnd_msg_add_handler_to_end( wnd_t *wnd, char *msg_name, void *handler );

/* Remove handler from the handlers chain beginning */
void wnd_msg_rem_handler( wnd_t *wnd, char *msg_name );

/* Free handlers chain */
void wnd_msg_free_handlers( wnd_msg_handler_t *h );

#endif

/* End of 'wnd_msg.h' file */

