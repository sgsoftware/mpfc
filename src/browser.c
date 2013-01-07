/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. File browser functions implementation.
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

#include <stdlib.h>
#include <fnmatch.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "types.h"
#include "browser.h"
#include "help_screen.h"
#include "metadata_io.h"
#include "player.h"
#include "plist.h"
#include "song_info.h"
#include "wnd.h"
#include "util.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_hbox.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"

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

/* Obtain the height of spaces for files in browser */
#define FB_HEIGHT(fb)	(WND_HEIGHT(fb) - 3)

/* Variable names associated with columns length */
char *fb_vars[FB_COL_NUM] = 
{ "fname-len", "title-len", "artist-len", "album-len", 
  "year-len", "genre-len", "comments-len", "track-len", 
  "time-len" };

/* Create a new file browser window */
browser_t *fb_new( wnd_t *parent, char *dir )
{
	browser_t *fb;

	/* Allocate memory */
	fb = (browser_t *)malloc(sizeof(browser_t));
	if (fb == NULL)
		return NULL;
	memset(fb, 0, sizeof(*fb));
	WND_OBJ(fb)->m_class = fb_class_init(WND_GLOBAL(parent));

	/* Initialize window */
	if (!fb_construct(fb, parent, dir))
	{
		free(fb);
		return NULL;
	}
	wnd_postinit(fb);
	return fb;
} /* End of 'fb_new' function */

/* Initialize file browser */
bool_t fb_construct( browser_t *fb, wnd_t *parent, char *dir )
{
	wnd_t *wnd = (wnd_t *)fb;

	/* Initialize window part */
	if (!wnd_construct(wnd, parent, _("File browser"), 0, 0, 0, 0, 
				WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED))
		return FALSE;

	/* Register handlers */
	wnd_msg_add_handler(wnd, "display", fb_on_display);
	wnd_msg_add_handler(wnd, "keydown", fb_on_keydown);
	wnd_msg_add_handler(wnd, "action", fb_on_action);
	wnd_msg_add_handler(wnd, "mouse_ldown", fb_on_mouse_ldown);
	wnd_msg_add_handler(wnd, "mouse_mdown", fb_on_mouse_mdown);
	wnd_msg_add_handler(wnd, "mouse_rdown", fb_on_mouse_rdown);
	wnd_msg_add_handler(wnd, "mouse_ldouble", fb_on_mouse_ldouble);
	wnd_msg_add_handler(wnd, "destructor", fb_destructor);

	/* Set fields */
	util_strncpy(fb->m_cur_dir, dir, sizeof(fb->m_cur_dir));
	fb->m_num_files = 0;
	fb->m_files = NULL;
	fb->m_cursor = 0;
	fb->m_scrolled = 0;
	fb->m_info_mode = FALSE;
	fb->m_search_mode = FALSE;
	strcpy(fb->m_search_str, "");
	fb->m_search_str_len = 0;
	fb_load_files(fb);
	wnd->m_cursor_hidden = TRUE;
	return TRUE;
} /* End of 'fb_init' function */

/* Browser destructor */
void fb_destructor( wnd_t *wnd )
{
	util_strncpy(player_fb_dir, ((browser_t *)wnd)->m_cur_dir, 
			sizeof(player_fb_dir));
	fb_free_files((browser_t *)wnd);
} /* End of 'fb_free' function */

