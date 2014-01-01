/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. String management functions implementation.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "types.h"
#include "mystring.h"
#include "util.h"

/* String portion size */
#define STR_PORTION_SIZE 64

/* Some private functions */
static void str_allocate( str_t *str, int new_len );
static int str_move_back( str_t *str, int pos );

/* Create a new string */
str_t *str_new( const char *s )
{
	str_t *str;
	int len;

	if (s == NULL)
		return NULL;

	/* Allocate memory */
	str = (str_t *)malloc(sizeof(str_t));
	if (str == NULL)
		return NULL;

	/* Initialize fields */
	str->m_len = strlen(s);
	str->m_data = NULL;
	str->m_allocated = 0;
	str->m_portion_size = STR_PORTION_SIZE;
	str->m_bytes_to_complete = 0;
	str->m_utf8_seq_len = 0;
	str->m_width = ((*s) == 0) ? 0 : -1;
	str_allocate(str, str->m_len);
	strcpy(str->m_data, s);
	return str;
} /* End of 'str_new' function */

/* Duplicate string */
str_t *str_dup( const str_t *s )
{
	return str_new(s->m_data);
} /* End of 'str_dup' function */

/* Free string */
void str_free( str_t *str )
{
	if (str == NULL)
		return;

	if (str->m_data != NULL)
		free(str->m_data);
	free(str);
} /* End of 'str_free' function */
 
/* Clear string */
void str_clear( str_t *str )
{
	if (str == NULL)
		return;

	*str->m_data = 0;
	str->m_len = 0;
	str->m_width = -1;
	str->m_bytes_to_complete = 0;
	str->m_utf8_seq_len = 0;
} /* End of 'str_clear' function */

/* Copy string from (char *) */
str_t *str_copy_cptr( str_t *dest, const char *src )
{
	int len;
	
	if (dest == NULL || src == NULL)
		return NULL;

	str_allocate(dest, dest->m_len = strlen(src));
	strcpy(dest->m_data, src);
	dest->m_width = -1;
	return dest;
} /* End of 'str_copy_cptr' function */

/* Copy string */
str_t *str_copy( str_t *dest, const str_t *src )
{
	str_t *new_str = str_copy_cptr(dest, src->m_data);
	new_str->m_width = src->m_width;
	return new_str;
} /* End of 'str_copy' function */

/* Concatenate string with (char *) */
str_t *str_cat_cptr( str_t *dest, const char *src )
{
	int len;

	if (dest == NULL || src == NULL)
		return NULL;

	len = strlen(src);
	str_allocate(dest, dest->m_len + len);
	dest->m_len += len;
	dest->m_width = -1;
	strcat(dest->m_data, src);
	return dest;
} /* End of 'str_cat_cptr' function */

/* Concatenate strings */
str_t *str_cat( str_t *dest, const str_t *src )
{
	int was_width = dest->m_width;
	str_t *new_str = str_cat_cptr(dest, src->m_data);
	if (was_width >= 0 && src->m_width >= 0)
		new_str->m_width = was_width + src->m_width;
	return new_str;
} /* End of 'str_cat' function */

/* Is the string in consistent state? */
static inline bool_t str_is_consistent( str_t *str )
{
	return str->m_bytes_to_complete == 0;
} /* End of 'str_is_consistent' function */

/* Insert a character to string */
int str_insert_char( str_t *str, char ch, int index )
{
	index += str->m_utf8_seq_len;
	if (str == NULL || index < 0 || index > str->m_len)
		return 0;

	str_allocate(str, str->m_len + 1);
	memmove(&str->m_data[index + 1], &str->m_data[index],
			str->m_len - index + 1);
	str->m_data[index] = ch;
	str->m_len ++;

	/* Easy case: normal ASCII char */
	if (utf8_is_ascii_byte(ch))
	{
		if (str->m_width >= 0)
			++str->m_width;
		return 1;
	}
	else
	{
		/* Analyze utf8 completeness */
		if (str_is_consistent(str))
		{
			/* If we receive a non-ascii byte, enter the incomplete state */
			str->m_bytes_to_complete = utf8_decode_num_bytes(ch) - 1;
		}
		else
		{
			--str->m_bytes_to_complete;
		}
		++str->m_utf8_seq_len;

		if (str_is_consistent(str))
		{
			str->m_utf8_seq_len = 0;

			wchar_t c = str_wchar_at(str, str_move_back(str, index + 1), NULL);
			int w = wcwidth(c);
			if (w < 0)
				w = 0;

			if (str->m_width >= 0)
				str->m_width += w;

			return w;
		}

		/* In inconsistent state */
		return -1;
	}
} /* End of 'str_insert_char' function */

