/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for file library functions.
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

#ifndef __SG_MPFC_FILE_H__
#define __SG_MPFC_FILE_H__

#include <stdio.h>
#include "types.h"
#include "logger.h"
#include "mystring.h"

/* Function used to print messages */
typedef void (*file_print_msg_t)( char *msg, ... );

/* File type */
typedef struct tag_file_t
{
	/* File name */
	char *m_name;

	/* File type */
	byte m_type;

	/* Logger object */
	logger_t *m_log;

	/* Additional data */
	void *m_data;
} file_t;

/* File types */
#define FILE_TYPE_REGULAR	0
#define FILE_TYPE_HTTP		1

/* Open a file */
file_t *file_open( const char *filename, char *mode, logger_t *log );

/* Close file */
int file_close( file_t *f );

/* Read from file */
size_t file_read( void *buf, size_t size, size_t nmemb, file_t *f );

/* Write to file */
size_t file_write( void *buf, size_t size, size_t nmemb, file_t *f );

/* Write line to file */
void file_puts( char *s, file_t *f );

/* Get line from file */
char *file_gets( char *s, int size, file_t *f );

/* Get string from file */
str_t *file_get_str( file_t *f );

/* Seek file */
int file_seek( file_t *f, long offset, int whence );

/* Tell file position */
long file_tell( file_t *f );

/* Check for end of file */
bool_t file_eof( file_t *f );

/* Set minimal buffer size */
void file_set_min_buf_size( file_t *f, int size );

/* Get content type */
char *file_get_content_type( file_t *f );

/* Get file type */
byte file_get_type( const char *name );

#endif

/* End of 'file.h' file */

