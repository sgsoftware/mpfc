/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : colors.c
 * PURPOSE     : SG MPFC. User interface elements colors handling 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.08.2004
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

#include <string.h>
#include "types.h"
#include "cfg.h"
#include "colors.h"
#include "wnd.h"

/* Elements colors array */
colors_el_t col_elements[COL_NUM_ELEMENTS];

/* Initialize colors */
void col_init( void )
{
	/* Initialize colors with default values */
	col_set_el(COL_EL_PLIST_TITLE, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_PLIST_TITLE_CUR, WND_COLOR_RED, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_PLIST_TITLE_SEL, WND_COLOR_WHITE, WND_COLOR_BLUE, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_PLIST_TITLE_CUR_SEL, WND_COLOR_RED, WND_COLOR_BLUE, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_CUR_TITLE, WND_COLOR_YELLOW, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_CUR_TIME, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_AUDIO_PARAMS, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_PLIST_TIME, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_ABOUT, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_SLIDER, WND_COLOR_CYAN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_PLAY_MODES, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_STATUS, WND_COLOR_RED, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_HELP_TITLE, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_HELP_STRINGS, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_DLG_TITLE, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_DLG_ITEM_TITLE, WND_COLOR_WHITE, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_DLG_ITEM_CONTENT, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_DLG_ITEM_GRAYED, WND_COLOR_BLACK, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_LBOX_CUR_FOCUSED, WND_COLOR_WHITE, WND_COLOR_BLUE, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_LBOX_CUR, WND_COLOR_WHITE, WND_COLOR_GREEN, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_BTN_FOCUSED, WND_COLOR_WHITE, WND_COLOR_BLUE, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_BTN, WND_COLOR_WHITE, WND_COLOR_GREEN, WND_ATTRIB_BOLD);
	col_set_el(COL_EL_FB_FILE, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_FB_FILE_HL, WND_COLOR_WHITE, WND_COLOR_BLUE, 0);
	col_set_el(COL_EL_FB_DIR, WND_COLOR_GREEN, WND_COLOR_BLACK, 0);
	col_set_el(COL_EL_FB_DIR_HL, WND_COLOR_GREEN, WND_COLOR_BLUE, 0);
	col_set_el(COL_EL_FB_SEL, WND_COLOR_YELLOW, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_FB_SEL_HL, WND_COLOR_YELLOW, WND_COLOR_BLUE, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_FB_TITLE, WND_COLOR_GREEN, WND_COLOR_BLACK, 
			WND_ATTRIB_BOLD);
	col_set_el(COL_EL_DEFAULT, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);

	/* Load configuration */
	col_load_cfg();
} /* End of 'col_init' function */

/* Set one element color */
void col_set_el( int el, int fg, int bg, int attr )
{
	if (el >= COL_NUM_ELEMENTS)
		return;

	col_elements[el].m_fg = fg;
	col_elements[el].m_bg = bg;
	col_elements[el].m_attr = attr;
} /* End of 'col_set_el' function */

/* Set color */
void col_set_color( wnd_t *wnd, int el )
{
	wnd_set_attrib(wnd, col_elements[el].m_attr);
	wnd_set_fg_color(wnd, col_elements[el].m_fg);
	wnd_set_bg_color(wnd, col_elements[el].m_bg);
} /* End of 'col_set_color' function */

/* Load colors from configuration */
void col_load_cfg( void )
{
#if 0
	int i;

	/* Read all variables starting with 'col_el_' */
	for ( i = 0; i < cfg_list->m_num_vars; i ++ )
	{
		int el = col_var2el(cfg_list->m_vars[i].m_name);
		if (el >= 0)
			col_set_var(el, cfg_list->m_vars[i].m_val);
	}
#endif
} /* End of 'col_load_cfg' function */

