/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for file library http files managament 
 * functions.
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

#ifndef __SG_MPFC_FILE_HTTP_H__
#define __SG_MPFC_FILE_HTTP_H__

#include "types.h"
#include "file.h"
#include "mystring.h"

/* HTTP file additional data */
typedef struct tag_file_http_data_t
{
	/* File and host names */
	char *m_file_name, *m_host_name;

	/* Socket descriptor */
	int m_sock;

	/* Buffer */
	byte *m_buf, *m_actual_ptr;
	int m_real_size, m_buf_size;
	int m_read_size;
	int m_portion_size;
	int m_min_buf_size;

	/* Content type */
	char *m_content_type;

	/* Current file position */
	int m_file_pos;

	/* Thread data */
	pthread_t m_tid;
	pthread_mutex_t m_mutex;
	bool_t m_end_thread;
	bool_t m_finished;
	bool_t m_eof;
} file_http_data_t;

/* Open a file */
file_t *fhttp_open( file_t *f, char *mode );

/* Close file */
int fhttp_close( file_t *f );

/* Read from file */
size_t fhttp_read( void *buf, size_t size, size_t nmemb, file_t *f );

/* Write to file */
size_t fhttp_write( void *buf, size_t size, size_t nmemb, file_t *f );

/* Write a line to file */
void fhttp_puts( char *s, file_t *f );

/* Get line from file */
char *fhttp_gets( char *s, int size, file_t *f );

/* Get string from file */
str_t *fhttp_get_str( file_t *f );

/* Check for end of file */
bool_t fhttp_eof( file_t *f );

/* Seek file */
int fhttp_seek( file_t *f, long offset, int whence );

/* Tell file position */
long fhttp_tell( file_t *f );

/* Set minimal buffer size */
void fhttp_set_min_buf_size( file_t *f, int size );

/* Get content type */
char *fhttp_get_content_type( file_t *f );

/* Thread function */
void *fhttp_thread( void *arg );

/* Check if HTTP header is complete and return its end offset */
int fhttp_header_complete( char *header, int size );

/* Parse URL */
void fhttp_parse_url( char *url, char **host_name, char **file_name, 
		int *port );

/* Get HTTP response return value */
int fhttp_get_return( char *header, int size );

/* Get HTTP header field */
char *fhttp_get_field( char *header, int size, char *field );

#endif

/* End of 'http.h' file */

