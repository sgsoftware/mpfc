/******************************************************************
 * Copyright (C) 2003 - 2012 by SG Software.
 *
 * SG MPFC. Filesystem utility functions.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "file_utils.h"

static bool_t fu_file_type_recc( char *name, bool_t *is_dir, int rec_level )
{
	struct stat st;
	if (stat(name, &st))
		return FALSE;

	if (S_ISREG(st.st_mode))
	{
		(*is_dir) = FALSE;
		return TRUE;
	}
	else if (S_ISDIR(st.st_mode))
	{
		(*is_dir) = TRUE;
		return TRUE;
	}
	else if (S_ISLNK(st.st_mode))
	{
		/* Cyclic link? */
		if (rec_level > 10)
			return FALSE;

		char linked_name[MAX_FILE_NAME];
		if (readlink(name, linked_name, sizeof(linked_name)) < 0)
			return FALSE;
		return fu_file_type_recc(linked_name, is_dir, rec_level + 1);
	}

	/* Special file which we are not interested in */
	return FALSE;
} 

/* Determine file type (regular or directory) resolving symlinks */
bool_t fu_file_type(char *name, bool_t *is_dir)
{
	return fu_file_type_recc(name, is_dir, 0);
}

/* Open a directory */
fu_dir_t *fu_opendir(char *name)
{
	fu_dir_t *dir = (fu_dir_t *)malloc(sizeof(fu_dir_t));

	if (!(dir->m_dir = opendir(name)))
	{
		free(dir);
		return NULL;
	}

	/* Allocate dirent */
	if (!(dir->m_dirent = (struct dirent *)malloc(offsetof(struct dirent, d_name) +
			pathconf(name, _PC_NAME_MAX) + 1)))
	{
		closedir(dir->m_dir);
		free(dir);
		return NULL;
	}

	return dir;
}

/* Read directory entry */
struct dirent *fu_readdir(fu_dir_t *dir)
{
	struct dirent *de_result;
	if (readdir_r(dir->m_dir, dir->m_dirent, &de_result))
		return NULL;
	if (!de_result)
		return NULL;
	return de_result;
}

/* Close directory */
void fu_closedir(fu_dir_t *dir)
{
	if (!dir)
		return;
	closedir(dir->m_dir);
	free(dir->m_dirent);
	free(dir);
}

/* Is this a '.' or '..' ? */
bool_t fu_is_special_dir(char *name)
{
	return (name[0] == '.' &&
			(name[1] == 0 || 
			 (name[1] == '.' && name[2] == 0)));
}

/* Does path have a prefix? */
bool_t fu_is_prefixed(char *name)
{
	const char *p = strchr(name, '/');
	return (p != name && *(p - 1) == ':' && *(p + 1) == '/');
}

/* End of 'file_utils.h' file */


