/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : file.c
 * PURPOSE     : SG MPFC. File library functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.02.2004
 * NOTE        : Module prefix 'file'.
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "types.h"
#include "file_http.h"
#include "file_reg.h"

/* Open a file */
file_t *file_open( char *filename, char *mode, file_print_msg_t printer )
{
	file_t *f;
	
	/* Allocate memory for file object */
	f = (file_t *)malloc(sizeof(file_t));
	if (f == NULL)
		return NULL;

	/* Save parameters */
	f->m_name = strdup(filename);
	f->m_print = printer;
	f->m_data = NULL;
	
	/* Get file type from its name and call respective open function */
	f->m_type = file_get_type(filename);
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_open(f, mode);
	case FILE_TYPE_HTTP:
		return fhttp_open(f, mode);
	}
	file_close(f);
	return NULL;
} /* End of 'file_open' function */

/* Close file */
int file_close( file_t *f )
{
	int ret;
	
	if (f != NULL)
	{
		switch (f->m_type)
		{
		case FILE_TYPE_REGULAR:
			ret = freg_close(f);
			break;
		case FILE_TYPE_HTTP:
			ret = fhttp_close(f);
			break;
		default:
			ret = 0;
		}
			
		if (f->m_name != NULL)
			free(f->m_name);
	}
	return ret;
} /* End of 'file_close' function */

/* Read from file */
size_t file_read( void *buf, size_t size, size_t nmemb, file_t *f )
{
	if (f == NULL)
		return 0;

	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_read(buf, size, nmemb, f);
	case FILE_TYPE_HTTP:
		return fhttp_read(buf, size, nmemb, f);
	}
	return 0;
} /* End of 'file_read' function */

/* Write to file */
size_t file_write( void *buf, size_t size, size_t nmemb, file_t *f )
{
	if (f == NULL)
		return 0;

	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_write(buf, size, nmemb, f);
	case FILE_TYPE_HTTP:
		return fhttp_write(buf, size, nmemb, f);
	}
	return 0;
} /* End of 'file_write' function */

/* Seek file */
int file_seek( file_t *f, long offset, int whence )
{
	if (f == NULL)
		return 0;

	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_seek(f, offset, whence);
	case FILE_TYPE_HTTP:
		return fhttp_seek(f, offset, whence);
	}
	return 0;
} /* End of 'file_seek' function */

/* Tell file position */
long file_tell( file_t *f )
{
	if (f == NULL)
		return 0;

	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_tell(f);
	case FILE_TYPE_HTTP:
		return fhttp_tell(f);
	}
	return 0;
} /* End of 'file_tell' function */

/* Get file type */
byte file_get_type( char *name )
{
	if (!strncmp(name, "http://", 7))
		return FILE_TYPE_HTTP;
	return FILE_TYPE_REGULAR;
} /* End of 'file_get_type' function */

/* Print message */
void file_print_msg( file_t *f, char *format, ... )
{
	va_list ap;
	char msg[256];
	
	if (f == NULL || f->m_print == NULL)
		return;

	va_start(ap, format);
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);
	f->m_print(msg);
} /* End of 'file_print_msg' function */

/* Set minimal buffer size */
void file_set_min_buf_size( file_t *f, int size )
{
	if (f == NULL)
		return;
	
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		break;
	case FILE_TYPE_HTTP:
		fhttp_set_min_buf_size(f, size);
		break;
	}
} /* End of 'file_set_min_buf_size' function */

/* Get content type */
char *file_get_content_type( file_t *f )
{
	if (f == NULL)
		return;
	
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return NULL;
	case FILE_TYPE_HTTP:
		return fhttp_get_content_type(f);
	}
} /* End of 'file_get_content_type' function */

/* Get line from file */
char *file_gets( char *s, int size, file_t *f )
{
	if (f == NULL)
		return;
	
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_gets(s, size, f);
	case FILE_TYPE_HTTP:
		return fhttp_gets(s, size, f);
	}
} /* End of 'file_gets' function */

/* Check for end of file */
bool_t file_eof( file_t *f )
{
	if (f == NULL)
		return;
	
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_eof(f);
	case FILE_TYPE_HTTP:
		return fhttp_eof(f);
	}
	return FALSE;
} /* End of 'file_eof' function */

/* Get string from file */
str_t *file_get_str( file_t *f )
{
	if (f == NULL)
		return;
	
	switch (f->m_type)
	{
	case FILE_TYPE_REGULAR:
		return freg_get_str(f);
	case FILE_TYPE_HTTP:
		return fhttp_get_str(f);
	}
	return NULL;
} /* End of 'file_get_str' function */

/* End of 'file.c' file */

