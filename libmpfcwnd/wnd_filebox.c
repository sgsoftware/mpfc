/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_filebox.c
 * PURPOSE     : MPFC Window Library. File box functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 18.08.2004
 * NOTE        : Module prefix 'filebox'.
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
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "types.h"
#include "mystring.h"
#include "wnd.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"
#include "wnd_hbox.h"

/* Create a new file box */
filebox_t *filebox_new( wnd_t *parent, char *id, char *text, char letter,
		int width )
{
	filebox_t *fb;
	wnd_class_t *klass;

	/* Allocate memory */
	fb = (filebox_t *)malloc(sizeof(*fb));
	if (fb == NULL)
		return NULL;
	memset(fb, 0, sizeof(*fb));

	/* Initialize class */
	klass = editbox_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
	{
		free(fb);
		return NULL;
	}
	WND_OBJ(fb)->m_class = klass;

	/* Initialize file box */
	if (!filebox_construct(fb, parent, id, text, letter, width))
	{
		free(fb);
		return NULL;
	}
	WND_FLAGS(fb) |= WND_FLAG_INITIALIZED;
	return fb;
} /* End of 'filebox_new' function */

/* File box constructor */
bool_t filebox_construct( filebox_t *fb, wnd_t *parent, char *id, char *text, 
		char letter, int width )
{
	wnd_t *wnd = WND_OBJ(fb);

	/* Initialize edit box part */
	if (!editbox_construct(EDITBOX_OBJ(fb), parent, id, text, letter, width))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(wnd, "keydown", filebox_on_keydown);
	wnd_msg_add_handler(wnd, "destructor", filebox_destructor);
	return TRUE;
} /* End of 'filebox_construct' function */

/* Create an edit box with label */
filebox_t *filebox_new_with_label( wnd_t *parent, char *title, char *id,
		char *text, char letter, int width )
{
	hbox_t *hbox;
	hbox = hbox_new(parent, NULL, 0);
	label_new(WND_OBJ(hbox), title, "", 0);
	return filebox_new(WND_OBJ(hbox), id, text, letter, width);
} /* End of 'filebox_new_with_label' function */

/* Destructor */
void filebox_destructor( wnd_t *wnd )
{
	filebox_free_names(FILEBOX_OBJ(wnd));
} /* End of 'filebox_destructor' function */

/* 'keydown' message handler */
wnd_msg_retcode_t filebox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	filebox_t *fb = FILEBOX_OBJ(wnd);

	/* Handle only TAB */
	if (key != KEY_TAB)
	{
		filebox_free_names(fb);
		return WND_MSG_RETCODE_OK;
	}

	/* Insert the next name */
	filebox_insert_next(fb);
	wnd_msg_send(wnd, "changed", editbox_changed_new());
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_STOP;
} /* End of 'filebox_on_keydown' function */