/* Delete a character from string */
bool_t str_delete_char( str_t *str, int index, bool_t before )
{
	/* Find byte positions to delete */
	int bp = index;
	int sp = 0; /* dummy */
	str_skip_positions(str, &bp, &sp, before ? -1 : 1);
	if (bp == index)
		return FALSE;
	if (bp < index)
	{
		int t = bp;
		bp = index;
		index = t;
	}

	memmove(&str->m_data[index], &str->m_data[bp],
			str->m_len - bp + 1);

	if (str->m_width >= 0)
		--str->m_width;

	int new_len = str->m_len - (bp - index);
	str_allocate(str, new_len);
	str->m_len = new_len;
	return TRUE;
} /* End of 'str_delete_char' function */

/* Replace all characters 'from' to 'to' */
void str_replace_char( str_t *str, char from, char to )
{
	int i;

	if (str == NULL || from == to)
		return;

	for ( i = 0; i < str->m_len; i ++ )
		if (str->m_data[i] == from)
			str->m_data[i] = to;
} /* End of 'str_replace_char' function */

/* Insert a (char *) */
str_t *str_insert_cptr( str_t *dest, const char *src, int index )
{
	int len;
	
	if (dest == NULL || src == NULL || index < 0 || index > dest->m_len)
		return NULL;

	len = strlen(src);
	str_allocate(dest, dest->m_len + len);
	memmove(&dest->m_data[index + len], &dest->m_data[index],
			dest->m_len - index + 1);
	memcpy(&dest->m_data[index], src, len);
	dest->m_len += len;
	dest->m_width = -1;
	return dest;
} /* End of 'str_insert_cptr' function */

/* Insert a string */
str_t *str_insert_str( str_t *dest, const str_t *src, int index )
{
	int was_width = dest->m_width;
	str_t *new_str = str_insert_cptr(dest, src->m_data, index);
	if (was_width >= 0 && src->m_width >= 0)
		new_str->m_width = was_width + src->m_width;
	return new_str;
} /* End of 'str_insert_str' function */

/* Formatted print */
int str_printf( str_t *str, const char *fmt, ... )
{
	int n, size = 100;
	va_list ap;

	if (str == NULL)
		return 0;

	str->m_width = -1;
	str_allocate(str, size);
	for ( ;; )
	{
		va_start(ap, fmt);
		n = vsnprintf(str->m_data, size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size)
			return (str->m_len = n);
		else if (n > -1)
			size = n + 1;
		else 
			size *= 2;

		str_allocate(str, size);
	}
	return 0;
} /* End of 'str_printf' function */

/* Allocate space for string data */
static void str_allocate( str_t *str, int new_len )
{
	for ( str->m_allocated = new_len + 1; 
			str->m_allocated % str->m_portion_size; str->m_allocated ++ );
	str->m_data = (char *)realloc(str->m_data, str->m_allocated);
} /* End of 'str_allocate' function */

/* Extract a substring */
str_t *str_substring( const str_t *str, int start, int end )
{
	str_t *new_str;

	if (str == NULL)
		return NULL;
	if (end < start)
		return str_new("");

	/* Allocate memory */
	new_str = (str_t *)malloc(sizeof(str_t));
	if (new_str == NULL)
		return NULL;

	/* Initialize fields */
	new_str->m_len = end - start + 1;
	new_str->m_data = NULL;
	new_str->m_allocated = 0;
	new_str->m_portion_size = STR_PORTION_SIZE;
	new_str->m_bytes_to_complete = 0;
	new_str->m_utf8_seq_len = 0;
	new_str->m_width = -1;
	str_allocate(new_str, new_str->m_len);
	memcpy(new_str->m_data, &str->m_data[start], new_str->m_len);
	new_str->m_data[new_str->m_len] = 0;
	return new_str;
} /* End of 'str_substring' function */

