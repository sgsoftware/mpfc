/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : buf.c
 * PURPOSE     : SG MPFC Utility Library. Buffered input functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.08.2003
 * NOTE        : Module prefix 'util_buf'.
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
#include <stdlib.h>
#include "types.h"
#include "util.h"

/* Open file for buffered input */
util_buf_t *util_buf_open( char *filename, int buf_size )
{
	util_buf_t *buf;

	/* Allocate memory */
	buf = (util_buf_t *)malloc(sizeof(util_buf_t));
	if (buf == NULL)
		return NULL;
	if (buf_size)
	{
		buf->m_buf = (byte *)malloc(buf_size);
		if (buf->m_buf == NULL)
		{
			free(buf);
			return NULL;
		}
	}
	else
		buf->m_buf = NULL;

	/* Open file */
	buf->m_fd = fopen(filename, "rb");
	if (buf->m_fd == NULL)
	{
		if (buf->m_buf != NULL)
			free(buf->m_buf);
		free(buf);
		return NULL;
	}

	/* Set fields */
	buf->m_size = buf_size;
	buf->m_real_size = buf_size;
	buf->m_pos = buf_size;
	buf->m_offset = 0;
	fseek(buf->m_fd, 0, SEEK_END);
	buf->m_file_size = ftell(buf->m_fd);
	fseek(buf->m_fd, 0, SEEK_SET);
	return buf;
} /* End of 'util_buf_open' function */

/* Close file */
void util_buf_close( util_buf_t *buf )
{
	if (buf == NULL)
		return;

	if (buf->m_fd != NULL)
		fclose(buf->m_fd);
	if (buf->m_buf != NULL)
		free(buf->m_buf);
	free(buf);
} /* End of 'util_buf_close' function */

/* Buffered read */
int util_buf_read( void *ptr, int size, util_buf_t *buf )
{
	int rem, s;
	
	if (buf == NULL || ptr == NULL)
		return 0;

	if (buf->m_buf == NULL)
	{
		return fread(ptr, 1, size, buf->m_fd);
	}
		
	rem = buf->m_real_size - buf->m_pos;
	if (rem < size)
	{
		memmove(buf->m_buf, &buf->m_buf[buf->m_pos], rem);
		s = fread(&buf->m_buf[rem], 1, buf->m_real_size - rem, buf->m_fd);
		buf->m_real_size = s + rem;
		buf->m_pos = 0;
		buf->m_offset += s;
	}
	if (buf->m_pos + size > buf->m_real_size)
		size = buf->m_real_size - buf->m_pos;
	memcpy(ptr, &buf->m_buf[buf->m_pos], size);
	buf->m_pos += size;
	return size;
} /* End of 'util_buf_read' function */

/* Seek file opened for buffered input */
void util_buf_seek( util_buf_t *buf, int offset, int whence )
{
	int pos = 0;
	
	if (buf == NULL)
		return;

	if (buf->m_buf == NULL)
	{
		fseek(buf->m_fd, offset, whence);
		return;
	}

	switch (whence)
	{
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_END:
		pos = buf->m_file_size - offset;
		break;
	case SEEK_CUR:
		pos = buf->m_offset + buf->m_pos + offset;
		break;
	}
	if (pos >= buf->m_offset && pos < buf->m_offset + buf->m_real_size)
		buf->m_pos = pos - buf->m_offset;
	else
	{
		fseek(buf->m_fd, pos, SEEK_SET);
		buf->m_pos = buf->m_real_size;
	}
} /* End of 'util_buf_seek' function */

/* Tell file position */
int util_buf_tell( util_buf_t *buf )
{
	if (buf == NULL)
		return 0;
	return (buf->m_buf == NULL) ? ftell(buf->m_fd) : 
		buf->m_offset + buf->m_pos;
} /* End of 'util_buf_tell' function */

/* End of 'buf.c' file */

