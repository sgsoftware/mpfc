/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : file_input.c
 * PURPOSE     : SG Konsamp. File input edit box functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.05.2003
 * NOTE        : Module prexix 'fin'.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "editbox.h"
#include "error.h"
#include "file_input.h"
#include "window.h"
#include "util.h"

/* Create a new file input box */
file_input_box_t *fin_new( wnd_t *parent, int x, int y, int width, char *label )
{
	file_input_box_t *wnd;
	
	/* Allocate memory */
	wnd = (file_input_box_t *)malloc(sizeof(file_input_box_t));
	if (wnd == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize file input box fields */
	if (!fin_init(wnd, parent, x, y, width, label))
	{
		free(wnd);
		return NULL;
	}
	
	return wnd;
} /* End of 'fin_new' function */

/* Initialize file input box */
bool fin_init( file_input_box_t *fin, wnd_t *parent, int x, int y,
	   			int w, char *label )
{
	/* Create common edit box part */
	if (!ebox_init(&fin->m_box, parent, x, y, w, 1, 256, label, ""))
	{
		return FALSE;
	}

	/* Set fileinputbox-specific fields */
	strcpy(fin->m_cur_path, "");
	fin->m_dir_list = fin->m_cur_list = NULL;
	fin->m_was_cursor = -1;
	fin->m_paste_len = 0;
	((wnd_t *)fin)->m_wnd_destroy = fin_destroy;
	wnd_register_handler(fin, WND_MSG_KEYDOWN, fin_handle_key);
	return TRUE;
} /* End of 'fin_init' function */

/* Destroy file input box */
void fin_destroy( wnd_t *wnd )
{
	/* Free directory list */
	fin_free_list((file_input_box_t *)wnd);

	/* Destroy edit box */
	ebox_destroy(wnd);
} /* End of 'fin_destroy' function */
 
/* File input box key handler function */
void fin_handle_key( wnd_t *wnd, dword data )
{
	file_input_box_t *box = (file_input_box_t *)wnd;
	int key = (int)data;

	/* Remember last pressed key */
	box->m_box.m_last_key = key;

	/* Add character to text if it is printable */
	if (key >= ' ' && key < 256)
		ebox_add(&box->m_box, key);
	/* Expand file name */
	else if (key == '\t')
		fin_expand(box);
	/* Move cursor */
	else if (key == KEY_RIGHT)
		ebox_move(&box->m_box, TRUE, 1);
	else if (key == KEY_LEFT)
		ebox_move(&box->m_box, TRUE, -1);
	else if (key == KEY_HOME)
		ebox_move(&box->m_box, FALSE, 0);
	else if (key == KEY_END)
		ebox_move(&box->m_box, FALSE, box->m_box.m_len);
	/* Delete character */
	else if (key == KEY_BACKSPACE)
		ebox_del(&box->m_box, box->m_box.m_cursor - 1);
	else if (key == KEY_DC)
		ebox_del(&box->m_box, box->m_box.m_cursor);
	/* Exit */
	else if (key == '\n' || key == 27)
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);

	/* Break expansion cycle if we have pressed anything except TAB */
	if (key != '\t')
	{
		box->m_was_cursor = -1;
		box->m_cur_list = NULL;
		box->m_paste_len = 0;
	}
} /* End of 'fin_handle_key' function */

/* Expand file name */
void fin_expand( file_input_box_t *box )
{
	char path[256];
	int i;

	/* Do following things if we are not in expansion cycle now */
	if (box->m_was_cursor == -1)
	{
		/* Get current path */
		for ( i = box->m_box.m_cursor; 
				i >= 0 && box->m_box.m_text[i] != '/'; i -- );
		strcpy(path, box->m_box.m_text);
		path[i + 1] = 0;
	
		/* Rebuild directory list if it is invalid now */
		if (box->m_dir_list == NULL || strcmp(box->m_cur_path, path))
		{
			strcpy(box->m_cur_path, path);
			fin_rebuild_list(box);
		}
	}

	/* Expand name */
	if (box->m_dir_list != NULL)
	{
		/* Was are starting expansion cycle */
		if (box->m_was_cursor == -1)
		{
			/* Search */
			if (fin_search(box))
			{
				box->m_was_cursor = box->m_box.m_cursor;
				fin_paste(box);
			}
			else
			{
				return;
			}
		}
		/* Simply paste current match */
		else
		{
			fin_paste(box);
		}

		/* Search for next match */
		if (!fin_search(box))
		{
			box->m_was_cursor = -1;
			box->m_cur_list = NULL;
			box->m_paste_len = 0;
		}
	}
} /* End of 'fin_expand' function */

