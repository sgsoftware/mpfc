/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : sat.h
 * PURPOSE     : SG MPFC. Interface for song adder thread
 *               functions.
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

#ifndef __SG_MPFC_SAT_H__
#define __SG_MPFC_SAT_H__

#include "types.h"
#include "plist.h"

/* SAT queue type */
typedef struct tag_sat_queue_t
{
	plist_t *m_pl;
	char m_file_name[256];
	char m_title[256];
	int m_len;
	bool m_has_title;
	struct tag_sat_queue_t *m_next;
} sat_queue_t;

/* Initialize SAT module */
bool sat_init( void );

/* Uninitialize SAT module */
void sat_free( void );

/* Push file name to queue */
void sat_push( plist_t *pl, char *filename, char *title, int len );

/* Pop file name from queue */
char *sat_pop( plist_t **pl, char *filename, char *title, int *has_title, 
		int *len );

/* Song adder thread function */
void *sat_thread( void *arg );

#endif

/* End of 'sat.h' file */

