/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for string management functions.
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

#ifndef __SG_MPFC_MYSTRING_H__
#define __SG_MPFC_MYSTRING_H__

#include "types.h"

/* String type */
typedef struct
{
	/* String data */
	char *m_data;

	/* String length in bytes */
	int m_len;

	/* Amount of memory allocated for string and size of allocation portion */
	int m_allocated, m_portion_size;

	/* Multibyte UTF-8 sequence insertion state */
	int m_bytes_to_complete;
	int m_utf8_seq_len;

	/* Cached string width. -1 if not initialized */
	int m_width;
} str_t;

/* Get string length */
#define STR_BYTE_LEN(str) ((str)->m_len)

/* Convert string to (char *) */
#define STR_TO_CPTR(str) ((str)->m_data)
#define STR_AT(str, i) (STR_TO_CPTR(str)[i])

/* Create a new string */
str_t *str_new( const char *s );

/* Duplicate string */
str_t *str_dup( const str_t *s );

/* Free string */
void str_free( str_t *str );
 
/* Clear string */
void str_clear( str_t *str );

/* Copy string from (char *) */
str_t *str_copy_cptr( str_t *dest, const char *src );

/* Copy string */
str_t *str_copy( str_t *dest, const str_t *src );

/* Concatenate string with (char *) */
str_t *str_cat_cptr( str_t *dest, const char *src );

/* Concatenate strings */
str_t *str_cat( str_t *dest, const str_t *src );

/* Insert a character into string 
 * Returns number of inserted screen positions
 * If string becomes temporarily inconsistent (because incomplete utf-8 sequence
 * has been inserted) return value is negative */
int str_insert_char( str_t *str, char ch, int index );

/* Delete a character from string */
bool_t str_delete_char( str_t *str, int index, bool_t before );

/* Replace all characters 'from' to 'to' */
void str_replace_char( str_t *str, char from, char to );

/* Insert a (char *) */
str_t *str_insert_cptr( str_t *dest, const char *src, int index );

/* Insert a string */
str_t *str_insert_str( str_t *dest, const str_t *src, int index );

/* Extract a substring */
str_t *str_substring( const str_t *str, int start, int end );

/* Extract a substring from (char *) */
str_t *str_substring_cptr( const char *str, int start, int end );

/* Formatted print */
int str_printf( str_t *str, const char *fmt, ... );

/* Escape the special symbols (assuming that string is a file name) */
void str_fn_escape_specs( str_t *str, bool_t escape_slashes );

/* Decode multibyte sequence at the given position */
wchar_t str_wchar_at( str_t *str, unsigned pos, int *nbytes );

/* Calculate display width of the string */
int str_calc_width( str_t *str );

/* Get display width of the string */
static inline int str_width( str_t *str )
{
	if (str->m_width >= 0)
		return str->m_width;
	return str_calc_width(str);
}

/* Skip 'delta' displayable positions from byte_pos
 * sym_pos may be updated by a different delta if wide symbols are encountered */
void str_skip_positions( str_t *str, int *byte_pos, int *sym_pos, int delta );

#endif

/* End of 'mystring.h' file */

