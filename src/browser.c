/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : browser.c
 * PURPOSE     : SG MPFC. File browser functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.03.2004
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
#include "help_screen.h"
#include "player.h"
#include "plist.h"
#include "song_info.h"
#include "window.h"
#include "util.h"

/* Info mode columns IDs */
#define FB_COL_FILENAME 0
#define FB_COL_TITLE 1
#define FB_COL_ARTIST 2
#define FB_COL_ALBUM 3
#define FB_COL_YEAR 4
#define FB_COL_GENRE 5
#define FB_COL_COMMENTS 6
#define FB_COL_TRACK 7
#define FB_COL_TIME 8
#define FB_COL_NUM 9

/* Variable names associated with columns length */
char *fb_vars[FB_COL_NUM] = 
{ "fb-fname-len", "fb-title-len", "fb-artist-len", "fb-album-len", 
  "fb-year-len", "fb-genre-len", "fb-comments-len", "fb-track-len", 
  "fb-time-len" };

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
	util_strncpy(fb->m_cur_dir, dir, sizeof(fb->m_cur_dir));
	fb->m_num_files = 0;
	fb->m_files = NULL;
	fb->m_cursor = 0;
	fb->m_scrolled = 0;
	fb->m_height = h - 3;
	fb->m_info_mode = FALSE;
	fb->m_search_mode = FALSE;
	strcpy(fb->m_search_str, "");
	fb->m_search_str_len = 0;
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

	/* Print header */
	fb_print_header(fb);
	
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
		fb_print_file(fb, &fb->m_files[i]);
		if (type & FB_ITEM_DIR)
			wnd_printf(wnd, "/\n");
		else
			wnd_printf(wnd, "\n");
	}
	col_set_color(wnd, COL_EL_DEFAULT);
	wnd_clear(wnd, TRUE);

	/* Print search stuff */
	if (fb->m_search_mode)
	{
		wnd_move(wnd, 0, wnd->m_height - 1);
		wnd_set_attrib(wnd, A_BOLD);
		wnd_printf(wnd, "Enter name you want to search: ");
		wnd_set_attrib(wnd, A_NORMAL);
		wnd_printf(wnd, "%s", fb->m_search_str);
	}

	/* Remove cursor */
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'fb_display' function */

/* Handle key pressing */
void fb_handle_key( wnd_t *wnd, dword data )
{
	int key = (int)data, i;
	browser_t *fb = (browser_t *)wnd;
	char str[MAX_FILE_NAME];

	if (fb == NULL)
		return;

	/* Handle key in search mode */
	if (fb->m_search_mode)
	{
		bool_t dont_exit = FALSE;

		if (key == 27)
		{
			fb->m_search_mode = FALSE;
			fb->m_search_str[fb->m_search_str_len = 0] = 0;
		}
		else if (key >= ' ' && key <= 0xFF)
		{
			struct browser_list_item *item;
			char *str;

			fb->m_search_str[fb->m_search_str_len ++] = key;
			fb->m_search_str[fb->m_search_str_len] = 0;

			for ( i = 0; i < fb->m_num_files; i ++ )
			{
				item = &fb->m_files[i];
				if (fb->m_info_mode && !(item->m_type & 
							(FB_ITEM_DIR | FB_ITEM_UPDIR)) && 
						item->m_info != NULL && item->m_info->m_name != NULL)
					str = item->m_info->m_name;
				else 
					str = item->m_name;
				if (!strncasecmp(str, fb->m_search_str, fb->m_search_str_len))
					break;
			}

			if (i < fb->m_num_files)
				fb_move_cursor(fb, i, FALSE);
			else
				fb->m_search_str[--fb->m_search_str_len] = 0;
		}
		else if (key == KEY_BACKSPACE)
		{
			if (fb->m_search_str_len > 0)
				fb->m_search_str[--fb->m_search_str_len] = 0;
			else
			{
				fb->m_search_mode = FALSE;
				fb->m_search_str[fb->m_search_str_len = 0] = 0;
			}
		}
		else if (key == '\n' || key == KEY_IC || key == KEY_RIGHT ||
					key == KEY_LEFT || key == KEY_DOWN || key == KEY_UP ||
					key == KEY_NPAGE || key == KEY_PPAGE || key == KEY_HOME ||
					key == KEY_END)
		{
			fb->m_search_mode = FALSE;
			fb->m_search_str[fb->m_search_str_len = 0] = 0;
			dont_exit = TRUE;
		}
		if (!dont_exit)
			return;
	}

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

	/* Go to home directory */
	case 'h':
		fb_change_dir(fb, getenv("HOME"));
		break;

	/* Go to parent directory */
	case KEY_BACKSPACE:
		if (fb->m_num_files && (fb->m_files[0].m_type & FB_ITEM_UPDIR))
		{
			fb->m_cursor = fb->m_scrolled = 0;
			fb_go_to_dir(fb);
		}
		break;

	/* Add selected files to playlist */
	case 'a':
		fb_add2plist(fb);
		break;

	/* Replace files in playlist with select files */
	case 'r':
		fb_replace_plist(fb);
		break;
 
	/* Toggle song info mode */
	case 'i':
		fb_toggle_info(fb);
		break;

	/* Toggle search mode */
	case 's':
		fb->m_search_mode = !fb->m_search_mode;
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

	/* Show help */
	case '?':
		fb_help(fb);
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
	char pattern[MAX_FILE_NAME];

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
	fb->m_files[0].m_info = NULL;
	fb->m_files[0].m_len = 0;

	/* Find files */
	memset(&gl, 0, sizeof(gl));
	snprintf(pattern, sizeof(pattern), "%s*", fb->m_cur_dir);
	glob(pattern, GLOB_BRACE | GLOB_TILDE, NULL, &gl);

	/* Add these files */
	for ( i = 0; i < gl.gl_pathc; i ++ )
		fb_add_file(fb, gl.gl_pathv[i]);
	globfree(&gl);

	/* Load info if we are in info mode */
	if (fb->m_info_mode)
		fb_load_info(fb);
} /* End of 'fb_load_files' function */

