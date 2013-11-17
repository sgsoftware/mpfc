/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for printing functions.
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

#ifndef __SG_MPFC_WND_PRINT_H__
#define __SG_MPFC_WND_PRINT_H__

#include "types.h"

/* Forward declaration of window */
struct tag_wnd_t;

/* Possible moving cursor styles */
typedef enum
{
	/* Offsets are given from the start of client area */
	WND_MOVE_NORMAL = 0,	
	/* Offsets are given from the start of the window */
	WND_MOVE_ABSOLUTE = 1,	
	/* Offsets are given from the current position */	
	WND_MOVE_RELATIVE = 2,	
	/* Move addressing mask */
	WND_MOVE_ADRESS_MASK = 3,
	/* Advance to specified point (i.e. print spaces between current
	 * cursor position and destination) */
	WND_MOVE_ADVANCE = 1 << 2,
} wnd_move_style_t;

/* Value for coordinate passed to 'wnd_move'. Means that respective
 * coordinate shouldn't be changed */
#define WND_MOVE_DONT_CHANGE -1

/* Printing flags */
typedef enum
{
	/* Normal printing */
	WND_PRINT_NORMAL 		= 0,
	/* Also print in non-client area */
	WND_PRINT_NONCLIENT		= 1 << 0,	
	/* Print ellipses if string doesn't fit in the area */
	WND_PRINT_ELLIPSES		= 1 << 1,
	/* Don't clip string and continue printing on the next line */
	WND_PRINT_NOCLIP		= 1 << 2,
} wnd_print_flags_t;

/* Color values */
typedef enum
{
	WND_COLOR_WHITE = 0,
	WND_COLOR_BLACK,
	WND_COLOR_RED,
	WND_COLOR_GREEN,
	WND_COLOR_BLUE,
	WND_COLOR_YELLOW,
	WND_COLOR_MAGENTA,
	WND_COLOR_CYAN,
	WND_COLOR_NUMBER
} wnd_color_t;

/* Symbol attributes */
#define WND_ATTRIB_NORMAL		A_NORMAL
#define WND_ATTRIB_STANDOUT		A_STANDOUT
#define WND_ATTRIB_UNDERLINE	A_UNDERLINE
#define WND_ATTRIB_REVERSE		A_REVERSE
#define WND_ATTRIB_BLINK		A_BLINK
#define WND_ATTRIB_DIM			A_DIM
#define WND_ATTRIB_BOLD			A_BOLD
#define WND_ATTRIB_PROTECT		A_PROTECT
#define WND_ATTRIB_INVIS		A_INVIS
#define WND_ATTRIB_ALTCHARSET	A_ALTCHARSET

/* Set window current color and symbol attributes */
#define wnd_set_color(wnd, fg, bg) ((wnd)->m_fg_color = (fg), \
									(wnd)->m_bg_color = (bg))
#define wnd_set_fg_color(wnd, fg)  ((wnd)->m_fg_color = (fg))
#define wnd_set_bg_color(wnd, bg)  ((wnd)->m_bg_color = (bg))
#define wnd_set_attrib(wnd, attr)  ((wnd)->m_attrib = attr)
#define wnd_attr_on(wnd, attr)     ((wnd)->m_attrib |= (attr))
#define wnd_attr_off(wnd, attr)    ((wnd)->m_attrib &= ~(attr))

/* Move cursor to a specified position */
void wnd_move( struct tag_wnd_t *wnd, wnd_move_style_t style, int x, int y );

/* Low-level character printing */
void wnd_putc( struct tag_wnd_t *wnd, wchar_t ch );

/* Low-level special (ACS_*) character printing */
void wnd_put_special( struct tag_wnd_t *wnd, wchar_t ch );

/* Put a character */
void wnd_putchar( struct tag_wnd_t *wnd, wnd_print_flags_t flags, wchar_t ch );

/* Print a string */
void wnd_putstring( struct tag_wnd_t *wnd, wnd_print_flags_t flags,
		int right_border, char *str );

/* Print a formatted string */
int wnd_printf( struct tag_wnd_t *wnd, wnd_print_flags_t flags,
		int right_border, char *format, ... );

/* Check that cursor is inside client area */
bool_t wnd_cursor_in_client( struct tag_wnd_t *wnd );

/* Check that position inside the window is visible */
bool_t wnd_pos_visible( struct tag_wnd_t *wnd, int x, int y, 
		struct wnd_display_buf_symbol_t **pos );

#endif

/* End of 'wnd_print.h' file */

