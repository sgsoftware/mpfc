/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Virtual file system functions implementation.
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

#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "types.h"
#include "inp.h"
#include "pmng.h"
#include "vfs.h"
#include "util.h"

/* Check if symbol is a special pattern symbol */
#define VFS_SYM_SPECIAL(ch, space_is_special) \
		((ch) == '*' || (ch) == '?' || (ch) == '[' || (ch) == ']' || \
				(ch) == '~' || ((space_is_special) && (ch) == ' '))

/* Initialize VFS */
vfs_t *vfs_init( pmng_t *pmng )
{
	vfs_t *vfs;

	/* Allocate memory */
	vfs = (vfs_t *)malloc(sizeof(*vfs));
	if (vfs == NULL)
		return NULL;
	memset(vfs, 0, sizeof(*vfs));

	/* Set fields */
	vfs->m_pmng = pmng;
	return vfs;
} /* End of 'vfs_init' function */

/* Free VFS */
void vfs_free( vfs_t *vfs )
{
	assert(vfs);
	free(vfs);
} /* End of 'vfs_free' function */

/* List files matching pattern */
void vfs_glob( vfs_t *vfs, char *pattern, vfs_callback_t callback, void *data,
		int max_level, vfs_glob_flags_t flags )
{
	char *full_pattern;
	in_plugin_t *inp;
	str_t *abs_pattern;
	vfs_glob_list_t *list, *new_list, *matches;
	str_t *prefix;
	int bare_slash = FALSE;
	bool_t not_fixed_vfs = FALSE;

	/* Get plugin */
	full_pattern = pattern;
	inp = vfs_plugin_from_prefix(vfs, pattern, &pattern);
	prefix = str_substring_cptr(full_pattern, 0, pattern - full_pattern - 1);
	
	/* Convert name to absolute */
	abs_pattern = vfs_pattern2absolute(vfs, inp, pattern);
	if (abs_pattern == NULL)
		return;
	while (STR_AT(abs_pattern, STR_LEN(abs_pattern) - 1) == '/' &&
			STR_LEN(abs_pattern) > 1)
		str_delete_char(abs_pattern, STR_LEN(abs_pattern) - 1);
	bare_slash = (STR_LEN(abs_pattern) == 1);
	str_insert_str(abs_pattern, prefix, 0);

	/* If plugin does not have fixed VFS we must use NOPATTERN flag */
	if (VFS_INP_HAS(inp) && (inp_get_flags(inp) & INP_VFS_NOT_FIXED))
	{
		flags |= VFS_GLOB_NOPATTERN;
		not_fixed_vfs = TRUE;
	}

	/* Create list of files matching pattern */
	if (!bare_slash && !not_fixed_vfs &&
			(!(flags & VFS_GLOB_NOPATTERN) || STR_LEN(prefix) > 0) &&
			(file_get_type(pattern) == FILE_TYPE_REGULAR))
	{
		int index1, index2;
		char *s, *ap;

		/* Start from root directory */
		list = vfs_glob_list_new();
		vfs_glob_list_add(list, str_cat_cptr(str_dup(prefix), "/"));
		index1 = STR_LEN(prefix);

		/* Go into sub-directories */
		ap = STR_TO_CPTR(abs_pattern);
		for ( ;; )
		{
			str_t *cur_pattern;
			struct vfs_glob_list_item_t *dir;
			bool_t finish = FALSE;

			/* Skip duplicate slashes */
			while (ap[index1] == '/')
				index1 ++;

			/* Get part of the pattern corresponding to current directory level */
			s = strchr(&ap[index1], '/');
			if (s == NULL)
			{
				index2 = STR_LEN(abs_pattern);
				finish = TRUE;
			}
			else
				index2 = (s - ap);

			cur_pattern = str_substring(abs_pattern, index1, index2 - 1);

			/* Find files matching pattern */
			new_list = vfs_glob_list_new();
			for ( dir = list->m_head; dir != NULL; dir = dir->m_next )
			{
				vfs_dir_t *dh;
				vfs_file_t fd;

				/* Enumerate files in directory */
				vfs_file_desc_init(vfs, &fd, STR_TO_CPTR(dir->m_name), inp);
				dh = vfs_opendir(vfs, &fd);
				if (dh == NULL)
				{
					logger_error(VFS_LOGGER(vfs), 1, 
							_("Unable to read directory %s"), fd.m_full_name);
					continue;
				}
				for ( ;; )
				{
					bool_t matches = FALSE;

					/* Get file name */
					char *name = vfs_readdir(dh);
					if (name == NULL)
						break;

					/* Check if file matches */
					if (!strcmp(name, ".") || !strcmp(name, ".."))
						matches = (!strcmp(STR_TO_CPTR(cur_pattern), name));
					else
						matches = (!fnmatch(STR_TO_CPTR(cur_pattern), 
									name, FNM_PERIOD));
					if (matches)
					{
						vfs_glob_list_add(new_list, 
								vfs_cat_dir_and_name(dir->m_name, name));
					}
				}
				vfs_closedir(dh);
			}

			/* Move to next directory */
			vfs_glob_list_free(list);
			str_free(cur_pattern);
			index1 = index2;
			list = new_list;

			/* Finish */
			if (finish)
			{
				matches = new_list;
				break;
			}
		}
	}
	else
	{
		matches = vfs_glob_list_new();
		vfs_glob_list_add(matches, str_dup(abs_pattern));
	}

	/* Visit the files */
	vfs_glob_list_sort(matches);
	vfs_visit_matches(vfs, inp, matches, callback, data, 0, max_level, flags);

	/* Free memory */
	vfs_glob_list_free(matches);
	str_free(prefix);
	str_free(abs_pattern);
} /* End of 'vfs_glob' function */