/* Free files list */
void fb_free_files( browser_t *fb )
{
	int i;

	if (fb == NULL || fb->m_files == NULL)
		return;

	for ( i = 0; i < fb->m_num_files; i ++ )
	{
		free(fb->m_files[i].m_full_name);
		si_free(fb->m_files[i].m_info);
	}
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
	item->m_name = util_short_name(item->m_full_name);
	item->m_y = -1;
	item->m_info = NULL;
	item->m_len = 0;
	fb->m_num_files ++;
} /* End of 'fb_add_file' function */

/* Go to the directory under cursor */
void fb_go_to_dir( browser_t *fb )
{
	struct browser_list_item *item;
	int cursor = 0;
	char was_dir[MAX_FILE_NAME];
	
	if (fb == NULL || fb->m_cursor < 0 || fb->m_cursor >= fb->m_num_files)
		return;

	/* Get current item */
	item = &fb->m_files[fb->m_cursor];
	if (!(item->m_type & FB_ITEM_DIR))
		return;

	/* Set new directory name */
	util_strncpy(was_dir, fb->m_cur_dir, sizeof(was_dir));
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
		snprintf(fb->m_cur_dir, sizeof(fb->m_cur_dir), "%s/", 
				item->m_full_name);
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
	plist_set_t *set;

	if (fb == NULL)
		return;

	set = plist_set_new();
	for ( i = 1; i < fb->m_num_files; i ++ )
		if (fb->m_files[i].m_type & FB_ITEM_SEL)
			plist_set_add(set, fb->m_files[i].m_full_name);
	plist_add_set(player_plist, set);
	plist_set_free(set);
} /* End of 'fb_add2plist' function */

