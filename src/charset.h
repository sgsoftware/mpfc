/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for libcharset functions.
 * $Id$
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

#ifndef __SG_MPFC_CHARSET_H__
#define __SG_MPFC_CHARSET_H__

#include "types.h"
#include "pmng.h"

/* Charsets that are supported internally */
#define CS_1_BYTE 0
#define CS_UTF8 1
#define CS_UNKNOWN -1

/* Charset information structure */
typedef struct 
{
	/* Charset ID */
	int m_id;

	/* Plugin that handles this charset and charset index in this plugin */
	cs_plugin_t *m_csp;
	int m_index;
} cs_info_t;

/* Output string helper type */
typedef struct
{
	char *m_str;
	int m_len;
	int m_allocated;
} cs_output_t;

/* Convert string between charsets */
char *cs_convert( char *str, char *cs_in, char *cs_out, pmng_t *pmng );

/* Get charset info */
void cs_get_info( char *name, cs_info_t *info, pmng_t *pmng );

/* Get next character from string */
dword cs_get_next_ch( char **str, cs_info_t *info );

/* Convert character code to unicode */
dword cs_to_unicode( dword ch, cs_info_t *info );

/* Convert unicode to character code */
dword cs_from_unicode( dword unicode, cs_info_t *info );

/* Reallocate memory for output string */
void cs_reallocate( cs_output_t *str, int new_len );

/* Append character to output string */
void cs_append2out( cs_output_t *str, char ch );

/* Add a unicode character to output string */
void cs_unicode2str( cs_output_t *str, dword unicode, cs_info_t *info );

/* Get number of bytes occupied by symbol in UTF-8 charset (looking
 * at its first byte) */
int cs_utf8_get_num_bytes( byte first );

#endif

/* End of 'charset.h' file */