/* Visit glob match */
void vfs_visit_match( vfs_t *vfs, vfs_file_t *file, vfs_callback_t callback, 
		void *data, int level, int max_level, vfs_glob_flags_t flags )
{
	struct stat st;
	int ret;

	/* Check recursion level */
	if (level > max_level)
		return;

	/* Get file parameters */
	if (file->m_stat_return != 0)
	{
		logger_error(VFS_LOGGER(vfs), 1,
				_("Unable to stat file %s"), file->m_full_name);
		return;
	}
	st = file->m_stat;
	
	/* Call callback function */
	if (((flags & VFS_GLOB_RETURN_DIRS) || !S_ISDIR(st.st_mode)) && 
			(!(flags & VFS_GLOB_AT_LEVEL_ONLY) || (level == max_level)) && 
			callback != NULL)
	{
		if (flags & VFS_GLOB_OUTPUT_ESCAPED)
		{
			vfs_file_t escaped_file;
			vfs_file_escape(&escaped_file, file, 
					flags & VFS_GLOB_SPACE_IS_SPECIAL);
			callback(&escaped_file, data);
			free(escaped_file.m_full_name);
		}
		else
			callback(file, data);
	}

	/* In case of directory visit its children */
	if ((level < max_level) && S_ISDIR(st.st_mode))
	{
		vfs_dir_t *dh;
		vfs_glob_list_t *list;
		dh = vfs_opendir(vfs, file);
		if (dh == NULL)
		{
			logger_error(VFS_LOGGER(vfs), 1,
					_("Unable to read directory %s"), file->m_full_name);
			return;
		}

		/* Build list of files */
		list = vfs_glob_list_new();
		for ( ;; )
		{
			str_t *dir_name, *full_name;
			char *name = vfs_readdir(dh);
			if (name == NULL)
				break;
			if (!(flags & VFS_GLOB_RETURN_SPEC_LINKS) &&
					(!strcmp(name, ".") || !strcmp(name, "..") ||
					(vfs && cfg_get_var_bool(vfs->m_pmng->m_cfg_list, "skip-hidden-files") && 
					 (*name) == '.')))
				continue;

			dir_name = str_new(file->m_full_name);
			vfs_glob_list_add(list, vfs_cat_dir_and_name(dir_name, name));
			str_free(dir_name);
		}
		vfs_closedir(dh);

		/* Visit children */
		vfs_glob_list_sort(list);
		vfs_visit_matches(vfs, /*file->m_inp*/NULL, list, callback, data, 
				level + 1, max_level, flags);
		vfs_glob_list_free(list);
	}
} /* End of 'vfs_visit_match' function */

