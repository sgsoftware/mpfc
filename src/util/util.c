/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : util.c
 * PURPOSE     : SG MPFC. Various utility functions 
 *               implementation.
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "util.h"

/* Check that file name symbol is special */
#define UTIL_FNAME_IS_SPECIAL(sym) \
	((sym) == ' ' || (sym) == '\t' || (sym) == '\'' || (sym) == '\"' || \
	 (sym) == '(' || (sym) == ')' || (sym) == ';' || \
	 (sym) == '!' || (sym) == '&' || (sym) == '\\')

/* Write message to log file */
void util_log( char *format, ... )
{
	FILE *fd;
	va_list ap;
	
	/* Try to open file */
	fd = fopen("log", "at");
	if (fd == NULL)
		return;

	/* Write message */
	va_start(ap, format);
	vfprintf(fd, format, ap);
	va_end(ap);

	/* Close file */
	fclose(fd);
} /* End of 'util_log' function */

/* Search for a substring */
bool util_search_str( char *ptext, char *text )
{
	int t_len = strlen(text), p_len = strlen(ptext), 
		i = p_len - 1;
	bool found = FALSE;

	/* Search for a substring using a Boyer-Moore algorithm */
	while (i < t_len && !found)
	{
		int j, k;
		
		for ( j = i, k = p_len - 1; k >= 0 && text[j] == ptext[k];
		   		j --, k -- );
		
		if (k < 0)
		{
			found = TRUE;
			break;
		}
		else
		{
			for ( ; k >= 0 && ptext[k] != text[j]; k -- );
			i += (p_len - k - 1);
		}
	}

	return found;
} /* End of 'util_search_str' function */

/* Get file extension */
char *util_get_ext( char *name )
{
	int i;

	for ( i = strlen(name) - 1; i >= 0 && name[i] != '.'; i -- );
	return &name[i + 1];
} /* End of 'util_get_ext' function */

/* Delay */
void util_delay( long s, long ns )
{
	struct timespec tv;

	tv.tv_sec = 0;
	tv.tv_nsec = ns;
	nanosleep(&tv, NULL);
} /* End of 'util_delay' function */

/* Get file name without full path */
char *util_get_file_short_name( char *name )
{
	int i;

	for ( i = strlen(name) - 1; i >= 0 && name[i] != '/'; i -- );
	return (i >= 0) ? &name[i + 1] : name;
} /* End of 'util_get_file_short_name' function */

/* Convert file name to the one with escaped special symbols */
char *util_escape_fname( char *out, char *in )
{
	int i, j, len;
	char in_name[256];
	
	len = strlen(in);
	strcpy(in_name, in);
	for ( i = j = 0; i <= len; i ++ )
	{
		if (UTIL_FNAME_IS_SPECIAL(in_name[i]))
			out[j ++] = '\\';
		out[j ++] = in_name[i];
	}
} /* End of 'util_escape_fname' function */

/* Open a file expanding home directories */
FILE *util_fopen( char *filename, char *flags )
{
	char fname[256];
	
	if (filename[0] == '~' && filename[1] == '/')
		sprintf(fname, "%s/%s", getenv("HOME"), &filename[2]);
	else
		strcpy(fname, filename);
	return fopen(fname, flags);
} /* End of 'util_fopen' function */

/* Get short plugin name */
char *util_get_plugin_short_name( char *dest, char *src )
{
	int i, j;
	
	for ( i = strlen(src) - 1; i >= 0 && src[i] != '.'; i -- );
	if (i <= 0)
	{
		strcpy(dest, src);
		return dest;
	}
	for ( j = i - 1; j >= 0 && src[j] != '/'; j -- );
		if (j < 0)
		{
			strcpy(dest, src);
			return dest;
		}
	memcpy(dest, &src[j + 4], i - j - 4);
	dest[i - j - 4] = 0;
	return dest;
} /* End of 'util_get_plugin_short_name' function */

/* Get file size */
int util_get_file_size( char *filename )
{
	FILE *fd;
	int s;
	
	fd = util_fopen(filename, "rb");
	if (fd == NULL)
		return 0;
	fseek(fd, 0, SEEK_END);
	s = ftell(fd);
	fclose(fd);
	return s;
} /* End of 'util_get_file_size' function */

/* End of 'util.c' file */

