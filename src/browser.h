/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : browser.h
 * PURPOSE     : SG MPFC. Interface for file browser functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.02.2004
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

#ifndef __SG_MPFC_BROWSER_H__
#define __SG_MPFC_BROWSER_H__

#include "types.h"
#include "song_info.h"
#include "window.h"

/* File browser window type */
typedef struct tag_browser_t
{
	/* Common window part */
	wnd_t m_wnd;

	/* Current directory name */
	char m_cur_dir[MAX_FILE_NAME];

	/* Directory contents */
	struct browser_list_item
	{
		char *m_name;
		char *m_full_name;
		song_info_t *m_info;
		int m_len;
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

	/* List part height */
	int m_height;

	/* Is info mode active? */
	bool_t m_info_mode;
} browser_t;

/* Change selection of an item */
#define fb_change_sel(fb, pos) (((fb)->m_files[pos].m_type & FB_ITEM_SEL) ? \
		((fb)->m_files[pos].m_type &= ~FB_ITEM_SEL) : \
		((fb)->m_files[pos].m_type |= FB_ITEM_SEL))
	
/* Create a new file browser window */
browser_t *fb_new( wnd_t *parent, int x, int y, int w, int h, char *dir );

/* Initialize file browser */
bool_t fb_init( browser_t *fb, wnd_t *parent, int x, int y, int w, 
					int h, char *dir );

/* Destroy window */
void fb_free( wnd_t *wnd );

/* Display window */
void fb_display( wnd_t *wnd, dword data );

/* Handle key pressing */
void fb_handle_key( wnd_t *wnd, dword data );

/* Handle mouse left button */
void fb_handle_mouse( wnd_t *wnd, dword data );

/* Handle mouse right button */
void fb_handle_mouse_right( wnd_t *wnd, dword data );

/* Handle mouse middle button */
void fb_handle_mouse_mdl( wnd_t *wnd, dword data );

/* Handle mouse left button double click */
void fb_handle_mouse_dbl( wnd_t *wnd, dword data );

/* Move cursor to a specified position */
void fb_move_cursor( browser_t *fb, int pos, bool_t rel );

/* Reload directory files list */
void fb_load_files( browser_t *fb );

/* Add a file to list */
void fb_add_file( browser_t *fb, char *name );

/* Free files list */
void fb_free_files( browser_t *fb );

/* Go to the directory under cursor */
void fb_go_to_dir( browser_t *fb );

/* Add selected files to play list */
void fb_add2plist( browser_t *fb );

/* Select/deselect files matching a pattern */
void fb_select_pattern( browser_t *fb, bool_t sel );

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

#endif

/* End of 'browser.h' file */