/* Visit list of matches */
void vfs_visit_matches( vfs_t *vfs, in_plugin_t *inp, vfs_glob_list_t *list,
		vfs_callback_t callback, void *data, 
		int level, int max_level, vfs_glob_flags_t flags )
{
	struct vfs_glob_list_item_t *file;
	for ( file = list->m_head; file != NULL; file = file->m_next )
	{
		vfs_file_t fd;
		vfs_file_desc_init(vfs, &fd, STR_TO_CPTR(file->m_name), inp);
		vfs_visit_match(vfs, &fd, callback, data, level, max_level, flags);
	}
} /* End of 'vfs_visit_matches' function */

/* Initialize file descriptor structure */
void vfs_file_desc_init( vfs_t *vfs, vfs_file_t *file, char *full_name, 
		in_plugin_t *inp )
{
	memset(file, 0, sizeof(*file));
	file->m_full_name = full_name;
	file->m_extension = util_extension(full_name);
	file->m_short_name = util_short_name(full_name);

	/* We know plugin, so we are sure that plugin prefix is specified */
	if (inp != NULL)
		file->m_name = strchr(full_name, '/') + 2;
	/* Find plugin from prefix */
	else
		inp = vfs_plugin_from_prefix(vfs, full_name, &file->m_name);
	/* If there is no prefix - determine plugin from file name */
	/*
	if (inp == NULL && vfs != NULL)
		inp = pmng_search_format(vfs->m_pmng, file->m_name, file->m_extension);
		*/
	file->m_inp = inp;

	/* Get file parameters */
	file->m_stat_return = vfs_stat(file, &file->m_stat);
} /* End of 'vfs_file_desc_init' function */

/* Escape special symbols in file name */
void vfs_file_escape( vfs_file_t *dest, vfs_file_t *src, 
		bool_t space_is_special )
{
	int num_special = 0;
	int prefix_len, src_len, i, j;
	char *s, *full_name;
	assert(dest);
	assert(src);

	/* Calculate number of special symbols */
	for ( s = src->m_name; *s; s ++ )
		if (VFS_SYM_SPECIAL(*s, space_is_special))
			num_special ++;

	/* Initialize name */
	src_len = strlen(src->m_full_name);
	prefix_len = src->m_name - src->m_full_name;
	full_name = (char *)malloc(src_len + num_special + 1);
	for ( i = 0, j = 0; i <= src_len; i ++ )
	{
		if ((i >= prefix_len) && 
				VFS_SYM_SPECIAL(src->m_full_name[i], space_is_special))
			full_name[j ++] = '\\';
		full_name[j ++] = src->m_full_name[i];
	}

	/* Initialize file descriptor */
	memcpy(dest, src, sizeof(*src));
	dest->m_full_name = full_name;
	dest->m_name = full_name + (src->m_name - src->m_full_name);
	dest->m_short_name = util_short_name(full_name);
	dest->m_extension = util_extension(full_name);
} /* End of 'vfs_file_escape' function */

/* Get input plugin from the plugin prefix. Also return file name without
 * prefix */
