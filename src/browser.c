/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : browser.c
 * PURPOSE     : SG MPFC. File browser functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.01.2004
 * NOTE        : Module prefix 'fb'.
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
#include <glob.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "types.h"
#include "browser.h"
#include "colors.h"
#include "editbox.h"
#include "error.h"
#include "player.h"
#include "plist.h"
#include "window.h"
#include "util.h"

/* Create a new file browser window */
browser_t *fb_new( wnd_t *parent, int x, int y, int w, int h, char *dir )
{
	browser_t *fb;

	/* Allocate memory */
	fb = (browser_t *)malloc(sizeof(browser_t));
	if (fb == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize window */
	if (!fb_init(fb, parent, x, y, w, h, dir))
	{
		free(fb);
		return NULL;
	}
	return fb;
} /* End of 'fb_new' function */

/* Initialize file browser */
bool_t fb_init( browser_t *fb, wnd_t *parent, int x, int y, int w, 
					int h, char *dir )
{
	wnd_t *wnd = (wnd_t *)fb;

	/* Initialize window part */
	if (!wnd_init(wnd, parent, x, y, w, h))
		return FALSE;

	/* Register handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, fb_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, fb_handle_key);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_CLICK, fb_handle_mouse);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_DOUBLE, fb_handle_mouse_dbl);
	wnd_register_handler(wnd, WND_MSG_MOUSE_RIGHT_CLICK, fb_handle_mouse_right);
	wnd_register_handler(wnd, WND_MSG_MOUSE_MIDDLE_CLICK, fb_handle_mouse_mdl);

	/* Set fields */
	strcpy(fb->m_cur_dir, dir);
	fb->m_num_files = 0;
	fb->m_files = NULL;
	fb->m_cursor = 0;
	fb->m_scrolled = 0;
	fb->m_height = h - 2;
	fb_load_files(fb);
	WND_OBJ(fb)->m_flags |= (WND_INITIALIZED);
	return TRUE;
} /* End of 'fb_init' function */

/* Destroy window */
void fb_free( wnd_t *wnd )
{
	fb_free_files((browser_t *)wnd);
	wnd_destroy(wnd);
} /* End of 'fb_free' function */

/* Display window */
void fb_display( wnd_t *wnd, dword data )
{
	browser_t *fb = (browser_t *)wnd;
	int i, y;

	if (fb == NULL)
		return;

	wnd_move(wnd, 0, 0);

	/* Print current directory */
	col_set_color(wnd, COL_EL_FB_TITLE);
	wnd_printf(wnd, _("Current directory is %s\n"), fb->m_cur_dir);

	/* Clean information about items position in window */
	for ( i = 0; i < fb->m_num_files; i ++ )
		fb->m_files[i].m_y = -1;
	
	/* Print files */
	y = wnd_gety(wnd);
	for ( i = fb->m_scrolled; i < fb->m_num_files && 
			wnd_gety(wnd) < y + fb->m_height; i ++ )
	{
		byte type = fb->m_files[i].m_type;
		
		if (type & FB_ITEM_SEL)
			col_set_color(wnd, i == fb->m_cursor ? COL_EL_FB_SEL_HL :
					COL_EL_FB_SEL);
		else if (type & FB_ITEM_DIR)
			col_set_color(wnd, i == fb->m_cursor ? COL_EL_FB_DIR_HL :
					COL_EL_FB_DIR);
		else
			col_set_color(wnd, i == fb->m_cursor ? COL_EL_FB_FILE_HL :
					COL_EL_FB_FILE);
		fb->m_files[i].m_y = wnd_gety(wnd);
		wnd_printf(wnd, "%s", fb->m_files[i].m_name);
		if (type & FB_ITEM_DIR)
			wnd_printf(wnd, "/\n");
		else
			wnd_printf(wnd, "\n");
	}
	col_set_color(wnd, COL_EL_DEFAULT);
	wnd_clear(wnd, TRUE);

	/* Remove cursor */
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'fb_display' function */

/* Handle key pressing */
void fb_handle_key( wnd_t *wnd, dword data )
{
	int key = (int)data;
	browser_t *fb = (browser_t *)wnd;

	if (fb == NULL)
		return;

	switch (key)
	{
	/* Exit */
	case 'q':
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;

	/* Move cursor */
	case 'j':
	case KEY_DOWN:
		fb_move_cursor(fb, 1, TRUE);
		break;
	case 'k':
	case KEY_UP:
		fb_move_cursor(fb, -1, TRUE);
		break;
	case 'd':
	case KEY_NPAGE:
		fb_move_cursor(fb, fb->m_height, TRUE);
		break;
	case 'u':
	case KEY_PPAGE:
		fb_move_cursor(fb, -fb->m_height, TRUE);
		break;
	case KEY_HOME:
		fb_move_cursor(fb, 0, FALSE);
		break;
	case KEY_END:
		fb_move_cursor(fb, fb->m_num_files - 1, FALSE);
		break;

	/* Go to directory */
	case '\n':
		fb_go_to_dir(fb);
		break;

	/* Add selected files to playlist */
	case 'a':
		fb_add2plist(fb);
		break;

	/* Select/deselect files */
	case KEY_IC:
		if (fb->m_num_files)
		{
			fb_change_sel(fb, fb->m_cursor);
			fb_move_cursor(fb, 1, TRUE);
		}
		break;
	case '+':
	case '-':
		fb_select_pattern(fb, key == '+');
		break;
	}
} /* End of 'fb_handle_key' function */

/* Handle mouse left button */
void fb_handle_mouse( wnd_t *wnd, dword data )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, WND_MOUSE_X(data), WND_MOUSE_Y(data));
	if (pos >= 0)
	{
		fb_move_cursor(fb, pos, FALSE);
	}
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'fb_handle_mouse' function */

/* Handle mouse right button */
void fb_handle_mouse_right( wnd_t *wnd, dword data )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, WND_MOUSE_X(data), WND_MOUSE_Y(data));
	if (pos >= 0)
	{
		fb_change_sel(fb, pos);
	}
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'fb_handle_mouse_right' function */

