/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : util.c
 * PURPOSE     : SG Konsamp. Different utility functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 28.01.2003
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
#include "types.h"
#include "util.h"

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

/* End of 'util.c' file */

