/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : outp.h
 * PURPOSE     : SG MPFC. Interface for output plugin management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 19.09.2004
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
#include "plugin.h"

struct tag_pmng_t;

/* Data for exchange */
typedef struct 
{
	/* Common plugin data */
	plugin_data_t m_pd;

	/* Plugin start function */
	bool_t (*m_start)( void );

	/* Plugin end function */
	void (*m_end)( void );

	/* Play stream function */
	void (*m_play)( void *buf, int size );

	/* Set channels number function */
	void (*m_set_channels)( int ch );

	/* Set frequency function */
	void (*m_set_freq)( int freq );

	/* Set format function */
	void (*m_set_fmt)( dword fmt );
	
	/* Set/get volume */
	void (*m_set_volume)( int left, int right );
	void (*m_get_volume)( int *left, int *right );

	/* Flush function */
	void (*m_flush)( void );

	/* Pause playing */
	void (*m_pause)( void );

	/* Resume playint */
	void (*m_resume)( void );

	/* Reserved data */
	byte m_reserved[80];

	/* Flags */
	dword m_flags;

	/* Reserved data */
	byte m_reserved1[60];
} outp_data_t;

/* Output plugin type */
typedef struct tag_out_plugin_t
{
	/* Plugin object */
	plugin_t m_plugin;

	/* Data for exchange */
	outp_data_t m_pd;
} out_plugin_t;

/* Helper macros */
#define OUTPUT_PLUGIN(p) ((out_plugin_t *)p)
#define OUTP_DATA(pd) ((outp_data_t *)pd)

/* Output plugin flags */
#define OUTP_NO_SOUND 0x00000001

/* Initialize output plugin */
plugin_t *outp_init( char *name, struct tag_pmng_t *pmng );

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

/* Pause playing */
void outp_pause( out_plugin_t *p );

/* Resume playing */
void outp_resume( out_plugin_t *p );

/* Set volume */
void outp_set_volume( out_plugin_t *p, int left, int right );

/* Get volume */
void outp_get_volume( out_plugin_t *p, int *left, int *right );

/* Get plugin flags */
dword outp_get_flags( out_plugin_t *p );

#endif

/* End of 'outp.h' file */

