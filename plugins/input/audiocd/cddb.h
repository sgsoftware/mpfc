/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cddb.h
 * PURPOSE     : SG MPFC AudioCD input plugin. Interface for CDDB
 *               support functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.09.2003
 * NOTE        : Module prefix 'cddb'.
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

#ifndef __SG_MPFC_AUDIOCD_CDDB_H__
#define __SG_MPFC_AUDIOCD_CDDB_H__

#include "types.h"
#include "song_info.h"

/* Initialize CDDB entry for currently playing disc */
bool cddb_read( void );

/* Free stuff */
void cddb_free( void );

/* Fill song info for specified track */
bool cddb_get_trk_info( int track, song_info_t *info );

/* Save track info */
void cddb_save_trk_info( int track, song_info_t *info );

/* Search for CDDB entry on local machine */
bool cddb_read_local( dword id );

/* Search for CDDB entry on server */
bool cddb_read_server( dword id );

/* Calculate disc ID */
dword cddb_id( void );

/* Find sum for disc ID */
int cddb_sum( int n );

/* Send data to CDDB server */
bool cddb_server_send( int fd, char *buf, int size );

/* Receice data from CDDB server */
bool cddb_server_recv( int fd, char *buf, int size );

/* Convert data received from server to our format */
void cddb_server2data( char *buf );

/* Save CDDB data */
void cddb_save_data( dword id );

/* Add a string to data */
void cddb_data_add( char *str, int index );

#endif

/* End of 'cddb.h' file */

