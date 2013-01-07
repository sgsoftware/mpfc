/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for different utility functions.
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

#ifndef __SG_MPFC_UTIL_H__
#define __SG_MPFC_UTIL_H__

#include <stdio.h>
#include "types.h"
#include "mystring.h"

/* Write message to log file */
void util_log( char *format, ... );

/* Search for a substring */
bool_t util_search_str( char *ptext, char *text );

/* Get file extension */
char *util_extension( const char *name );

/* Get file name without full path */
char *util_short_name( const char *name );

/* Delay */
void util_delay( long s, long ns );

/* Wait a little */
void util_wait( void );

/* Convert file name to the one with escaped special symbols */
char *util_escape_fname( char *out, char *in );

/* Get short plugin name */
char *util_get_plugin_short_name( char *dest, char *src );

/* Get user's home directory */
char *util_get_home_dir( char *user );

/* Open a file expanding home directories */
FILE *util_fopen( char *filename, char *flags );

/* Get file size */
int util_get_file_size( char *filename );

/* Wrapper around fgets */
str_t *util_fgets( FILE *fd );

/* Replace characters */
void util_replace_char( char *str, char from, char to );

/* Search for regexp */
bool_t util_search_regexp( char *ptext, char *text, bool_t nocase );

/* Delete new line characters from end of string */
void util_del_nl( char *dest, char *src );

/* Remove multiple slashes in file name */
void util_rem_slashes( char *name );

/* Get file directory name */
void util_get_dir_name( char *dir, const char *filename );

/* A safe string copying (writes null to the end) */
char *util_strncpy( char *dest, char *src, size_t n );

/* Concatenate multiple strings */
char *util_strcat( const char *first, ... );

int mbslen( char *str );

/* Determine file type (regular or directory) resolving symlinks */
bool_t util_file_type(char *name, bool_t *is_dir);

#endif

/* End of 'util.h' file */

