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
#include "types.h"
#include "mystring.h"

/* String portion size */
#define STR_PORTION_SIZE 64

/* Some private functions */
static void str_allocate( str_t *str, int new_len );

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
} /* End of 'str_clear' function */

/* Copy string from (char *) */
str_t *str_copy_cptr( str_t *dest, const char *src )
{
	int len;
	
	if (dest == NULL || src == NULL)
		return NULL;

	str_allocate(dest, dest->m_len = strlen(src));
	strcpy(dest->m_data, src);
	return dest;
} /* End of 'str_copy_cptr' function */

/* Copy string */
str_t *str_copy( str_t *dest, const str_t *src )
{
	return str_copy_cptr(dest, src->m_data);
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
	strcat(dest->m_data, src);
	return dest;
} /* End of 'str_cat_cptr' function */

/* Concatenate strings */
str_t *str_cat( str_t *dest, const str_t *src )
{
	return str_cat_cptr(dest, src->m_data);
} /* End of 'str_cat' function */

/* Insert a character to string */
void str_insert_char( str_t *str, char ch, int index )
{
	if (str == NULL || index < 0 || index > str->m_len)
		return;

	str_allocate(str, str->m_len + 1);
	memmove(&str->m_data[index + 1], &str->m_data[index],
			str->m_len - index + 1);
	str->m_data[index] = ch;
	str->m_len ++;
} /* End of 'str_insert_char' function */

/* Delete a character from string */
char str_delete_char( str_t *str, int index )
{
	char ch;
	
	if (str == NULL || index < 0 || index >= str->m_len)
		return 0;

	ch = str->m_data[index];
	memmove(&str->m_data[index], &str->m_data[index + 1],
			str->m_len - index);
	str_allocate(str, str->m_len - 1);
	str->m_len --;
	return ch;
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
	return dest;
} /* End of 'str_insert_cptr' function */

/* Insert a string */
str_t *str_insert_str( str_t *dest, const str_t *src, int index )
{
	return str_insert_cptr(dest, src->m_data, index);
} /* End of 'str_insert_str' function */

/* Formatted print */
int str_printf( str_t *str, const char *fmt, ... )
{
	int n, size = 100;
	va_list ap;

	if (str == NULL)
		return 0;

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

/* End of 'string.c' file */

