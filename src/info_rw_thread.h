/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : info_rw_thread.h
 * PURPOSE     : SG MPFC. Interface for song information read/write
 *               thread functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.11.2004
 * NOTE        : Module prefix 'irw'.
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

#ifndef __SG_MPFC_INFO_RW_THREAD_H__
#define __SG_MPFC_INFO_RW_THREAD_H__

#include "types.h"
#include "song.h"

/* Songs queue */
typedef struct tag_irw_queue_t 
{
	/* The song */
	song_t *m_song;

	/* Next and previous songs in the queue */
	struct tag_irw_queue_t *m_next, *m_prev;
} irw_queue_t;

/* Initialize info read/write thread */
bool_t irw_init( void );

/* Free thread */
void irw_free( void );

/* Add song to the queue */
void irw_push( song_t *song, song_flags_t flag );

/* Get song from the queue */
song_t *irw_pop( void );

/* Thread function */
void *irw_thread( void *arg );

/* Lock queue */
void irw_lock( void );

/* Unlock queue */
void irw_unlock( void );

#endif

/* End of 'info_rw_thread.h' file */

