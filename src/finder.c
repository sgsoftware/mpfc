/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : finder.c
 * PURPOSE     : SG MPFC. File finder functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.10.2003
 * NOTE        : Module prefix 'find'.
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

#include <dirent.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "types.h"
#include "file.h"
#include "finder.h"

/* Main finder function */
int find_do( char *path, char *pattern, int (*find_handler)( char *, void * ),
		void *data )
{
	glob_t gl;
	int i, num = 0;

	/* If file is http - simply add it */
	if (file_get_type(path) == FILE_TYPE_HTTP)
		return find_handler(path, data);
	
	/* Find first-level files in path */
	memset(&gl, 0, sizeof(gl));
	if (glob(path, GLOB_BRACE | GLOB_TILDE, NULL, &gl))
	{
		globfree(&gl);
		return 0;
	}

	/* Add each file */
	for ( i = 0; i < gl.gl_pathc; i ++ )
		num += find_add_file(gl.gl_pathv[i], pattern, find_handler, data);
	globfree(&gl);
	return num;
} /* End of 'find_do' function */

/* Add file */
int find_add_file( char *name, char *pattern, 
		int (*find_handler)( char *, void * ), void *data )
{
	struct stat stat_info;
	
	/* Determine file type (directory or regular) and if it is a directory
	 * call directory adding routine */
	stat(name, &stat_info);
	if (S_ISDIR(stat_info.st_mode))
		return find_add_dir(name, pattern, find_handler, data);
	else if (!S_ISREG(stat_info.st_mode))
		return 0;

	/* Check that file name matches pattern */
	if (!fnmatch(pattern, name, 0))
		return find_handler(name, data);
	return 0;
} /* End of 'find_add_file' function */

/* Add directory */
int find_add_dir( char *filename, char *pattern, 
		int (*find_handler)( char *, void * ), void *data )
{
	struct dirent **de;
	int num = 0, len, n, i;
	
	/* Read directory */
	len = strlen(filename);
	n = scandir(filename, &de, 0, alphasort);
	if (n < 0)
		return 0;
	for ( i = 0; i < n; i ++ )
	{
		struct stat s;
		char *str, *name = de[i]->d_name;

		/* Skip '.' and '..' */
		if (!strcmp(name, ".") || !strcmp(name, ".."))
			continue;

		/* Add file or directory */
		str = (char *)malloc(len + strlen(name) + 2);
		strcpy(str, filename);
		str[len] = '/';
		strcpy(&str[len + 1], name);
		free(de[i]);
		stat(str, &s);
		if (S_ISDIR(s.st_mode))
			num += find_add_dir(str, pattern, find_handler, data);
		else if (S_ISREG(s.st_mode))
			num += find_add_file(str, pattern, find_handler, data);
		free(str);
	}
	free(de);
	return num;
} /* End of 'find_add_dir' function */

/* End of 'finder.c' file */