/* Convert variable name to element ID */
int col_var2el( char *name )
{
	if (!strcmp(name, "col-el-plist-title"))
		return COL_EL_PLIST_TITLE;
	else if (!strcmp(name, "col-el-plist-title-cur"))
		return COL_EL_PLIST_TITLE_CUR;
	else if (!strcmp(name, "col-el-plist-title-sel"))
		return COL_EL_PLIST_TITLE_SEL;
	else if (!strcmp(name, "col-el-plist-title-cur-sel"))
		return COL_EL_PLIST_TITLE_CUR_SEL;
	else if (!strcmp(name, "col-el-cur-title"))
		return COL_EL_CUR_TITLE;
	else if (!strcmp(name, "col-el-cur-time"))
		return COL_EL_CUR_TIME;
	else if (!strcmp(name, "col-el-plist-time"))
		return COL_EL_PLIST_TIME;
	else if (!strcmp(name, "col-el-about"))
		return COL_EL_ABOUT;
	else if (!strcmp(name, "col-el-slider"))
		return COL_EL_SLIDER;
	else if (!strcmp(name, "col-el-play-modes"))
		return COL_EL_PLAY_MODES;
	else if (!strcmp(name, "col-el-status"))
		return COL_EL_STATUS;
	else if (!strcmp(name, "col-el-help-title"))
		return COL_EL_HELP_TITLE;
	else if (!strcmp(name, "col-el-help-strings"))
		return COL_EL_HELP_STRINGS;
	else if (!strcmp(name, "col-el-dlg-title"))
		return COL_EL_DLG_TITLE;
	else if (!strcmp(name, "col-el-dlg-item-title"))
		return COL_EL_DLG_ITEM_TITLE;
	else if (!strcmp(name, "col-el-dlg-item-content"))
		return COL_EL_DLG_ITEM_CONTENT;
	else if (!strcmp(name, "col-el-lbox-cur-focused"))
		return COL_EL_LBOX_CUR_FOCUSED;
	else if (!strcmp(name, "col-el-lbox-cur"))
		return COL_EL_LBOX_CUR;
	else if (!strcmp(name, "col-el-btn-focused"))
		return COL_EL_BTN_FOCUSED;
	else if (!strcmp(name, "col-el-btn"))
		return COL_EL_BTN;
	else if (!strcmp(name, "col-el-fb-title"))
		return COL_EL_FB_TITLE;
	else if (!strcmp(name, "col-el-fb-file"))
		return COL_EL_FB_FILE;
	else if (!strcmp(name, "col-el-fb-file-hl"))
		return COL_EL_FB_FILE_HL;
	else if (!strcmp(name, "col-el-fb-dir"))
		return COL_EL_FB_DIR;
	else if (!strcmp(name, "col-el-fb-dir-hl"))
		return COL_EL_FB_DIR_HL;
	else if (!strcmp(name, "col-el-fb-sel"))
		return COL_EL_FB_SEL;
	else if (!strcmp(name, "col-el-fb-sel-hl"))
		return COL_EL_FB_SEL_HL;
	else if (!strcmp(name, "col-el-default"))
		return COL_EL_DEFAULT;
	return -1;
} /* End of 'col_var2el' function */

/* Set color for element from variable */
void col_set_var( int el, char *val )
{
	int fg, bg, attr, i, j, k;
	char str[80];
	
	if (el < 0 || el >= COL_NUM_ELEMENTS)
		return;

	for ( i = 0, j = 0, k = 0, fg = WND_COLOR_WHITE, 
			bg = WND_COLOR_BLACK, attr = 0;; i ++ )
	{
		if (val[i] == ':' || val[i] == '\0')
		{
			str[j] = 0;
			if (k == 0 || k == 1)
			{
				int col;
				if (!strcmp(str, "black"))
					col = WND_COLOR_BLACK;
				else if (!strcmp(str, "red"))
					col = WND_COLOR_RED;
				else if (!strcmp(str, "green"))
					col = WND_COLOR_GREEN;
				else if (!strcmp(str, "yellow"))
					col = WND_COLOR_YELLOW;
				else if (!strcmp(str, "blue"))
					col = WND_COLOR_BLUE;
				else if (!strcmp(str, "magenta"))
					col = WND_COLOR_MAGENTA;
				else if (!strcmp(str, "cyan"))
					col = WND_COLOR_CYAN;
				else 
					col = WND_COLOR_WHITE;
				(k == 0) ? (fg = col) : (bg = col);
			}
			else if (k == 2)
			{
				if (!strcmp(str, "bold"))
					attr = WND_ATTRIB_BOLD;
				else
					attr = 0;
			}
			else
				break;

			if (!val[i])
				break;
			k ++;
			j = 0;
		}
		else
			str[j ++] = val[i];
	}

	/* Set value */
	col_set_el(el, fg, bg, attr);
} /* End of 'col_set_var' function */

/* End of 'colors.c' file */

