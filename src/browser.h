/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for file browser functions.
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

#ifndef __SG_MPFC_BROWSER_H__
#define __SG_MPFC_BROWSER_H__

#include "types.h"
#include "main_types.h"
#include "song_info.h"
#include "wnd.h"

/* File browser window type */
typedef struct 
{
	/* Common window part */
	wnd_t m_wnd;

	/* Current directory name */
	char m_cur_dir[MAX_FILE_NAME];

	/* Directory contents */
	struct browser_list_item
	{
		const char *m_name;
		char *m_full_name;
		song_info_t *m_info;
		song_time_t m_len;
		int m_y;
		byte m_type;

/* Item types */
#define FB_ITEM_DIR 0x01
#define FB_ITEM_SEL 0x02
#define FB_ITEM_UPDIR 0x04

	} *m_files;
	int m_num_files;

	/* Cursor position */
	int m_cursor;

	/* Scrolling value */ 
	int m_scrolled;

	/* Is info mode active? */
	bool_t m_info_mode;

	/* Is search mode active? */
	bool_t m_search_mode;

	/* Current search string */
	char m_search_str[MAX_FILE_NAME];
	int m_search_str_len;
} browser_t;

/* Change selection of an item */
#define fb_change_sel(fb, pos) (((fb)->m_files[pos].m_type & FB_ITEM_SEL) ? \
		((fb)->m_files[pos].m_type &= ~FB_ITEM_SEL) : \
		((fb)->m_files[pos].m_type |= FB_ITEM_SEL))
	
/* Create a new file browser window */
browser_t *fb_new( wnd_t *parent, char *dir );

/* Initialize file browser */
bool_t fb_construct( browser_t *fb, wnd_t *parent, char *dir );

/* Browser destructor */
void fb_destructor( wnd_t *wnd );

/* Display window */
wnd_msg_retcode_t fb_on_display( wnd_t *wnd );

/* Handle key pressing */
wnd_msg_retcode_t fb_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'action' message handler */
wnd_msg_retcode_t fb_on_action( wnd_t *wnd, char *action );

/* Handle mouse left button */
wnd_msg_retcode_t fb_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle mouse right button */
wnd_msg_retcode_t fb_on_mouse_rdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle mouse middle button */
wnd_msg_retcode_t fb_on_mouse_mdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle mouse left button double click */
wnd_msg_retcode_t fb_on_mouse_ldouble( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Move cursor to a specified position */
void fb_move_cursor( browser_t *fb, int pos, bool_t rel );

/* Reload directory files list */
void fb_load_files( browser_t *fb );

/* Free files list */
void fb_free_files( browser_t *fb );

/* Go to the directory under cursor */
void fb_go_to_dir( browser_t *fb );

/* Add selected files to play list */
void fb_add2plist( browser_t *fb );

/* Replace files in playlist with selected files  */
void fb_replace_plist( browser_t *fb );

/* Launch select/deselect files matching a pattern dialog */
void fb_select_pattern_dialog( browser_t *fb, bool_t sel );

/* Handle 'ok_clicked' for pattern dialog */
wnd_msg_retcode_t fb_on_sel_pattern( wnd_t *wnd );

/* Find a browser item by the mouse coordinates */
int fb_find_item_by_mouse( browser_t *fb, int x, int y );

/* Change directory */
void fb_change_dir( browser_t *fb, char *dir );

/* Toggle song info mode */
void fb_toggle_info( browser_t *fb );

/* Load song info */
void fb_load_info( browser_t *fb );

/* Print header */
void fb_print_header( browser_t *fb );

/* Print file */
void fb_print_file( browser_t *fb, struct browser_list_item *item );

/* Print info column */
void fb_print_info_col( browser_t *fb, int id, struct browser_list_item *item );

/* Show help screen */
void fb_help( browser_t *fb );

/* Launch change to directory dialog */
void fb_cd_dialog( browser_t *fb );

/* 'ok_clicked' handler for directory changing dialog */
wnd_msg_retcode_t fb_on_cd( wnd_t *wnd );

/* Initialize browser class */
wnd_class_t *fb_class_init( wnd_global_data_t *global );

/* Set browser class default styles */
void fb_class_set_default_styles( cfg_node_t *list );

#endif

/* End of 'browser.h' file */