/* Search for current file name in list */
bool fin_search( file_input_box_t *box )
{
	fin_list_t *l, *lb;
	bool found = FALSE;
	char *text = box->m_box.m_text;
	int last_slash, cur;

	/* Do nothing if list is empty or 
	 * if it consists of one element which is already pasted */
	if (box->m_dir_list == NULL || 
			(box->m_dir_list == box->m_dir_list->m_next && 
			 box->m_cur_list != NULL))
	{
		return FALSE;
	}

	/* Symbol we start searching from */
	cur = (box->m_was_cursor == -1) ? box->m_box.m_cursor - 1: 
				box->m_was_cursor - 1;

	/* Find last slash */
	for ( last_slash = cur; last_slash >= 0 && text[last_slash] != '/';
	   		last_slash -- );

	/* Set first list item */
	l = (box->m_cur_list == NULL) ? box->m_dir_list : box->m_cur_list->m_next;
	lb = (box->m_cur_list == NULL) ? box->m_dir_list : box->m_cur_list;

	/* Search cycle */
	do
	{
		int i, j;
		bool equal = TRUE;
		char *ptext = l->m_name;

		/* Compare */
		for ( i = cur, j = cur - last_slash - 1; 
				i >= 0 && j >= 0 && text[i] != '/'; i --, j -- )
		{
			/* If symbols differ - strings are not equal */
			if (text[i] != ptext[j])
			{
				equal = FALSE;
				break;
			}
		}

		/* If names are equal - all is OK; else - go to next list item */
		if (equal)
			found = TRUE;
		else
			l = l->m_next;
	} while (l != lb && !found);

	/* Save new current list item and exit */
	box->m_cur_list = l;
	return found;
} /* End of 'fin_search' function */

/* Paste current match */
void fin_paste( file_input_box_t *box )
{
	char *text = box->m_box.m_text, *ptext = box->m_cur_list->m_name;
	int last_slash;

	/* Find last slash */
	for ( last_slash = box->m_was_cursor; last_slash >= 0 &&
			text[last_slash] != '/'; last_slash -- );
	
	/* Shift name after cursor and paste expansion name */
	memmove(&text[last_slash + strlen(ptext) + 1], 
			&text[box->m_was_cursor + box->m_paste_len],
			strlen(text) - box->m_was_cursor - box->m_paste_len + 1);
	memcpy(&text[box->m_was_cursor], &ptext[box->m_was_cursor - last_slash - 1],
			(box->m_paste_len = last_slash + strlen(ptext) - 
			 	box->m_was_cursor + 1));

	/* Save new text length and move cursor */
	box->m_box.m_len = strlen(text);
	ebox_set_cursor(&box->m_box, last_slash + strlen(ptext) + 1);
} /* End of 'fin_paste' function */

/* Rebuild directory list */
void fin_rebuild_list( file_input_box_t *box )
{
	FILE *fd;
	char str[256], fname[256];
	fin_list_t *l = NULL;
	
	/* Free existing list */
	fin_free_list(box);

	/* Open pipe to 'ls' command and add each file it prints */
	util_escape_fname(fname, box->m_cur_path);
	sprintf(str, "ls %s -p 2>/dev/null", fname);
	fd = popen(str, "r");
	while (fd != NULL)
	{
		char fn[256];
			
		fgets(fn, 256, fd);
		if (feof(fd))
			break;
		if (fn[strlen(fn) - 1] == '\n')
			fn[strlen(fn) - 1] = 0;
		l = fin_add_entry(box, fn, l);
	}
	pclose(fd);
} /* End of 'fin_rebuild_list' function */

/* Add an entry to list */
fin_list_t *fin_add_entry( file_input_box_t *box, char *name, 
							fin_list_t *l )
{
	/* List is empty - create new list */
	if (l == NULL)
	{
		box->m_dir_list = (fin_list_t *)malloc(sizeof(fin_list_t));
		strcpy(box->m_dir_list->m_name, name);
		box->m_dir_list->m_next = box->m_dir_list;
		return box->m_dir_list;
	}
	/* List isn't empty - add element to its end */
	else
	{
		l->m_next = (fin_list_t *)malloc(sizeof(fin_list_t));
		strcpy(l->m_next->m_name, name);
		l->m_next->m_next = box->m_dir_list;
		return l->m_next;
	}
} /* End of 'fin_add_entry' function */

/* Free directory list */
void fin_free_list( file_input_box_t *box )
{
	if (box->m_dir_list != NULL)
	{
		fin_list_t *base = box->m_dir_list, *l = base;

		do
		{
			fin_list_t *t = l->m_next;
			
			free(l);
			l = t;
		} while (l != base);
		box->m_dir_list = box->m_cur_list = NULL;
	}
} /* End of 'fin_free_list' function */

/* End of 'file_input.c' file */

