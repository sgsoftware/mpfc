/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : colors.h
 * PURPOSE     : SG MPFC. Interface for user interface elements 
 *               colors handling functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 20.01.2004
 * NOTE        : Module prefix 'colors'.
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

#ifndef __SG_MPFC_COLORS_H__
#define __SG_MPFC_COLORS_H__

#include "types.h"
#include "window.h"

/* Element color type */
typedef struct tag_colors_el_t
{ 
	int m_fg, m_bg;
	int m_attr;
	int m_pair;
} colors_el_t;

/* Elements */
#define COL_EL_PLIST_TITLE			0
#define COL_EL_PLIST_TITLE_CUR		1
#define COL_EL_PLIST_TITLE_SEL		2
#define COL_EL_PLIST_TITLE_CUR_SEL	3
#define COL_EL_CUR_TITLE			4
#define COL_EL_CUR_TIME				5
#define COL_EL_PLIST_TIME			6
#define COL_EL_ABOUT				7
#define COL_EL_SLIDER				8
#define COL_EL_PLAY_MODES			9
#define COL_EL_STATUS				10
#define COL_EL_HELP_TITLE			11
#define COL_EL_HELP_STRINGS			12
#define COL_EL_DLG_TITLE			13
#define COL_EL_DLG_ITEM_TITLE		14
#define COL_EL_DLG_ITEM_CONTENT		15
#define COL_EL_LBOX_CUR				16
#define COL_EL_LBOX_CUR_FOCUSED		17
#define COL_EL_BTN_FOCUSED			18
#define COL_EL_BTN					19
#define COL_EL_DLG_ITEM_GRAYED		20
#define COL_EL_FB_FILE				21
#define COL_EL_FB_FILE_HL			22
#define COL_EL_FB_DIR				23
#define COL_EL_FB_DIR_HL			24
#define COL_EL_FB_SEL				25
#define COL_EL_FB_SEL_HL			26
#define COL_EL_FB_TITLE				27
#define COL_EL_AUDIO_PARAMS			28
#define COL_EL_DEFAULT				29
#define COL_NUM_ELEMENTS			(COL_EL_DEFAULT+1)

/* Initialize colors */
void col_init( void );

/* Set one element color */
void col_set_el( int el, int fg, int bg, int attr );

/* Set color */
void col_set_color( wnd_t *wnd, int el );

/* Load colors from configuration */
void col_load_cfg( void );

/* Convert variable name to element ID */
int col_var2el( char *name );

/* Set color for element from variable */
void col_set_var( int el, char *val );

#endif

/* End of 'colors.h' file */