/* Select/deselect files matching a pattern */
void fb_select_pattern( browser_t *fb, bool_t sel )
{
	editbox_t *box;
	char *pattern;
	int i;

	if (fb == NULL)
		return;

	/* Display box for pattern input */
	box = ebox_new(WND_OBJ(fb), 0, WND_HEIGHT(fb) - 1, WND_WIDTH(fb), 1,
			-1, sel ? _("Select files matching pattern: ") : 
			_("Deselect files matching pattern: "), "*");
	if (box == NULL)
		return;
	box->m_hist_list = player_hist_lists[PLAYER_HIST_FB_PATTERN];
	wnd_run(box);
	if (box->m_last_key == 27)
	{
		wnd_destroy(box);
		return;
	}

	/* Get pattern */
	pattern = strdup(EBOX_TEXT(box));

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
	free(pattern);
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

/* Change directory */
void fb_change_dir( browser_t *fb, char *dir )
{
	if (fb == NULL)
		return;

	util_strncpy(fb->m_cur_dir, dir, sizeof(fb->m_cur_dir));
	if (fb->m_cur_dir[strlen(fb->m_cur_dir) - 1] != '/')
		strcat(fb->m_cur_dir, "/");
	fb_load_files(fb);
	fb->m_cursor = 0;
	fb->m_scrolled = 0;
} /* End of 'fb_change_dir' function */

/* Toggle song info mode */
void fb_toggle_info( browser_t *fb )
{
	if (fb == NULL)
		return;

	/* Change mode */
	fb->m_info_mode = !fb->m_info_mode;
	if (fb->m_info_mode)
		fb_load_info(fb);
} /* End of 'fb_toggle_info' function */

/* Load song info */
void fb_load_info( browser_t *fb )
{
	int i;

	if (fb == NULL)
		return;

	for ( i = 0; i < fb->m_num_files; i ++ )
	{
		struct browser_list_item *item = &fb->m_files[i];
		char *ext;
		in_plugin_t *inp;

		/* Don't load info if it is already loaded */
		if (item->m_info != NULL && item->m_len)
			continue;
		/* Don't load info for directories */
		if (item->m_type & FB_ITEM_DIR)
			continue;

		/* Determine file type and its associated plugin */
		ext = util_extension(item->m_name);
		inp = pmng_search_format(player_pmng, ext);
		if (inp == NULL)
			continue;

		/* Load info */
		si_free(item->m_info);
		item->m_info = inp_get_info(inp, item->m_full_name, &item->m_len);
	}
} /* End of 'fb_load_info' function */

/* Print header */
void fb_print_header( browser_t *fb )
{
	int i;
	wnd_t *wnd = WND_OBJ(fb);

	if (!fb->m_info_mode)
		return;

	for ( i = 0; i < FB_COL_NUM; i ++ )
		fb_print_info_col(fb, i, NULL);
	wnd_printf(wnd, "\n");
} /* End of 'fb_print_header' function */

/* Print file */
void fb_print_file( browser_t *fb, struct browser_list_item *item )
{
	wnd_t *wnd = WND_OBJ(fb);
	
	/* If we are not in info mode - print file name */
	if (!fb->m_info_mode || item->m_info == NULL)
		wnd_printf(wnd, "%s", item->m_name);
	/* Print file info */
	else
	{
		int i;
		
		for ( i = 0; i < FB_COL_NUM; i ++ )
			fb_print_info_col(fb, i, item);
	}
} /* End of 'fb_print_file' function */

/* Print info column */
void fb_print_info_col( browser_t *fb, int id, 
							struct browser_list_item *item )
{
	int size, next_size = 0, i;
	wnd_t *wnd = WND_OBJ(fb);
	song_info_t *info;

	if (id < 0 || id >= FB_COL_NUM)
		return;

	size = cfg_get_var_int(cfg_list, fb_vars[id]);
	if (size == 0)
		return;
	for ( i = id + 1; i < FB_COL_NUM; i ++ )
	{
		next_size = cfg_get_var_int(cfg_list, fb_vars[i]);
		if (next_size > 0)
			break;
	}
	if (item != NULL)
		info = item->m_info;
	
	switch (id)
	{
	case FB_COL_FILENAME:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("File name"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					item->m_name);
		break;
	case FB_COL_TITLE:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Title"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_name);
		break;
	case FB_COL_ARTIST:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Artist"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_artist);
		break;
	case FB_COL_ALBUM:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Album"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_album);
		break;
	case FB_COL_YEAR:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Year"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_year);
		break;
	case FB_COL_GENRE:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Genre"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_genre);
		break;
	case FB_COL_TRACK:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Track"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%s", 
					info->m_genre);
		break;
	case FB_COL_TIME:
		if (item == NULL)
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, _("Time"));
		else
			wnd_printf_bound(wnd, size, WND_PRINT_FILL_REST, "%d:%02d", 
					item->m_len / 60, item->m_len % 60);
		break;
	}
	if (next_size > 0)
		wnd_print_char(wnd, ACS_VLINE);
} /* End of 'fb_print_info_col' function */

/* Show help screen */
void fb_help( browser_t *fb )
{
	help_screen_t *h;

	h = help_new(WND_OBJ(fb), HELP_BROWSER, 0, 0, WND_WIDTH(fb), 
			WND_HEIGHT(fb));
	wnd_run(h);
	wnd_destroy(h);
} /* End of 'fb_help' function */

/* Replace files in playlist with selected files  */
void fb_replace_plist( browser_t *fb )
{
	int i;
	plist_set_t *set;

	if (fb == NULL)
		return;

	/* Clear playlist first */
	plist_clear(player_plist);

	/* Add these files */
	set = plist_set_new();
	for ( i = 1; i < fb->m_num_files; i ++ )
		if (fb->m_files[i].m_type & FB_ITEM_SEL)
			plist_set_add(set, fb->m_files[i].m_full_name);
	plist_add_set(player_plist, set);
	plist_set_free(set);
} /* End of 'fb_replace_plist' function */

/* End of 'browser.c' file */

