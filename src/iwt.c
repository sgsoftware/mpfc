/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : iwt.c
 * PURPOSE     : SG MPFC. Info saver thread functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 18.12.2003
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

/* Initialize IWT module */
bool_t iwt_init( void )
{
	/* Initialize thread */
	pthread_create(&iwt_tid, NULL, iwt_thread, NULL);
} /* End of 'iwt_init' function */

/* Uninitialize IWT module */
void iwt_free( void )
{
	/* Stop thread */
	if (iwt_tid)
	{
		iwt_exit = TRUE;
		pthread_join(iwt_tid, NULL);
		iwt_tid = 0;
		iwt_exit = FALSE;
	}
} /* End of 'iwt_free' function */

/* Push song to queue */
void iwt_push( song_t *song )
{
	if (song != NULL)
		song->m_flags |= SONG_SAVE_INFO;
} /* End of 'iwt_push' function */

/* Pop song from queue */
song_t *iwt_pop( void )
{
	int i;
	plist_t *pl = player_plist;
	
	if (pl == NULL)
		return NULL;
	for ( i = 0; i < pl->m_len; i ++ )
		if (pl->m_list[i] != NULL && (pl->m_list[i]->m_flags & SONG_SAVE_INFO))
			return pl->m_list[i];
	return NULL;
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
			player_print_msg(_("Saving info to file %s"), song->m_file_name);
			inp_save_info(song_get_inp(song), song->m_file_name, song->m_info);
			song->m_flags &= (~SONG_SAVE_INFO);
			plist_lock(player_plist);
			song_update_info(song);
			plist_unlock(player_plist);
			player_print_msg(_("Saved"));
		}

		/* Wait a little */
		util_wait();
	}
} /* End of 'iwt_thread' function */

/* End of 'iwt.c' file */

