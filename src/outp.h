/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : outp.h
 * PURPOSE     : SG Konsamp. Interface for output plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
 * NOTE        : Module prefix 'outp'.
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

#ifndef __SG_KONSAMP_OUTP_H__
#define __SG_KONSAMP_OUTP_H__

#include "types.h"

/* Output plugin functions list */
typedef struct 
{
	/* Plugin start function */
	bool (*m_start)( void );

	/* Plugin end function */
	void (*m_end)( void );

	/* Set channels number function */
	void (*m_set_channels)( int ch );

	/* Set frequency function */
	void (*m_set_freq)( int freq );

	/* Set format function */
	void (*m_set_bits)( int bits );
	
	/* Play stream function */
	void (*m_play)( void *buf, int size );
} outp_func_list_t;

/* Output plugin type */
typedef struct tag_out_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Functions list */
	outp_func_list_t m_fl;
} out_plugin_t;

/* Initialize output plugin */
out_plugin_t *outp_init( char *name );

/* Free output plugin object */
void outp_free( out_plugin_t *p );

#endif

/* End of 'outp.h' file */