/* Display window */
wnd_msg_retcode_t fb_on_display( wnd_t *wnd )
{
	browser_t *fb = (browser_t *)wnd;
	int i, j, y;

	assert(fb);

	wnd_move(wnd, 0, 0, 0);

	/* Print current directory */
	wnd_apply_style(wnd, "title-style");
	wnd_printf(wnd, 0, 0, _("Current directory is %s\n"), fb->m_cur_dir);

	/* Clean information about items position in window */
	for ( i = 0; i < fb->m_num_files; i ++ )
		fb->m_files[i].m_y = -1;

	/* Print header */
	fb_print_header(fb);
	
	/* Print files */
	y = wnd->m_cursor_y;
	for ( i = fb->m_scrolled, j = y; i < fb->m_num_files && 
			wnd->m_cursor_y < y + FB_HEIGHT(fb); i ++, j ++ )
	{
		byte type = fb->m_files[i].m_type;
		
		wnd_move(wnd, 0, 0, j);
		if (type & FB_ITEM_SEL)
			wnd_apply_style(wnd, i == fb->m_cursor ? "cur-selection-style" :
					"selection-style");
		else if (type & FB_ITEM_DIR)
			wnd_apply_style(wnd, i == fb->m_cursor ? "cur-dir-style" :
					"dir-style");
		else
			wnd_apply_style(wnd, i == fb->m_cursor ? "cur-file-style" :
					"file-style");
		fb->m_files[i].m_y = wnd->m_cursor_y;
		fb_print_file(fb, &fb->m_files[i]);
		if (type & FB_ITEM_DIR)
			wnd_printf(wnd, 0, 0, "/");
	}

	/* Print search stuff */
	if (fb->m_search_mode)
	{
		wnd_move(wnd, 0, 0, WND_HEIGHT(wnd) - 1);
		wnd_apply_style(wnd, "search-prompt-style");
		wnd_printf(wnd, 0, 0, _("Enter name you want to search: "));
		wnd_apply_style(wnd, "search-string-style");
		wnd_printf(wnd, 0, 0, "%s", fb->m_search_str);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_display' function */

/* Handle key pressing */
wnd_msg_retcode_t fb_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	int i;
	browser_t *fb = (browser_t *)wnd;
	char str[MAX_FILE_NAME];
	wnd_msg_retcode_t ret = WND_MSG_RETCODE_STOP;
	assert(fb);

	/* Handle key in search mode */
	if (!fb->m_search_mode)
		return WND_MSG_RETCODE_OK;

	if (key == 27 || key == KEY_CTRL_G)
	{
		fb->m_search_mode = FALSE;
		fb->m_search_str[fb->m_search_str_len = 0] = 0;
	}
	else if (key >= ' ' && key <= 0xFF)
	{
		struct browser_list_item *item;
		const char *str;

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
	else
	{
		fb->m_search_mode = FALSE;
		fb->m_search_str[fb->m_search_str_len = 0] = 0;
		ret = WND_MSG_RETCODE_OK;
	}
	wnd_invalidate(WND_OBJ(fb));
	return ret;
} /* End of 'fb_handle_key' function */

/* 'action' message handler */
wnd_msg_retcode_t fb_on_action( wnd_t *wnd, char *action )
{
	browser_t *fb = (browser_t *)wnd;

	/* Exit */
	if (!strcasecmp(action, "quit"))
	{
		wnd_close(wnd);
	}
	/* Move cursor */
	else if (!strcasecmp(action, "move_down"))
	{
		fb_move_cursor(fb, 1, TRUE);
	}
	else if (!strcasecmp(action, "move_up"))
	{
		fb_move_cursor(fb, -1, TRUE);
	}
	else if (!strcasecmp(action, "screen_down"))
	{
		fb_move_cursor(fb, FB_HEIGHT(fb), TRUE);
	}
	else if (!strcasecmp(action, "screen_up"))
	{
		fb_move_cursor(fb, -FB_HEIGHT(fb), TRUE);
	}
	else if (!strcasecmp(action, "move_to_start"))
	{
		fb_move_cursor(fb, 0, FALSE);
	}
	else if (!strcasecmp(action, "move_to_end"))
	{
		fb_move_cursor(fb, fb->m_num_files - 1, FALSE);
	}
	/* Go to directory */
	else if (!strcasecmp(action, "go_to_dir"))
	{
		fb_go_to_dir(fb);
	}
	/* Go to home directory */
	else if (!strcasecmp(action, "go_to_home"))
	{
		fb_change_dir(fb, getenv("HOME"));
	}
	/* Go to parent directory */
	else if (!strcasecmp(action, "go_to_parent"))
	{
		if (fb->m_num_files && (fb->m_files[0].m_type & FB_ITEM_UPDIR))
		{
			fb->m_cursor = fb->m_scrolled = 0;
			fb_go_to_dir(fb);
		}
	}
	/* Add selected files to playlist */
	else if (!strcasecmp(action, "add_to_playlist"))
	{
		fb_add2plist(fb);
	}
	/* Replace files in playlist with select files */
	else if (!strcasecmp(action, "replace_playlist"))
	{
		fb_replace_plist(fb);
	}
	/* Toggle song info mode */
	else if (!strcasecmp(action, "toggle_info"))
	{
		fb_toggle_info(fb);
	}
	/* Toggle search mode */
	else if (!strcasecmp(action, "toggle_search"))
	{
		fb->m_search_mode = !fb->m_search_mode;
	}
	/* Change directory */
	else if (!strcasecmp(action, "change_dir"))
	{
		fb_cd_dialog(fb);
	}
	/* Select/deselect files */
	else if (!strcasecmp(action, "select"))
	{
		if (fb->m_num_files)
		{
			fb_change_sel(fb, fb->m_cursor);
			fb_move_cursor(fb, 1, TRUE);
		}
	}
	else if (!strcasecmp(action, "select_pattern"))
	{
		fb_select_pattern_dialog(fb, TRUE);
	}
	else if (!strcasecmp(action, "deselect_pattern"))
	{
		fb_select_pattern_dialog(fb, FALSE);
	}
	/* Show help */
	else if (!strcasecmp(action, "help"))
	{
		fb_help(fb);
	}
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_on_action' function */

/* Handle mouse left button */
wnd_msg_retcode_t fb_on_mouse_ldown( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, x, y);
	if (pos >= 0)
	{
		fb_move_cursor(fb, pos, FALSE);
	}
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_handle_mouse' function */

/* Handle mouse right button */
wnd_msg_retcode_t fb_on_mouse_rdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, x, y);
	if (pos >= 0)
	{
		fb_change_sel(fb, pos);
	}
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_handle_mouse_right' function */

/* Handle mouse middle button */
wnd_msg_retcode_t fb_on_mouse_mdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, x, y);
	if (pos >= 0)
	{
		fb->m_cursor = pos;
		fb->m_scrolled = pos - FB_HEIGHT(fb) / 2;
		if (fb->m_scrolled + FB_HEIGHT(fb) > fb->m_num_files)
			fb->m_scrolled = fb->m_num_files - FB_HEIGHT(fb);
		if (fb->m_scrolled < 0)
			fb->m_scrolled = 0;
	}
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_handle_mouse_mdl' function */

/* Handle mouse left button double click */
wnd_msg_retcode_t fb_on_mouse_ldouble( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	int pos;
	browser_t *fb = (browser_t *)wnd;
	
	/* Find item matching these coordinates */
	pos = fb_find_item_by_mouse(fb, x, y);
	if (pos >= 0)
	{
		fb_move_cursor(fb, pos, FALSE);
		fb_go_to_dir(fb);
	}
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_handle_mouse_dbl' function */

/* Move cursor to a specified position */
void fb_move_cursor( browser_t *fb, int pos, bool_t rel )
{
	int was_cursor;

	assert(fb);

	/* Move cursor */
	was_cursor = fb->m_cursor;
	fb->m_cursor = rel ? fb->m_cursor + pos : pos;
	if (fb->m_cursor < 0)
		fb->m_cursor = 0;
	else if (fb->m_cursor >= fb->m_num_files)
		fb->m_cursor = fb->m_num_files - 1;

	/* Scroll */
	if (fb->m_cursor < fb->m_scrolled || 
			fb->m_cursor >= fb->m_scrolled + FB_HEIGHT(fb))
	{
		fb->m_scrolled += (fb->m_cursor - was_cursor);
		if (fb->m_scrolled >= fb->m_num_files - FB_HEIGHT(fb))
			fb->m_scrolled = fb->m_num_files - FB_HEIGHT(fb);
		if (fb->m_scrolled < 0)
			fb->m_scrolled = 0;
	}
} /* End of 'fb_move_cursor' function */

/* Add a file to list */
static void fb_add_file( browser_t *fb, char *name )
{
	struct browser_list_item *item;
	bool_t isdir = FALSE;
	assert(fb);

	char *full_name = util_strcat(fb->m_cur_dir, name, NULL);
	struct stat st;
	if (stat(full_name, &st))
	{
		free(full_name);
		return;
	}
	
	/* Get file type */
	if (S_ISDIR(st.st_mode))
		isdir = TRUE;

	/* Rellocate memory for files list */
	fb->m_files = (struct browser_list_item *)realloc(fb->m_files,
			(fb->m_num_files + 1) * sizeof(*fb->m_files));

	/* If we have a directory - insert it before normal files */
	if (isdir)
	{
		int i;
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
	item->m_full_name = full_name;
	item->m_name = util_short_name(full_name);
	item->m_y = -1;
	item->m_info = NULL;
	item->m_len = 0;
	if (!strcmp(item->m_name, ".."))
		item->m_type |= FB_ITEM_UPDIR;
	fb->m_num_files ++;
} /* End of 'fb_add_file' function */

/* Reload directory files list */
void fb_load_files( browser_t *fb )
{
	assert(fb);

	/* Free current files list */
	fb_free_files(fb);

	/* Add link to parent directory */
	fb->m_files = (struct browser_list_item *)malloc(sizeof(*fb->m_files));
	fb->m_num_files = 0;

	/* Find files */
	struct dirent **namelist;
	int n = scandir(fb->m_cur_dir, &namelist, NULL, alphasort);
	if (n >= 0)
	{
		for ( int i = 0; i < n; ++i )
		{
			char *name = namelist[i]->d_name;

			/* Filter */
			if (strcmp(name, "..") && name[0] == '.')
				goto next;
			
			/* Add this file */
			fb_add_file(fb, name);

		next:
			free(namelist[i]);
		}
		free(namelist);
	}

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

	set = plist_set_new(FALSE);
	for ( i = 0; i < fb->m_num_files; i ++ )
	{
		if (fb->m_files[i].m_type & FB_ITEM_UPDIR)
			continue;
		if (fb->m_files[i].m_type & FB_ITEM_SEL)
			plist_set_add(set, fb->m_files[i].m_full_name);
	}
	plist_add_set(player_plist, set);
	plist_set_free(set);
} /* End of 'fb_add2plist' function */

/* Select/deselect files matching a pattern */
void fb_select_pattern_dialog( browser_t *fb, bool_t sel )
{
	dialog_t *dlg;
	hbox_t *hbox;
	
	dlg = dialog_new(WND_OBJ(fb), sel ? _("Select files matching pattern") :
			_("Deselect files matching pattern"));
	cfg_set_var_int(WND_OBJ(dlg)->m_cfg_list, "select", sel);
	editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("P&attern: "), 
			"pattern", "*", 'a', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", fb_on_sel_pattern);
	dialog_arrange_children(dlg);
} /* End of 'fb_select_pattern' function */

/* Handle 'ok_clicked' for pattern dialog */
wnd_msg_retcode_t fb_on_sel_pattern( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "pattern"));
	browser_t *fb = (browser_t *)(wnd->m_parent);
	int i;

	assert(eb);
	assert(fb);

	/* Select/deselect */
	for ( i = 0; i < fb->m_num_files; i ++ )
		if (!fnmatch(EDITBOX_TEXT(eb), fb->m_files[i].m_name, 0))
		{
			if (cfg_get_var_int(wnd->m_cfg_list, "select"))
				fb->m_files[i].m_type |= FB_ITEM_SEL;
			else
				fb->m_files[i].m_type &= ~FB_ITEM_SEL;
		}
	wnd_invalidate(WND_OBJ(fb));
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_on_sel_pattern' function */

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

	/* Handle tilde expansion */
	if (dir[0] == '~' && (dir[1] == 0 || dir[1] == '/'))
		snprintf(fb->m_cur_dir, sizeof(fb->m_cur_dir),
				"%s%s", getenv("HOME"), dir + 1);
	else
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

		/* Don't load info if it is already loaded */
		if (item->m_info != NULL && item->m_len)
			continue;
		/* Don't load info for directories */
		if (item->m_type & FB_ITEM_DIR)
			continue;

		/* Determine file type and its associated plugin */
		ext = util_extension(item->m_name);
		if (!pmng_search_format(player_pmng, item->m_name, ext))
			continue;

		/* TODO: m_full_name has to have prefix */
		/* Load info */
		si_free(item->m_info);
		item->m_info = md_get_info(item->m_full_name, NULL, &item->m_len);
	}
} /* End of 'fb_load_info' function */

