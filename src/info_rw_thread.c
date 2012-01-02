/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Song information read/write thread functions 
 * implementation.
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

#include <pthread.h>
#include <stdlib.h>
#include "types.h"
#include "info_rw_thread.h"
#include "player.h"
#include "song.h"
#include "util.h"

/* Thread queue */
irw_queue_t *irw_head, *irw_tail;
pthread_mutex_t irw_mutex;

/* Thread info */
pthread_t irw_tid = 0;
bool_t irw_stop_thread = FALSE;

/* Initialize info read/write thread */
bool_t irw_init( void )
{
	/* Initialize queue */
	irw_head = irw_tail = NULL;
	pthread_mutex_init(&irw_mutex, NULL);

	/* Initialize thread */
	if (pthread_create(&irw_tid, NULL, irw_thread, NULL))
		return FALSE;
	return TRUE;
} /* End of 'irw_init' function */

/* Free thread */
void irw_free( void )
{
	irw_queue_t *q;

	/* Stop thread */
	if (irw_tid)
	{
		irw_stop_thread = TRUE;
		pthread_join(irw_tid, NULL);
		irw_tid = 0;
	}

	/* Free queue */
	pthread_mutex_destroy(&irw_mutex);
	for ( q = irw_head; q != NULL; )
	{
		irw_queue_t *next = q->m_next;
		song_free(q->m_song);
		free(q);
		q = next;
	}
} /* End of 'irw_free' function */

/* Add song to the queue */
void irw_push( song_t *song, song_flags_t flag )
{
	irw_queue_t *node, *q;

	/* Check if this song is not in queue */
	irw_lock();
	for ( q = irw_head; q != NULL; q = q->m_next )
	{
		if (q->m_song == song)
		{
			/* If we want to write info and song in the queue has no this
			 * flag yet, move it to the queue head */
			if (flag & SONG_INFO_WRITE && !(song->m_flags & SONG_INFO_WRITE))
			{
				if (q->m_prev != NULL)
					q->m_prev->m_next = q->m_next;
				if (q->m_next != NULL)
					q->m_next->m_prev = q->m_prev;
				else
					irw_tail = q->m_prev;
				q->m_prev = NULL;
				q->m_next = irw_head;
				irw_head->m_prev = q;
				irw_head = q;
			}
			song->m_flags |= flag;
			irw_unlock();
			return;
		}
	}

	/* Create new queue node */
	node = (irw_queue_t *)malloc(sizeof(*node));
	if (node == NULL)
		return;
	node->m_song = song_add_ref(song);
	node->m_song->m_flags |= flag;

	/* If we want to write info - move node to the head */
	if (flag & SONG_INFO_WRITE)
	{
		node->m_prev = NULL;
		node->m_next = irw_head;
		if (irw_head != NULL)
			irw_head->m_prev = node;
		else
			irw_tail = node;
		irw_head = node;
	}
	/* Else - move node to the tail */
	else
	{
		node->m_next = NULL;
		node->m_prev = irw_tail;
		if (irw_tail != NULL)
			irw_tail->m_next = node;
		else
			irw_head = node;
		irw_tail = node;
	}
	irw_unlock();
} /* End of 'irw_push' function */

/* Get song from the queue */
song_t *irw_pop( void )
{
	song_t *s = NULL;
	irw_queue_t *next;

	irw_lock();
	if (irw_head != NULL)
	{
		s = irw_head->m_song;
		next = irw_head->m_next;
		free(irw_head);
		irw_head = next;
		if (irw_head == NULL)
			irw_tail = NULL;
		else
			irw_head->m_prev = NULL;
	}
	irw_unlock();
	return s;
} /* End of 'irw_pop' function */

/* Thread function */
void *irw_thread( void *arg )
{
	for ( ; !irw_stop_thread; )
	{
		for ( ;; )
		{
			song_flags_t flags;
			song_t *s;

			/* Get next task */
			s = irw_pop();
			if (s == NULL)
				break;
			flags = s->m_flags;

			/* Read song info */
			if (s->m_flags & SONG_INFO_READ)
			{
				song_update_info(s);
				wnd_invalidate(player_wnd);
			}

			/* Write song info */
			if (s->m_flags & SONG_INFO_WRITE)
			{
				song_write_info(s);
			}

			/* Release song reference */
			song_free(s);

			/* If we have not written info - leave cycle to be able to stop
			 * the thread */
			if (!(flags & SONG_INFO_WRITE))
				break;
		}
		util_wait();
	}
	return NULL;
} /* End of 'irw_thread' function */

/* Lock queue */
void irw_lock( void )
{
	pthread_mutex_lock(&irw_mutex);
} /* End of 'irw_lock' function */

/* Unlock queue */
void irw_unlock( void )
{
	pthread_mutex_unlock(&irw_mutex);
} /* End of 'irw_unlock' function */

/* End of 'info_rw_thread.c' file */

