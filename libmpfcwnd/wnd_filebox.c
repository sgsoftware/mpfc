/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. File box functions implementation.
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
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "types.h"
#include "mystring.h"
#include "wnd.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"
#include "wnd_hbox.h"
#include "wnd_label.h"
#include "util.h"

/* Free names list */
static void filebox_free_names( filebox_t *fb )
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

/* Handler for glob */
static void filebox_scandir( filebox_t *fb, char *dirname )
{
	/* Expand tilde */
	char *real_name = dirname;
	if (dirname[0] == '~' && (dirname[1] == 0 || dirname[1] == '/'))
	{
		char *home = getenv("HOME");
		if (home)
			real_name = util_strcat(home, dirname + 1, NULL);
	}

	struct dirent **namelist;
	int n = scandir(real_name, &namelist, NULL, alphasort);
	if (n < 0)
		goto finally;

	for ( int i = 0; i < n; ++i )
	{
		char *name = namelist[i]->d_name;

		struct stat st;
		char *fullname = util_strcat(real_name, "/", name, NULL);
		int st_ret = stat(fullname, &st);
		free(fullname);
		if (st_ret)
			goto next;

		/* Filter file */
		if (name[0] == '.' ||
				strncmp(STR_TO_CPTR(fb->m_pattern), name, STR_LEN(fb->m_pattern)))
			goto next;
		if (fb->m_command_box && !fb->m_not_first)
		{
			if (!(st.st_mode & S_IXUSR))
				goto next;
		}
		if ((fb->m_flags & FILEBOX_ONLY_DIRS) && !S_ISDIR(st.st_mode))
			goto next;

		/* Escape when in command mode */
		bool_t escape = FALSE;
		size_t sz = strlen(name) + 2;
		if (fb->m_command_box)
		{
			escape = TRUE;
			sz *= 2;
		}

		/* Create names list item */
		struct filebox_name_t *item = (struct filebox_name_t *)malloc(sizeof(*item));
		item->m_name = (char *)malloc(sz);
		if (escape)
			util_escape_fname(item->m_name, name);
		else
			strcpy(item->m_name, name);
		if (S_ISDIR(st.st_mode))
			strcat(item->m_name, "/");

		/* It's the first name */
		if (fb->m_names == NULL)
		{
			item->m_next = item;
			item->m_prev = item;
			fb->m_names = item;
		}
		else
		{
			item->m_next = fb->m_names;
			item->m_prev = fb->m_names->m_prev;
			fb->m_names->m_prev->m_next = item;
			fb->m_names->m_prev = item;
		}

	next:
		free(namelist[i]);
	}
	free(namelist);

finally:
	if (real_name != dirname)
		free(real_name);
} /* End of 'filebox_scandir' function */

/* Load names list */
static void filebox_load_names( filebox_t *fb )
{
	str_t *text = EDITBOX_OBJ(fb)->m_text, *dirname = NULL;
	int cursor = EDITBOX_OBJ(fb)->m_cursor;
	int i;
	int field_begin = 0;
	bool_t not_first = FALSE;
	bool_t use_global = FALSE;
	assert(fb);

	/* In case of command box mode multiple fields are allowed */
	if (fb->m_command_box)
	{
		char ch;

		/* Find fields delimeter */
		for ( i = cursor - 1; i >= 0; i -- )
		{
			char ch = STR_AT(text, i);
			if ((ch == ' ') && ((i == 0) || 
					((i > 0) && (STR_AT(text, i - 1) != '\\'))))
			{
				field_begin = i + 1;
				not_first = TRUE;
				break;
			}
		}
		ch = STR_AT(text, field_begin);
		use_global = (!not_first && !(ch == '.' || ch == '/' || ch == '~'));
	}

	/* Get directory name */
	if (!use_global)
	{
		for ( i = cursor - 1; i >= field_begin; i -- )
		{
			char ch = STR_AT(text, i);
			if (ch == '/')
				break;
		}
		dirname = str_substring(text, field_begin, i);
		if (dirname == NULL)
			return;
	}

	/* List files in the directory */
	fb->m_pattern = str_substring(text, i + 1, cursor - 1);
	fb->m_not_first = not_first;
	fb->m_use_global = use_global;
	if (!use_global)
	{
		filebox_scandir(fb, STR_TO_CPTR(dirname));
	}
	/* List global executables */
	else
	{
		char *path = getenv("PATH");
		for ( ;; )
		{
			char *s = strchr(path, ':');
			if (s != NULL)
				(*s) = 0;
			filebox_scandir(fb, path);
			if (s != NULL)
			{
				(*s) = ':';
				path = s + 1;
			}
			else
				break;
		}
	}

	str_free(fb->m_pattern);
	str_free(dirname);
	fb->m_names_valid = TRUE;
	fb->m_insert_start = i + 1;
	return;
} /* End of 'filebox_load_names' function */