/* Extract a substring from (char *) */
str_t *str_substring_cptr( const char *str, int start, int end )
{
	str_t *new_str;

	if (str == NULL)
		return NULL;
	if (end < start)
		return str_new("");

	/* Allocate memory */
	new_str = (str_t *)malloc(sizeof(str_t));
	if (new_str == NULL)
		return NULL;

	/* Initialize fields */
	new_str->m_len = end - start + 1;
	new_str->m_data = NULL;
	new_str->m_allocated = 0;
	new_str->m_portion_size = STR_PORTION_SIZE;
	new_str->m_bytes_to_complete = 0;
	new_str->m_utf8_seq_len = 0;
	new_str->m_width = -1;
	str_allocate(new_str, new_str->m_len);
	memcpy(new_str->m_data, &str[start], new_str->m_len);
	new_str->m_data[new_str->m_len] = 0;
	return new_str;
} /* End of 'str_substring_cptr' function */

/* Escape the special symbols (assuming that string is a file name) */
void str_fn_escape_specs( str_t *str, bool_t escape_slashes )
{
	int i;

	assert(str);

	for ( i = 0; i < str->m_len; i ++ )
	{
		char ch = str->m_data[i];
		if (ch == ' ' || ch == '*' || ch == '[' || ch == ']' ||
				ch == '\'' || ch == '\"' || ch == '!' ||
				(escape_slashes && ch == '/') || ch == '\\')
			str_insert_char(str, '\\', i ++);
	}
} /* End of 'str_fn_escape_specs' function */

/* Decode multibyte sequence at the given position */
wchar_t str_wchar_at( str_t *str, unsigned pos, int *nbytes )
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	size_t n = str->m_len - pos;
	wchar_t ch;
	n = mbrtowc(&ch, str->m_data + pos, n, &mbstate);
	if (n >= (size_t)(-2))
	{
		ch = L'?';
		n = 1;
	}
	else if (n == 0)
	{
		ch = 0;
		n = 1;
	}

	if (nbytes)
		(*nbytes) = n;
	return ch;
} /* End of 'str_wchar_at' function */

/* Move to start of a previous char */
static int str_move_back( str_t *str, int pos )
{
	while (pos > 0)
	{
		--pos;
		if (utf8_is_start_byte(str->m_data[pos]))
			break;
	}
	return pos;
} /* End of 'str_move_back' function */

/* Skip 'delta' displayable positions from byte_pos
 * sym_pos may be updated by a different delta if wide symbols are encountered */
void str_skip_positions( str_t *str, int *byte_pos, int *sym_pos, int delta )
{
	if (!str)
		return;

	int bp = *byte_pos;
	int sp = *sym_pos;

	/* Scan forward */
	if (delta >= 0)
	{
		while (delta > 0 && bp < str->m_len)
		{
			int nbytes;
			wchar_t c = str_wchar_at(str, bp, &nbytes);
			int w = wcwidth(c);
			if (w < 0)
				w = 0;

			delta -= w;
			sp += w;
			bp += nbytes;
		} 

		/* Skip remaining zero-width chars */
		while (bp < str->m_len)
		{
			int nbytes;
			wchar_t c = str_wchar_at(str, bp, &nbytes);
			int w = wcwidth(c);
			if (w > 0)
				break;

			bp += nbytes;
		}

		(*byte_pos) = bp;
		(*sym_pos) = sp;
	}
	/* Scan backwards */
	else if (delta < 0)
	{
		while (bp > 0 && delta < 0)
		{
			bp = str_move_back(str, bp);

			int nbytes;
			wchar_t c = str_wchar_at(str, bp, &nbytes);
			int w = wcwidth(c);
			if (w < 0)
				w = 0;

			delta += w;
			sp -= w;
		}

		(*byte_pos) = bp;
		(*sym_pos) = sp;
	}
} /* End of 'str_skip_positions' function*/

/* Calculate display width of the string */
int str_calc_width( str_t *str )
{
	return (str->m_width = utf8_width(str->m_data));
} /* End of 'str_calc_width' function */

/* End of 'string.c' file */