in_plugin_t *vfs_plugin_from_prefix( vfs_t *vfs, char *full_name, char **name )
{
	plugin_t *inp;

	if (vfs == NULL || vfs->m_pmng == NULL)
	{
		(*name) = full_name;
		return NULL;
	}

	/* Skip until plugin name */
	(*name) = full_name;
	while (isalnum(**name) || (**name) == '_' || (**name) == '-')
		(*name) ++;

	/* No plugin prefix found */
	if (strncmp(*name, "://", 3))
	{
		(*name) = full_name;
		return NULL;
	}

	/* Search for this plugin */
	(**name) = 0;
	inp = pmng_search_by_name(vfs->m_pmng, full_name, PLUGIN_TYPE_INPUT);
	(**name) = ':';
	if (inp == NULL)
		(*name) = full_name;
	else
		(*name) += 3;
	return INPUT_PLUGIN(inp);
} /* End of 'vfs_plugin_from_prefix' function */

/* Returns absolute path of this pattern */
str_t *vfs_pattern2absolute( vfs_t *vfs, in_plugin_t *inp, char *pattern )
{
	str_t *abs_pattern;

	/* Do things for regular files only */
	if (file_get_type(pattern) != FILE_TYPE_REGULAR)
		return str_new(pattern);

	/* Pattern is not absolute */
	if ((*pattern) != '/')
	{
		/* Insert home directory */
		if ((*pattern) == '~')
		{
			char *user_name, *name_end;
			char was_end;

			/* Extract user name */
			user_name = pattern + 1;
			name_end = user_name;
			while ((*name_end) && (*name_end) != '/')
				name_end ++;
			was_end = *name_end;
			(*name_end) = 0;

			/* Create resulting string */
			abs_pattern = str_new(util_get_home_dir(user_name));
			(*name_end) = was_end;
			str_cat_cptr(abs_pattern, name_end);
		}
		/* Insert current directory */
		else
		{
			char dir[MAX_FILE_NAME];
			abs_pattern = str_new(vfs_getcwd(vfs, inp, dir, sizeof(dir)));
			if (STR_AT(abs_pattern, STR_LEN(abs_pattern) - 1) != '/')
				str_cat_cptr(abs_pattern, "/");
			str_cat_cptr(abs_pattern, pattern);
		}
	}
	else
		abs_pattern = str_new(pattern);
	return abs_pattern;
} /* End of 'vfs_pattern2absolute' function */

/* Open directory */
vfs_dir_t *vfs_opendir( vfs_t *vfs, vfs_file_t *file )
{
	vfs_dir_t *dir;

	/* Initialize directory */
	dir = (vfs_dir_t *)malloc(sizeof(*dir));
	dir->m_vfs = vfs;
	dir->m_inp = file->m_inp;
	if (VFS_INP_HAS(dir->m_inp))
		dir->m_data = inp_vfs_opendir(dir->m_inp, file->m_name);
	else
		dir->m_data = (void *)opendir(file->m_name);
	if (dir->m_data == NULL)
	{
		free(dir);
		return NULL;
	}
	return dir;
} /* End of 'vfs_opendir' function */

/* Close directory */
void vfs_closedir( vfs_dir_t *dir )
{
	assert(dir);
	assert(dir->m_data);
	if (VFS_INP_HAS(dir->m_inp))
		inp_vfs_closedir(dir->m_inp, dir->m_data);
	else
		closedir((DIR *)dir->m_data);
	free(dir);
} /* End of 'vfs_opendir' function */

/* Read next name from directory */
char *vfs_readdir( vfs_dir_t *dir )
{
	assert(dir);
	assert(dir->m_data);
	if (VFS_INP_HAS(dir->m_inp))
		return inp_vfs_readdir(dir->m_inp, dir->m_data);
	else
	{
		struct dirent *de = readdir((DIR *)(dir->m_data));
		if (de == NULL)
			return NULL;
		return de->d_name;
	}
} /* End of 'vfs_readdir' function */

/* Get file parameters */
int vfs_stat( vfs_file_t *file, struct stat *st )
{
	if (file_get_type(file->m_name) != FILE_TYPE_REGULAR)
	{
		memset(st, 0, sizeof(*st));
		st->st_mode = S_IFREG;
		return 0;
	}

	if (VFS_INP_HAS(file->m_inp))
		return inp_vfs_stat(file->m_inp, file->m_name, st);
	else
		return stat(file->m_name, st);
} /* End of 'vfs_stat' function */