/* Insert next entry in the file names */
static void filebox_insert_next( filebox_t *fb )
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
		eb->m_state_changed = FALSE;
	}
} /* End of 'filebox_insert_next' function */

/* 'action' message handler */
static wnd_msg_retcode_t filebox_on_action( wnd_t *wnd, char *action )
{
	filebox_t *fb = FILEBOX_OBJ(wnd);

	/* Do completion */
	if (!strcasecmp(action, "complete"))
	{
		/* Free if something has changed */
		if (EDITBOX_OBJ(wnd)->m_state_changed)
		{
			filebox_free_names(fb);
		}

		/* Insert next name */
		filebox_insert_next(fb);
		wnd_msg_send(wnd, "changed", editbox_changed_new());
		wnd_invalidate(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'filebox_on_action' function */

/* Destructor */
static void filebox_destructor( wnd_t *wnd )
{
	filebox_free_names(FILEBOX_OBJ(wnd));
} /* End of 'filebox_destructor' function */

/* File box constructor */
bool_t filebox_construct( filebox_t *fb, wnd_t *parent, char *id, char *text, 
		char letter, int width )
{
	wnd_t *wnd = WND_OBJ(fb);

	/* Initialize edit box part */
	if (!editbox_construct(EDITBOX_OBJ(fb), parent, id, text, letter, width))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(wnd, "action", filebox_on_action);
	wnd_msg_add_handler(wnd, "destructor", filebox_destructor);
	return TRUE;
} /* End of 'filebox_construct' function */

/* Set file box class default styles */
static void filebox_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "kbind.complete", "<Tab>");
} /* End of 'filebox_class_set_default_styles' function */

/* Initialize file box class */
static wnd_class_t *filebox_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "filebox", editbox_class_init(global), NULL,
			NULL, filebox_class_set_default_styles);
} /* End of 'filebox_class_init' function */

/* Create a new file box */
filebox_t *filebox_new( wnd_t *parent, char *id, char *text, char letter,
		int width )
{
	filebox_t *fb;

	/* Allocate memory */
	fb = (filebox_t *)malloc(sizeof(*fb));
	if (fb == NULL)
		return NULL;
	memset(fb, 0, sizeof(*fb));
	WND_OBJ(fb)->m_class = filebox_class_init(WND_GLOBAL(parent));

	/* Initialize file box */
	if (!filebox_construct(fb, parent, id, text, letter, width))
	{
		free(fb);
		return NULL;
	}
	WND_FLAGS(fb) |= WND_FLAG_INITIALIZED;
	return fb;
} /* End of 'filebox_new' function */

/* Create an edit box with label */
filebox_t *filebox_new_with_label( wnd_t *parent, char *title, char *id,
		char *text, char letter, int width )
{
	hbox_t *hbox;
	hbox = hbox_new(parent, NULL, 0);
	label_new(WND_OBJ(hbox), title, "", 0);
	return filebox_new(WND_OBJ(hbox), id, text, letter - mbslen(title), width);
} /* End of 'filebox_new_with_label' function */

/* End of 'filebox.c' file */

