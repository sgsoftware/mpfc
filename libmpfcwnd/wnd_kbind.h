/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_kbind.h
 * PURPOSE     : MPFC Window Library. Interface for key bindings
 *               management functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.09.2004
 * NOTE        : Module prefix 'wnd_kbind'.
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

#ifndef __SG_MPFC_WND_KBIND_H__
#define __SG_MPFC_WND_KBIND_H__

#include "types.h"
#include "wnd_types.h"

/* 'kbind' data */
typedef struct
{
	/* Buffer */
#define WND_KBIND_BUF_SIZE 10
	wnd_key_t m_buf[WND_KBIND_BUF_SIZE];
	int m_buf_ptr;
} wnd_kbind_data_t;

/* Results for wnd_kbind_check_buf function */
#define WND_KBIND_NOT_EXISTING -1
#define WND_KBIND_START -2
#define WND_KBIND_FOUND 0

/* Initialize kbind module */
wnd_kbind_data_t *wnd_kbind_init( wnd_global_data_t *global );

/* Free kbind module */
void wnd_kbind_free( wnd_kbind_data_t *kb );

/* Move key to kbind buffer and send action message if need */
void wnd_kbind_key2buf( wnd_t *wnd, wnd_key_t key );

/* Check if buffer contains complete sequence */
int wnd_kbind_check_buf( wnd_kbind_data_t *kb, wnd_t *wnd, char **action );

/* Check buffer for sequence in a specified configuration list */
int wnd_kbind_check_buf_in_node( wnd_kbind_data_t *kb, wnd_t *wnd, 
		cfg_node_t *node, char **action );

/* Get next key from kbind string value */
wnd_key_t wnd_kbind_value_next_key( char **val );

#endif

/* End of 'wnd_kbind.h' file */

