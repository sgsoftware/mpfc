/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : iwt.h
 * PURPOSE     : SG MPFC. Interface for info saver thread
 *               functions.
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

#ifndef __SG_MPFC_IWT_H__
#define __SG_MPFC_IWT_H__

#include "types.h"
#include "song.h"

/* Initialize IWT module */
bool_t iwt_init( void );

/* Uninitialize IWT module */
void iwt_free( void );

/* Push song to queue */
void iwt_push( song_t *song );

/* Pop song from queue */
song_t *iwt_pop( void );

/* Info writer thread function */
void *iwt_thread( void *arg );

#endif

/* End of 'iwt.h' file */

