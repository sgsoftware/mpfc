/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : reg.c
 * PURPOSE     : SG MPFC. File library regular files managament 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.10.2003
 * NOTE        : Module prefix 'freg'.
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
#include "file.h"
#include "reg.h"

/* Get file data */
#define FREG_GET_DATA(data, file) \
	file_reg_data_t *(data) = (file_reg_data_t *)((file)->m_data)

/* Open a file */
file_t *freg_open( file_t *f, char *mode )
{
	file_reg_data_t *data;
	
	/* Allocate memory for additional data */
	f->m_data = malloc(sizeof(*data));
	data = (file_reg_data_t *)(f->m_data);
	memset(data, 0, sizeof(*data));

	/* Try to open file */
	data->m_fd = fopen(f->m_name, mode);
	if (data->m_fd == NULL)
	{
		file_close(f);
		return NULL;
	}
	return f;
} /* End of 'freg_open' function */

/* Close file */
void freg_close( file_t *f )
{
	FREG_GET_DATA(data, f);

	if (data != NULL)
	{
		if (data->m_fd != NULL)
			fclose(data->m_fd);
		free(data);
	}
} /* End of 'freg_close' function */

/* Read from file */
size_t freg_read( void *buf, size_t size, size_t nmemb, file_t *f )
{
	FREG_GET_DATA(data, f);

	if (data != NULL && data->m_fd != NULL)
		return fread(buf, size, nmemb, data->m_fd);
	else
		return 0;
} /* End of 'freg_read' function */

/* Write to file */
size_t freg_write( void *buf, size_t size, size_t nmemb, file_t *f )
{
	FREG_GET_DATA(data, f);

	if (data != NULL && data->m_fd != NULL)
		return fwrite(buf, size, nmemb, data->m_fd);
	else
		return 0;
} /* End of 'freg_write' function */

/* Seek file */
int freg_seek( file_t *f, long offset, int whence )
{
	FREG_GET_DATA(data, f);

	if (data != NULL && data->m_fd != NULL)
		return fseek(data->m_fd, offset, whence);
	else
		return 0;
} /* End of 'freg_seek' function */

/* Tell file position */
long freg_tell( file_t *f )
{
	FREG_GET_DATA(data, f);

	if (data != NULL && data->m_fd != NULL)
		return ftell(data->m_fd);
	else
		return 0;
} /* End of 'freg_tell' function */

/* Get line from file */
char *freg_gets( char *s, int size, file_t *f )
{
	FREG_GET_DATA(data, f);
	return fgets(s, size, data->m_fd);
} /* End of 'freg_gets' function */

/* Check for end of file */
bool_t freg_eof( file_t *f )
{
	FREG_GET_DATA(data, f);
	return feof(data->m_fd);
} /* End of 'freg_eof' function */

/* End of 'reg.c' file */