/* Print header */
void fb_print_header( browser_t *fb )
{
	int i;
	wnd_t *wnd = WND_OBJ(fb);

	if (!fb->m_info_mode)
		return;

	wnd_apply_style(wnd, "header-style");
	for ( i = 0; i < FB_COL_NUM; i ++ )
		fb_print_info_col(fb, i, NULL);
	wnd_printf(wnd, 0, 0, "\n");
} /* End of 'fb_print_header' function */

/* Print file */
void fb_print_file( browser_t *fb, struct browser_list_item *item )
{
	wnd_t *wnd = WND_OBJ(fb);

	/* If we are not in info mode - print file name */
	if (!fb->m_info_mode || item->m_info == NULL)
		wnd_printf(wnd, 0, 0, "%s", item->m_name);
	/* Print file info */
	else
	{
		int i;
		
		for ( i = 0; i < FB_COL_NUM; i ++ )
			fb_print_info_col(fb, i, item);
	}
} /* End of 'fb_print_file' function */

/* Print info column */
void fb_print_info_col( browser_t *fb, int id, struct browser_list_item *item )
{
	int size, next_size = 0, i, right;
	wnd_t *wnd = WND_OBJ(fb);
	song_info_t *info;

	if (id < 0 || id >= FB_COL_NUM)
		return;

	size = wnd_get_style_int(WND_OBJ(fb), fb_vars[id]);
	if (size == 0)
		return;
	for ( i = id + 1; i < FB_COL_NUM; i ++ )
	{
		next_size = wnd_get_style_int(WND_OBJ(fb), fb_vars[i]);
		if (next_size > 0)
			break;
	}
	if (item != NULL)
		info = item->m_info;
	
	right = WND_OBJ(fb)->m_cursor_x + size;
	switch (id)
	{
	case FB_COL_FILENAME:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("File name"));
		else
			wnd_printf(wnd, 0, right, "%s", item->m_name);
		break;
	case FB_COL_TITLE:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Title"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_name);
		break;
	case FB_COL_ARTIST:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Artist"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_artist);
		break;
	case FB_COL_ALBUM:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Album"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_album);
		break;
	case FB_COL_YEAR:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Year"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_year);
		break;
	case FB_COL_GENRE:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Genre"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_genre);
		break;
	case FB_COL_TRACK:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Track"));
		else
			wnd_printf(wnd, 0, right, "%s", info->m_genre);
		break;
	case FB_COL_TIME:
		if (item == NULL)
			wnd_printf(wnd, 0, right, _("Time"));
		else
			wnd_printf(wnd, 0, right, "%d:%02d", 
					item->m_len / 60, item->m_len % 60);
		break;
	}
	wnd_move(wnd, WND_MOVE_ADVANCE, right, WND_MOVE_DONT_CHANGE);
	if (next_size > 0)
		wnd_put_special(wnd, ACS_VLINE);
} /* End of 'fb_print_info_col' function */

