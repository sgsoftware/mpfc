/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : iwt.c
 * PURPOSE     : SG MPFC. Info saver thread functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 22.09.2004
 * NOTE        : Module prefix 'iwt'.
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
#include "iwt.h"
#include "player.h"
#include "plist.h"
#include "song.h"

/* Thread ID */
pthread_t iwt_tid = 0;

/* Exit thread flag */
bool_t iwt_exit = FALSE;

/* Songs-to-write queue */
struct iwt_queue_t
{
	song_t *m_song;
	struct iwt_queue_t *m_next;
} *iwt_queue = NULL;

/* Queue mutex */
pthread_mutex_t iwt_mutex;

/* Initialize IWT module */
bool_t iwt_init( void )
{
	/* Initialize queue */
	iwt_queue = NULL;
	pthread_mutex_init(&iwt_mutex, NULL);
	
	/* Initialize thread */
	if (pthread_create(&iwt_tid, NULL, iwt_thread, NULL))
	{
		pthread_mutex_destroy(&iwt_mutex);
		return FALSE;
	}
	return TRUE;
} /* End of 'iwt_init' function */

/* Uninitialize IWT module */
void iwt_free( void )
{
	/* Stop thread */
	if (iwt_tid)
	{
		iwt_exit = TRUE;
		pthread_join(iwt_tid, NULL);
		logger_debug(player_log, "iwt thread terminated");
		iwt_tid = 0;
		iwt_exit = FALSE;
	}

	/* Free queue */
	iwt_lock();
	for ( ; iwt_queue != NULL; )
	{
		struct iwt_queue_t *next = iwt_queue->m_next;
		song_free(iwt_queue->m_song);
		free(iwt_queue);
		iwt_queue = next;
	}
	iwt_unlock();
	pthread_mutex_destroy(&iwt_mutex);
} /* End of 'iwt_free' function */

/* Lock queue */
void iwt_lock( void )
{
	pthread_mutex_lock(&iwt_mutex);
} /* End of 'iwt_lock' function */

/* Unlock queue */
void iwt_unlock( void )
{
	pthread_mutex_unlock(&iwt_mutex);
} /* End of 'iwt_unlock' function */

/* Push song to queue */
void iwt_push( song_t *song )
{
	assert(song);

	/* Lock the queue */
	iwt_lock();
	
	/* Queue is empty */
	if (iwt_queue == NULL)
	{
		iwt_queue = (struct iwt_queue_t *)malloc(sizeof(*iwt_queue));
		iwt_queue->m_song = song_add_ref(song);
		iwt_queue->m_next = NULL;
	}
	else
	{
		struct iwt_queue_t *q, *node;

		/* Do nothing if this song is already in the queue */
		for ( q = iwt_queue; q->m_next != NULL; q = q->m_next )
		{
			if (q->m_song == song)
				break;
		}

		/* Add song to the queue */
		if (q->m_song != song)
		{
			node = (struct iwt_queue_t *)malloc(sizeof(*node));
			node->m_song = song_add_ref(song);
			node->m_next = NULL;
			q->m_next = node;
		}
	}

	/* Unlock the queue */
	iwt_unlock();
} /* End of 'iwt_push' function */

/* Pop song from queue */
song_t *iwt_pop( void )
{
	song_t *s = NULL;

	iwt_lock();
	if (iwt_queue != NULL)
	{
		struct iwt_queue_t *next = iwt_queue->m_next;
		s = iwt_queue->m_song;
		next = iwt_queue->m_next;
		free(iwt_queue);
		iwt_queue = next;
	}
	iwt_unlock();
	return s;
} /* End of 'iwt_pop' function */

/* Info writer thread function */
void *iwt_thread( void *arg )
{
	while (!iwt_exit)
	{
		song_t *song;
		
		/* Pop song */
		if ((song = iwt_pop()) != NULL)
		{
			inp_save_info(song_get_inp(song, NULL), 
					song->m_file_name, song->m_info);
			song_update_info(song);
			song_free(song);
			logger_message(player_log, 1,
				_("Saved info to file %s"), song->m_file_name);
		}

		/* Wait a little */
		util_wait();
	}
} /* End of 'iwt_thread' function */

/* End of 'iwt.c' file */

