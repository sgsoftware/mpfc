/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : vfs.h
 * PURPOSE     : SG MPFC. Interface for virtual file system
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.09.2004
 * NOTE        : Module prefix 'vfs'.
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

#ifndef __SG_MPFC_VFS_H__
#define __SG_MPFC_VFS_H__

#include <sys/stat.h>
#include "types.h"
#include "inp.h"
#include "logger.h"

/* Virtual file system data type */
typedef struct 
{
	/* Plugins manager */
	struct tag_pmng_t *m_pmng;
} vfs_t;

/* Check that input plugin uses VFS */
#define VFS_INP_HAS(inp)	(inp_get_flags(inp) & INP_VFS)

/* Get logger object */
#define VFS_LOGGER(vfs)		(((vfs) == NULL || ((vfs)->m_pmng == NULL)) ? \
								NULL : ((vfs)->m_pmng->m_log))

/* A file representation */
typedef struct tag_vfs_file_t
{
	/* File name */
	char *m_full_name, *m_name, *m_short_name, *m_extension;

	/* Input plugin for this file (got from plugin prefix) */
	in_plugin_t *m_inp;

	/* File parameters */
	struct stat m_stat;

	/* 'stat' call return code */
	int m_stat_return;
} vfs_file_t;

/* Function called for each of matching file in 'vfs_glob' */
typedef void (*vfs_callback_t)( vfs_file_t *file, void *data );

/* Flags for 'vfs_glob' */
typedef enum
{
	/* Pattern parameter is in fact not pattern but a real file name */
	VFS_GLOB_NOPATTERN = 1 << 1,

	/* Return also directories */
	VFS_GLOB_RETURN_DIRS = 1 << 2,

	/* Return '.' and '..' links */
	VFS_GLOB_RETURN_SPEC_LINKS = 1 << 3,

	/* Visit only files that are placed on the specified level */
	VFS_GLOB_AT_LEVEL_ONLY = 1 << 4,

	/* Output is escaped */
	VFS_GLOB_OUTPUT_ESCAPED = 1 << 5,

	/* Space is a special symbol */
	VFS_GLOB_SPACE_IS_SPECIAL = 1 << 6,
} vfs_glob_flags_t;
#define VFS_LEVEL_INFINITE 0xFFFF

/* List of matching files build in 'vfs_glob' */
typedef struct 
{
	struct vfs_glob_list_item_t
	{
		str_t *m_name;
		struct vfs_glob_list_item_t *m_next, *m_prev;
	} *m_head, *m_tail;
} vfs_glob_list_t;

/* Initialize VFS */
vfs_t *vfs_init( struct tag_pmng_t *pmng );

/* Free VFS */
void vfs_free( vfs_t *vfs );

/* List files matching pattern */
void vfs_glob( vfs_t *vfs, char *pattern, vfs_callback_t callback, void *data,
		int max_level, vfs_glob_flags_t flags );

/* Visit glob match */
void vfs_visit_match( vfs_t *vfs, vfs_file_t *fd, vfs_callback_t callback,
		void *dat, int level, int max_level, vfs_glob_flags_t flags );

/* Visit list of matches */
void vfs_visit_matches( vfs_t *vfs, in_plugin_t *inp, vfs_glob_list_t *list,
		vfs_callback_t callback, void *data, int level, int max_level,
		vfs_glob_flags_t flags );

/* Initialize file descriptor structure */
void vfs_file_desc_init( vfs_t *vfs, vfs_file_t *file, char *full_name,
		in_plugin_t *inp );

/* Escape special symbols in file name */
void vfs_file_escape( vfs_file_t *dest, vfs_file_t *src, 
		bool_t space_is_special );

/* Get input plugin from the plugin prefix. Also return file name without
 * prefix */
in_plugin_t *vfs_plugin_from_prefix( vfs_t *vfs, char *full_name, char **name );

/* Returns absolute path of this pattern */
str_t *vfs_pattern2absolute( vfs_t *vfs, in_plugin_t *inp, char *pattern );

/***
 * Virtual file system access functions
 ***/

/* Directory data */
typedef struct
{
	/* VFS data */
	vfs_t *m_vfs;

	/* Input plugin */
	in_plugin_t *m_inp;

	/* Plugin-specific data */
	void *m_data;
} vfs_dir_t;

/* Open directory */
vfs_dir_t *vfs_opendir( vfs_t *vfs, vfs_file_t *file );

/* Close directory */
void vfs_closedir( vfs_dir_t *dir );

/* Read next name from directory */
char *vfs_readdir( vfs_dir_t *dir );

/* Get file parameters */
int vfs_stat( vfs_file_t *file, struct stat *st );

/* Get current directory */
char *vfs_getcwd( vfs_t *vfs, in_plugin_t *inp, char *dir, int size );

/***
 * VFS glob list management functions 
 ***/

/* Initialize list */
vfs_glob_list_t *vfs_glob_list_new( void );

/* Free list */
void vfs_glob_list_free( vfs_glob_list_t *list );

/* Add a file to list */
void vfs_glob_list_add( vfs_glob_list_t *list, str_t *name );

/* Sort list */
void vfs_glob_list_sort( vfs_glob_list_t *list );

/* Concatenate directory and file name */
str_t *vfs_cat_dir_and_name( str_t *dir, char *name );

#endif

/* End of 'vfs.h' file */

