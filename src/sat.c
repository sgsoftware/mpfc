/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : sat.c
 * PURPOSE     : SG MPFC. Song adder thread functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.05.2003
 * NOTE        : Module prefix 'sat'.
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

#include <pthread.h>
#include <stdlib.h>
#include "types.h"
#include "plist.h"
#include "sat.h"

/* Thread ID */
pthread_t sat_tid = 0;

/* Exit thread flag */
bool sat_exit = FALSE;

/* Mutex used for synchronization of push/pop operations */
pthread_mutex_t sat_mutex;

/* Our queue */
sat_queue_t *sat_queue = NULL, *sat_queue_tail = NULL;

/* Initialize SAT module */
bool sat_init( void )
{
	/* Initialize mutex */
	pthread_mutex_init(&sat_mutex, NULL);

	/* Initialize thread */
	pthread_create(&sat_tid, NULL, sat_thread, NULL);
} /* End of 'sat_init' function */

/* Uninitialize SAT module */
void sat_free( void )
{
	/* Stop thread */
	if (sat_tid)
	{
		sat_exit = TRUE;
		pthread_join(sat_tid, NULL);
		sat_tid = 0;
		sat_exit = FALSE;
	}

	/* Destroy mutex */
	pthread_mutex_destroy(&sat_mutex);

	/* Destroy queue */
	if (sat_queue != NULL)
	{
		sat_queue_t *t, *t1;

		t = sat_queue;
		while (t != NULL)
		{
			t1 = t->m_next;
			free(t);
			t = t1;
		}
		sat_queue = NULL;
		sat_queue_tail = NULL;
	}
} /* End of 'sat_free' function */

/* Push file name to queue */
void sat_push( plist_t *pl, char *filename, char *title, int len )
{
	sat_queue_t *t;

	/* Get access to queue */
	pthread_mutex_lock(&sat_mutex);

	/* Push file name */
	if (sat_queue_tail == NULL)
		t = sat_queue = sat_queue_tail = (sat_queue_t *)malloc(
				sizeof(sat_queue_t));
	else
		t = sat_queue_tail->m_next = (sat_queue_t *)malloc(sizeof(sat_queue_t));
	t->m_pl = pl;
	strcpy(t->m_file_name, filename);
	if (t->m_has_title = (title != NULL))
		strcpy(t->m_title, title);
	t->m_len = len;
	t->m_next = NULL;
	sat_queue_tail = t;

	/* Release queue */
	pthread_mutex_unlock(&sat_mutex);
} /* End of 'sat_push' function */

/* Pop file name from queue */
char *sat_pop( plist_t **pl, char *filename, char *title, 
		int *has_title, int *len )
{
	sat_queue_t *t;
	
	/* Get access to queue */
	pthread_mutex_lock(&sat_mutex);

	/* Pop file name */
	if (sat_queue == NULL)
	{
		pthread_mutex_unlock(&sat_mutex);
		return NULL;
	}

	t = sat_queue;
	*pl = t->m_pl;
	strcpy(filename, t->m_file_name);
	if (*has_title = t->m_has_title)
		strcpy(title, t->m_title);
	*len = t->m_len;
	t = sat_queue->m_next;
	free(sat_queue);
	sat_queue = t;
	if (t == NULL)
		sat_queue_tail = NULL;

	/* Release queue */
	pthread_mutex_unlock(&sat_mutex);
	return filename;
} /* End of 'sat_pop' function */

/* Song adder thread function */
void *sat_thread( void *arg )
{
	while (!sat_exit)
	{
		plist_t *pl;
		char filename[256];
		char title[256];
		int len;
		bool has_title;
		
		/* Pop file name */
		if (sat_pop(&pl, filename, title, &has_title, &len) != NULL)
		{
			__plist_add_song(pl, filename, has_title ? title : NULL, len);
		}

		/* Wait a little */
		util_delay(0, 100000L);
	}
} /* End of 'sat_thread' function */

/* End of 'sat.c' file */
