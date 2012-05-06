/******************************************************************
 * Copyright (C) 2003 - 2012 by SG Software.
 *
 * SG MPFC. Interface for filesystem utility functions.
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

#ifndef __SG_MPFC_FILE_UTILS_H__
#define __SG_MPFC_FILE_UTILS_H__

#include <sys/types.h>
#include <dirent.h>
#include "types.h"

/* Determine file type (regular or directory) resolving symlinks */
bool_t fu_file_type(char *name, bool_t *is_dir);

typedef struct
{
	DIR *m_dir;
	struct dirent *m_dirent;
} fu_dir_t;

/* Open a directory */
fu_dir_t *fu_opendir(char *name);

/* Read directory entry */
struct dirent *fu_readdir(fu_dir_t *dir);

/* Close directory */
void fu_closedir(fu_dir_t *dir);

/* Is this a '.' or '..' ? */
bool_t fu_is_special_dir(char *name);

/* Does path have a prefix? */
bool_t fu_is_prefixed(char *name);

#endif

/* End of 'file_utils.h' file */