/* Get current directory */
char *vfs_getcwd( vfs_t *vfs, in_plugin_t *inp, char *dir, int size )
{
	if (VFS_INP_HAS(inp))
	{
		util_strncpy(dir, "/", size);
		return dir;
	}
	else
		return getcwd(dir, size);
} /* End of 'vfs_getcwd' function */

/* Initialize list */
vfs_glob_list_t *vfs_glob_list_new( void )
{
	vfs_glob_list_t *list;

	/* Allocate memory */
	list = (vfs_glob_list_t *)malloc(sizeof(*list));
	if (list == NULL)
		return NULL;

	/* Set fields */
	list->m_head = list->m_tail = NULL;
	return list;
} /* End of 'vfs_glob_list_new' function */

/* Free list */
void vfs_glob_list_free( vfs_glob_list_t *list )
{
	struct vfs_glob_list_item_t *item, *next;

	assert(list);
	for ( item = list->m_head; item != NULL; )
	{
		next = item->m_next;
		str_free(item->m_name);
		free(item);
		item = next;
	}
	free(list);
} /* End of 'vfs_glob_list_free' function */

/* Add a file to list */
void vfs_glob_list_add( vfs_glob_list_t *list, str_t *name )
{
	struct vfs_glob_list_item_t *item;

	assert(list);
	assert(name);

	/* Create item */
	item = (struct vfs_glob_list_item_t *)malloc(sizeof(*item));
	item->m_name = name;
	item->m_next = NULL;

	/* Insert item */
	if (list->m_tail == NULL)
		list->m_head = list->m_tail = item;
	else
	{
		list->m_tail->m_next = item;
		item->m_prev = list->m_tail;
		list->m_tail = item;
	}
} /* End of 'vfs_glob_list_add' function */

/* Sort list */
void vfs_glob_list_sort( vfs_glob_list_t *list )
{
	struct vfs_glob_list_item_t *cur, *next, *item;
	assert(list);

	if (list->m_head == NULL)
		return;

	/* Sort */
	for ( cur = list->m_head->m_next; cur != NULL; cur = next )
	{
		char *name = STR_TO_CPTR(cur->m_name);
		next = cur->m_next;

		/* Insert into list beginning */
		if (strcasecmp(STR_TO_CPTR(list->m_head->m_name), name) > 0)
		{
			if (cur->m_next != NULL)
				cur->m_next->m_prev = cur->m_prev;
			if (cur->m_prev != NULL)
				cur->m_prev->m_next = cur->m_next;
			cur->m_prev = NULL;
			cur->m_next = list->m_head;
			list->m_head->m_prev = cur;
			list->m_head = cur;
		}
		/* Find place for inserting */
		else
		{
			for ( item = list->m_head->m_next; item != cur;
					item = item->m_next )
			{
				if (strcasecmp(STR_TO_CPTR(item->m_name), name) > 0)
				{
					if (cur->m_next != NULL)
						cur->m_next->m_prev = cur->m_prev;
					if (cur->m_prev != NULL)
						cur->m_prev->m_next = cur->m_next;

					item->m_prev->m_next = cur;
					cur->m_prev = item->m_prev;
					item->m_prev = cur;
					cur->m_next = item;
					break;
				}
			}
		}
	}

	/* Update tail */
	for ( item = list->m_head; item->m_next != NULL; item = item->m_next);
	list->m_tail = item;
} /* End of 'vfs_glob_list_sort' function */

/* Concatenate directory and file name */
str_t *vfs_cat_dir_and_name( str_t *dir, char *name )
{
	str_t *res = str_dup(dir);
	if (STR_AT(res, STR_LEN(res) - 1) != '/')
		str_cat_cptr(res, "/");
	str_cat_cptr(res, name);
	return res;
} /* End of 'vfs_cat_dir_and_name' function */

/* End of 'vfs.c' file */