/* Show help screen */
void fb_help( browser_t *fb )
{
	help_new(WND_ROOT(fb), HELP_BROWSER);
} /* End of 'fb_help' function */

/* Replace files in playlist with selected files  */
void fb_replace_plist( browser_t *fb )
{
	int i;
	plist_set_t *set;

	if (fb == NULL)
		return;

	/* Make a set of files */
	set = plist_set_new(FALSE);
	for ( i = 1; i < fb->m_num_files; i ++ )
		if (fb->m_files[i].m_type & FB_ITEM_SEL)
			plist_set_add(set, fb->m_files[i].m_full_name);

	/* Check that this contains at least one file */
	if (set->m_head == NULL)
	{
		plist_set_free(set);
		return;
	}

	/* Replace files */
	plist_clear(player_plist);
	plist_add_set(player_plist, set);
	plist_set_free(set);
} /* End of 'fb_replace_plist' function */

/* Initialize browser class */
wnd_class_t *fb_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "browser",
			wnd_basic_class_init(global), NULL, NULL,
			fb_class_set_default_styles);
	return klass;
} /* End of 'fb_class_init' function */

/* Set browser class default styles */
void fb_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "file-style", "white:black");
	cfg_set_var(list, "cur-file-style", "white:blue");
	cfg_set_var(list, "dir-style", "green:black");
	cfg_set_var(list, "cur-dir-style", "green:blue");
	cfg_set_var(list, "selection-style", "yellow:black:bold");
	cfg_set_var(list, "cur-selection-style", "yellow:blue:bold");
	cfg_set_var(list, "search-prompt-style", "white:black:bold");
	cfg_set_var(list, "search-string-style", "white:black");
	cfg_set_var(list, "title-style", "green:black:bold");
	cfg_set_var(list, "header-style", "green:black:bold");
	cfg_set_var_int(list, "fname-len", 0);
	cfg_set_var_int(list, "artist-len", 15);
	cfg_set_var_int(list, "title-len", 30);
	cfg_set_var_int(list, "album-len", 20);
	cfg_set_var_int(list, "year-len", 4);
	cfg_set_var_int(list, "comments-len", 0);
	cfg_set_var_int(list, "genre-len", 0);
	cfg_set_var_int(list, "track-len", 0);
	cfg_set_var_int(list, "time-len", 5);

	/* Set kbinds */
	cfg_set_var(list, "kbind.quit", "q;<Escape>");
	cfg_set_var(list, "kbind.move_up", "k;<Up>;<Ctrl-p>");
	cfg_set_var(list, "kbind.move_down", "j;<Down>;<Ctrl-n>");
	cfg_set_var(list, "kbind.screen_up", "u;<PageUp>;<Alt-v>");
	cfg_set_var(list, "kbind.screen_down", "d;<PageDown>;<Ctrl-v>");
	cfg_set_var(list, "kbind.move_to_start", "<Ctrl-a>;<Home>");
	cfg_set_var(list, "kbind.move_to_end", "<Ctrl-e>;<End>");
	cfg_set_var(list, "kbind.go_to_dir", "<Enter>");
	cfg_set_var(list, "kbind.go_to_home", "h");
	cfg_set_var(list, "kbind.go_to_parent", "<Backspace>");
	cfg_set_var(list, "kbind.add_to_playlist", "a");
	cfg_set_var(list, "kbind.replace_playlist", "r");
	cfg_set_var(list, "kbind.toggle_info", "i");
	cfg_set_var(list, "kbind.toggle_search", "s;<Ctrl-s>");
	cfg_set_var(list, "kbind.change_dir", "c");
	cfg_set_var(list, "kbind.select", "<Insert>");
	cfg_set_var(list, "kbind.select_pattern", "+");
	cfg_set_var(list, "kbind.deselect_pattern", "-");
	cfg_set_var(list, "kbind.help", "?");
} /* End of 'fb_class_set_default_styles' function */

/* Launch change directory dialog */
void fb_cd_dialog( browser_t *fb )
{
	dialog_t *dlg;
	filebox_t *eb;

	dlg = dialog_new(WND_OBJ(fb), _("Change directory"));
	eb = filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Directory: "), 
			"dir", "", 'd', 50);
	eb->m_flags |= FILEBOX_ONLY_DIRS;
	EDITBOX_OBJ(eb)->m_history = player_hist_lists[PLAYER_HIST_FB_CD];
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", fb_on_cd);
	dialog_arrange_children(dlg);
} /* End of 'fb_cd_dialog' function */

/* 'ok_clicked' handler for directory changing dialog */
wnd_msg_retcode_t fb_on_cd( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "dir"));
	assert(eb);
	fb_change_dir((browser_t *)wnd->m_parent, EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'fb_on_cd' function */

/* End of 'browser.c' file */

