/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : sat.c
 * PURPOSE     : SG MPFC. Song adder thread functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 14.08.2003
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
#include "player.h"
#include "plist.h"
#include "sat.h"

/* Thread ID */
pthread_t sat_tid = 0;

/* Exit thread flag */
bool_t sat_exit = FALSE;

/* Initialize SAT module */
bool_t sat_init( void )
{
	/* Initialize thread */
	return !pthread_create(&sat_tid, NULL, sat_thread, NULL);
} /* End of 'sat_init' function */

/* Uninitialize SAT module */
void sat_free( void )
{
	/* Stop thread */
	if (sat_tid)
	{
		sat_exit = TRUE;
		pthread_join(sat_tid, NULL);
		logger_debug(player_log, "sat thread terminated");
		sat_tid = 0;
		sat_exit = FALSE;
	}
} /* End of 'sat_free' function */

/* Push file name to queue */
void sat_push( plist_t *pl, int index )
{
	plist_lock(pl);
	if (index >= 0 && index < pl->m_len && pl->m_list[index] != NULL)
		pl->m_list[index]->m_flags |= SONG_GET_INFO;
	plist_unlock(pl);
} /* End of 'sat_push' function */

/* Pop song from queue */
int sat_pop( plist_t *pl )
{
	int i;
	
	if (pl == NULL)
		return -1;
	for ( i = 0; i < pl->m_len; i ++ )
		if (pl->m_list[i] != NULL && (pl->m_list[i]->m_flags & SONG_GET_INFO))
			return i;
	return -1;
} /* End of 'sat_pop' function */

/* Song adder thread function */
void *sat_thread( void *arg )
{
	while (!sat_exit)
	{
		int i;
		
		/* Pop file name */
		if ((i = sat_pop(player_plist)) >= 0)
		{
			plist_set_song_info(player_plist, i);
		}

		/* Wait a little */
		util_wait();
	}
	return NULL;
} /* End of 'sat_thread' function */

/* End of 'sat.c' file */

