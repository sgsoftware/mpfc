/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : util.h
 * PURPOSE     : SG MPFC. Interface for different utility
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.09.2003
 * NOTE        : Module prefix 'util'.
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

/* Write message to log file */
void util_log( char *format, ... );

/* Search for a substring */
bool util_search_str( char *ptext, char *text );

/* Get file extension */
char *util_get_ext( char *name );

/* Delay */
void util_delay( long s, long ns );

/* Get file name without full path */
char *util_get_file_short_name( char *name );

/* Convert file name to the one with escaped special symbols */
char *util_escape_fname( char *out, char *in );

/* Get short plugin name */
char *util_get_plugin_short_name( char *dest, char *src );

/* Open a file expanding home directories */
FILE *util_fopen( char *filename, char *flags );

/* Get file size */
int util_get_file_size( char *filename );

#endif

/* End of 'util.h' file */

