/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : reg.h
 * PURPOSE     : SG MPFC. Interface for file library regular
 *               files managament functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 25.10.2003
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

#ifndef __SG_MPFC_FILE_REG_H__
#define __SG_MPFC_FILE_REG_H__

#include "types.h"
#include "file.h"

/* Regular file additional data */
typedef struct tag_file_reg_data_t
{
	/* File descriptor */
	FILE *m_fd;
} file_reg_data_t;

/* Open a file */
file_t *freg_open( file_t *f, char *mode );

/* Close file */
int freg_close( file_t *f );

/* Read from file */
size_t freg_read( void *buf, size_t size, size_t nmemb, file_t *f );

/* Write to file */
size_t freg_write( void *buf, size_t size, size_t nmemb, file_t *f );

/* Get line from file */
char *freg_gets( char *s, int size, file_t *f );

/* Check for end of file */
bool_t freg_eof( file_t *f );

/* Seek file */
int freg_seek( file_t *f, long offset, int whence );

/* Tell file position */
long freg_tell( file_t *f );

#endif

/* End of 'reg.h' file */

