/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : outp.h
 * PURPOSE     : SG MPFC. Interface for output plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2004
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

#ifndef __SG_MPFC_OUTP_H__
#define __SG_MPFC_OUTP_H__

#include "types.h"

struct tag_pmng_t;

/* Output plugin functions list */
typedef struct 
{
	/* Plugin start function */
	bool_t (*m_start)( void );

	/* Plugin end function */
	void (*m_end)( void );

	/* Set channels number function */
	void (*m_set_channels)( int ch );

	/* Set frequency function */
	void (*m_set_freq)( int freq );

	/* Set format function */
	void (*m_set_fmt)( dword fmt );
	
	/* Play stream function */
	void (*m_play)( void *buf, int size );

	/* Flush function */
	void (*m_flush)( void );

	/* Set/get volume */
	void (*m_set_volume)( int left, int right );
	void (*m_get_volume)( int *left, int *right );

	/* Plugins manager */
	struct tag_pmng_t *m_pmng;

	/* Reserved data */
	byte m_reserved[216];
} outp_func_list_t;

/* Output plugin type */
typedef struct tag_out_plugin_t
{
	/* Plugin library handler */
	void *m_lib_handler;

	/* Plugin short name */
	char m_name[256];

	/* Functions list */
	outp_func_list_t m_fl;
} out_plugin_t;

/* Initialize output plugin */
out_plugin_t *outp_init( char *name, struct tag_pmng_t *pmng );

/* Free output plugin object */
void outp_free( out_plugin_t *p );

/* Plugin start function */
bool_t outp_start( out_plugin_t *p );

/* Plugin end function */
void outp_end( out_plugin_t *p );

/* Set channels number function */
void outp_set_channels( out_plugin_t *p, int ch );

/* Set frequency function */
void outp_set_freq( out_plugin_t *p, int freq );

/* Set format function */
void outp_set_fmt( out_plugin_t *p, dword fmt );
	
/* Play stream function */
void outp_play( out_plugin_t *p, void *buf, int size );

/* Flush function */
void outp_flush( out_plugin_t *p );

/* Set volume */
void outp_set_volume( out_plugin_t *p, int left, int right );

/* Get volume */
void outp_get_volume( out_plugin_t *p, int *left, int *right );

#endif

/* End of 'outp.h' file */