/* Handle mouse middle button */
void fb_handle_mouse_mdl( wnd_t *wnd, dword data )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, WND_MOUSE_X(data), WND_MOUSE_Y(data));
	if (pos >= 0)
	{
		fb->m_cursor = pos;
		fb->m_scrolled = pos - fb->m_height / 2;
		if (fb->m_scrolled + fb->m_height > fb->m_num_files)
			fb->m_scrolled = fb->m_num_files - fb->m_height;
		if (fb->m_scrolled < 0)
			fb->m_scrolled = 0;
	}
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'fb_handle_mouse_mdl' function */

/* Handle mouse left button double click */
void fb_handle_mouse_dbl( wnd_t *wnd, dword data )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, WND_MOUSE_X(data), WND_MOUSE_Y(data));
	if (pos >= 0)
	{
		fb_move_cursor(fb, pos, FALSE);
		fb_go_to_dir(fb);
	}
	wnd_send_msg(wnd, WND_MSG_DISPLAY, 0);
} /* End of 'fb_handle_mouse_dbl' function */

/* Move cursor to a specified position */
void fb_move_cursor( browser_t *fb, int pos, bool_t rel )
{
	int was_cursor;

	if (fb == NULL)
		return;

	/* Move cursor */
	was_cursor = fb->m_cursor;
	fb->m_cursor = rel ? fb->m_cursor + pos : pos;
	if (fb->m_cursor < 0)
		fb->m_cursor = 0;
	else if (fb->m_cursor >= fb->m_num_files)
		fb->m_cursor = fb->m_num_files - 1;

	/* Scroll */
	if (fb->m_cursor < fb->m_scrolled || 
			fb->m_cursor >= fb->m_scrolled + fb->m_height)
	{
		fb->m_scrolled += (fb->m_cursor - was_cursor);
		if (fb->m_scrolled < 0)
			fb->m_scrolled = 0;
		else if (fb->m_scrolled >= fb->m_num_files - fb->m_height)
			fb->m_scrolled = fb->m_num_files - fb->m_height;
	}
} /* End of 'fb_move_cursor' function */

/* Reload directory files list */
void fb_load_files( browser_t *fb )
{
	glob_t gl;
	int i;
	char pattern[256];

	if (fb == NULL)
		return;

	/* Free current files list */
	fb_free_files(fb);

	/* Add link to parent directory */
	fb->m_files = (struct browser_list_item *)malloc(sizeof(*fb->m_files));
	fb->m_num_files = 1;
	fb->m_files[0].m_name = fb->m_files[0].m_full_name = strdup("..");
	fb->m_files[0].m_type = (FB_ITEM_DIR | FB_ITEM_UPDIR);
	fb->m_files[0].m_y = -1;

	/* Find files */
	memset(&gl, 0, sizeof(gl));
	sprintf(pattern, "%s*", fb->m_cur_dir);
	glob(pattern, GLOB_BRACE | GLOB_TILDE, NULL, &gl);

	/* Add these files */
	for ( i = 0; i < gl.gl_pathc; i ++ )
		fb_add_file(fb, gl.gl_pathv[i]);
	globfree(&gl);
} /* End of 'fb_load_files' function */

/* Free files list */
void fb_free_files( browser_t *fb )
{
	int i;

	if (fb == NULL || fb->m_files == NULL)
		return;

	for ( i = 0; i < fb->m_num_files; i ++ )
		free(fb->m_files[i].m_full_name);
	free(fb->m_files);
	fb->m_files = NULL;
	fb->m_num_files = 0;
} /* End of 'fb_free_files' function */