/* Load names list */
void filebox_load_names( filebox_t *fb )
{
	str_t *text = EDITBOX_OBJ(fb)->m_text, *dirname = NULL, *pattern = NULL;
	int cursor = EDITBOX_OBJ(fb)->m_cursor;
	DIR *dir = NULL;
	struct dirent *de;
	struct filebox_name_t *item, *last = NULL;
	int plen, i;

	assert(fb);

	/* Get directory name */
	for ( i = cursor - 1; i >= 0; i -- )
	{
		if (STR_AT(text, i) == '/')
			break;
	}
	dirname = str_substring(text, 0, i);
	if (dirname == NULL)
		goto failed;

	/* Build pattern */
	plen = cursor - i - 1;
	pattern = str_substring(text, i + 1, cursor - 1);
	if (pattern == NULL)
		goto failed;

	if (STR_AT(dirname, 0) == '~')
	{
		char *home_dir = getenv("HOME");
		str_delete_char(dirname, 0);
		str_insert_cptr(dirname, home_dir, 0);
	}

	/* Read directory contents */
	dir = opendir(STR_TO_CPTR(dirname));
	if (dir == NULL)
		goto failed;
	for ( ;; )
	{
		/* Read new name */
		de = readdir(dir);
		if (de == NULL)
			break;

		/* Match pattern */
		if (de->d_name[0] != '.' && !strncmp(de->d_name, 
					STR_TO_CPTR(pattern), plen))
		{
			struct stat s;
			str_t *full_name, *name;
			int name_len = strlen(de->d_name);
			bool_t isdir;

			/* Is this file a directory? */
			full_name = str_dup(dirname);
			str_cat_cptr(full_name, de->d_name);
			stat(STR_TO_CPTR(full_name), &s);
			isdir = S_ISDIR(s.st_mode);
			str_free(full_name);

			/* Add slash in case of directory */
			name = str_new(&de->d_name[plen]);
			if (isdir)
				str_cat_cptr(name, "/");

			/* Create names list item */
			item = (struct filebox_name_t *)malloc(sizeof(*item));
			item->m_name = strdup(STR_TO_CPTR(name));
			str_free(name);

			/* It's the first name */
			if (fb->m_names == NULL)
			{
				item->m_next = item;
				item->m_prev = item;
				fb->m_names = item;
			}
			/* Insert in the beginning */
			else if (strcmp(item->m_name, fb->m_names->m_name) <= 0)
			{
				item->m_next = fb->m_names;
				item->m_prev = fb->m_names->m_prev;
				fb->m_names->m_prev->m_next = item;
				fb->m_names->m_prev = item;
				fb->m_names = item;
			}
			/* Insert in any other position */
			else
			{
				/* Find proper position */
				struct filebox_name_t *i;
				for ( i = fb->m_names->m_next;; )
				{
					if (strcmp(item->m_name, i->m_name) < 0)
						break;
					i = i->m_next;
					if (i == fb->m_names)
						break;
				}

				/* Set links in item */
				item->m_prev = i->m_prev;
				item->m_next = i;
				i->m_prev->m_next = item;
				i->m_prev = item;
			}
		}
	} 
	closedir(dir);
	str_free(dirname);
	str_free(pattern);
	fb->m_names_valid = TRUE;
	fb->m_insert_start = EDITBOX_OBJ(fb)->m_cursor;
	return;

failed:
	if (pattern != NULL)
		str_free(pattern);
	if (dirname != NULL)
		str_free(dirname);
	if (dir != NULL)
		closedir(dir);
} /* End of 'filebox_load_names' function */

/* Insert next entry in the file names */
void filebox_insert_next( filebox_t *fb )
{
	assert(fb);

	/* Check if names list is valid */
	if (!fb->m_names_valid || 
			(fb->m_names != NULL && fb->m_names->m_next == fb->m_names))
	{
		filebox_free_names(fb);
		filebox_load_names(fb);
	}

	/* Insert current name */
	if (fb->m_names != NULL)
	{
		char *ch;
		int i, cursor;
		editbox_t *eb = EDITBOX_OBJ(fb);

		/* Delete current insertion first */
		cursor = eb->m_cursor;
		for ( i = fb->m_insert_start; i < cursor; i ++ )
			editbox_delch(eb, fb->m_insert_start);

		/* Insert this name */
		for ( ch = fb->m_names->m_name; (*ch) != 0; ch ++ )
			editbox_addch(eb, *ch);
		fb->m_names = fb->m_names->m_next;
	}
} /* End of 'filebox_insert_next' function */

/* Free names list */
void filebox_free_names( filebox_t *fb )
{
	struct filebox_name_t *item, *next;

	assert(fb);
	fb->m_names_valid = FALSE;
	if (fb->m_names == NULL)
		return;
	for ( item = fb->m_names;; )
	{
		next = item->m_next;
		free(item->m_name);
		free(item);
		item = next;
		if (item == fb->m_names)
			break;
	}
	fb->m_names = NULL;
} /* End of 'filebox_free_names' function */

/* End of 'filebox.c' file */