/* Add a file to list */
void fb_add_file( browser_t *fb, char *name )
{
	struct browser_list_item *item;
	struct stat s;
	bool_t isdir = FALSE;
	int i;

	if (fb == NULL || name == NULL)
		return;
	
	/* Get file type */
	stat(name, &s);
	if (S_ISDIR(s.st_mode))
		isdir = TRUE;

	/* Rellocate memory for files list */
	if (fb->m_files == NULL)
		fb->m_files = (struct browser_list_item *)malloc(sizeof(*fb->m_files));
	else
		fb->m_files = (struct browser_list_item *)realloc(fb->m_files,
				(fb->m_num_files + 1) * sizeof(*fb->m_files));

	/* If we have a directory - insert it before normal files */
	if (isdir)
	{
		for ( i = 0; i < fb->m_num_files; i ++ )
			if (!(fb->m_files[i].m_type & FB_ITEM_DIR))
			{
				memmove(&fb->m_files[i + 1], &fb->m_files[i], 
						(fb->m_num_files - i) * sizeof(*fb->m_files));
				break;
			}
		item = &fb->m_files[i];
	}
	else
		item = &fb->m_files[fb->m_num_files];

	/* Set file data */
	item->m_type = isdir ? FB_ITEM_DIR : 0;
	item->m_full_name = strdup(name);
	item->m_name = util_get_file_short_name(item->m_full_name);
	item->m_y = -1;
	fb->m_num_files ++;
} /* End of 'fb_add_file' function */

/* Go to the directory under cursor */
void fb_go_to_dir( browser_t *fb )
{
	struct browser_list_item *item;
	int cursor = 0;
	char was_dir[256];
	
	if (fb == NULL || fb->m_cursor < 0 || fb->m_cursor >= fb->m_num_files)
		return;

	/* Get current item */
	item = &fb->m_files[fb->m_cursor];
	if (!(item->m_type & FB_ITEM_DIR))
		return;

	/* Set new directory name */
	strcpy(was_dir, fb->m_cur_dir);
	if (item->m_type & FB_ITEM_UPDIR)
	{
		char *s;

		fb->m_cur_dir[strlen(fb->m_cur_dir) - 1] = 0;
		s = strrchr(fb->m_cur_dir, '/');
		if (s == NULL)
		{
			strcat(fb->m_cur_dir, "/");
			return;
		}
		else
		{
			fb->m_cur_dir[s - fb->m_cur_dir + 1] = 0;
		}
	}
	else
		sprintf(fb->m_cur_dir, "%s/", item->m_full_name);
	fb_load_files(fb);

	/* Set cursor */
	if (fb->m_cursor == 0)
	{
		int i;
		
		for ( i = 0; i < fb->m_num_files; i ++ )
		{
			if (!strncmp(fb->m_files[i].m_full_name, was_dir, 
						strlen(was_dir) - 1))
				break;
		}
		if (i < fb->m_num_files)
			cursor = i;
	}
	fb_move_cursor(fb, cursor, FALSE);
} /* End of 'fb_go_to_dir' function */

/* Add selected files to play list */
void fb_add2plist( browser_t *fb )
{
	int i;

	if (fb == NULL)
		return;

	for ( i = 1; i < fb->m_num_files; i ++ )
		if (fb->m_files[i].m_type & FB_ITEM_SEL)
			plist_add(player_plist, fb->m_files[i].m_full_name);
} /* End of 'fb_add2plist' function */

/* Select/deselect files matching a pattern */
void fb_select_pattern( browser_t *fb, bool_t sel )
{
	editbox_t *box;
	char pattern[256];
	int i;

	if (fb == NULL)
		return;

	/* Display box for pattern input */
	box = ebox_new(WND_OBJ(fb), 0, WND_HEIGHT(fb) - 1, WND_WIDTH(fb), 1,
			256, sel ? _("Select files matching pattern: ") : 
			_("Deselect files matching pattern: "), "*");
	if (box == NULL)
		return;
	box->m_hist_list = player_hist_lists[PLAYER_HIST_FB_PATTERN];
	wnd_run(box);

	/* Get pattern */
	strcpy(pattern, box->m_text);

	/* Free memory */
	wnd_destroy(box);

	/* Select/deselect */
	for ( i = 0; i < fb->m_num_files; i ++ )
		if (!fnmatch(pattern, fb->m_files[i].m_name, 0))
		{
			if (sel)
				fb->m_files[i].m_type |= FB_ITEM_SEL;
			else
				fb->m_files[i].m_type &= ~FB_ITEM_SEL;
		}
} /* End of 'fb_select_pattern' function */

/* Find a browser item by the mouse coordinates */
int fb_find_item_by_mouse( browser_t *fb, int x, int y )
{
	int i;

	if (fb == NULL || x < 0 || y < 0 || x >= WND_WIDTH(fb) ||
			y >= WND_HEIGHT(fb))
		return -1;

	for ( i = 0; i < fb->m_num_files; i ++ )
	{
		if (fb->m_files[i].m_y == y)
			return i;
	}
	return -1;
} /* End of 'fb_find_item_by_mouse' function */

/* End of 'browser.c' file */

