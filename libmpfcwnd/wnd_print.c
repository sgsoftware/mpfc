/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Printing functions implementation.
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

#include <assert.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include "types.h"
#include "wnd.h"
#include "wnd_print.h"

/* Move cursor to a specified position */
void wnd_move( wnd_t *wnd, wnd_move_style_t style, int x, int y )
{
	int new_x, new_y;
	wnd_move_style_t addressing = style & WND_MOVE_ADRESS_MASK;
	
	assert(wnd);

	/* Calculate new cursor position */
	if (addressing == WND_MOVE_NORMAL)
	{
		new_x = x;
		new_y = y;
	}
	else if (addressing == WND_MOVE_ABSOLUTE)
	{
		new_x = x - wnd->m_client_x;
		new_y = y - wnd->m_client_y;
	}
	else
	{
		new_x = wnd->m_cursor_x + x;
		new_y = wnd->m_cursor_y + y;
	}
	if (x == WND_MOVE_DONT_CHANGE)
		new_x = wnd->m_cursor_x;
	if (y == WND_MOVE_DONT_CHANGE)
		new_y = wnd->m_cursor_y;

	/* Move */
	if (!(style & WND_MOVE_ADVANCE) || 
			new_y < wnd->m_cursor_y || 
			(new_y == wnd->m_cursor_y && new_x <= wnd->m_cursor_x))
		wnd->m_cursor_x = new_x, wnd->m_cursor_y = new_y;
	else
	{
		while (wnd->m_cursor_x != new_x || wnd->m_cursor_y != new_y)
			wnd_putchar(wnd, 
					(style & WND_MOVE_ABSOLUTE) ? WND_PRINT_NONCLIENT : 0, ' ');
	}
} /* End of 'wnd_move' function */

/* Low-level character printing */
void wnd_putc( wnd_t *wnd, dword ch )
{
	int cx, cy;
	struct wnd_display_buf_symbol_t *pos;

	/* This function doesn't print not-printable characters */
	if (ch < 0x20)
		return;

	/* Print character */
	if (wnd_pos_visible(wnd, wnd->m_cursor_x, wnd->m_cursor_y, &pos))
	{
		short fg = wnd->m_fg_color, bg = wnd->m_bg_color - 1;
		if (bg < 0)
			bg += WND_COLOR_NUMBER;
		pos->m_attr = wnd->m_attrib | 
			COLOR_PAIR(fg * WND_COLOR_NUMBER + bg);
		pos->m_char = ch;
	}

	/* Advance cursor */
	wnd->m_cursor_x ++;
} /* End of 'wnd_putc' function */

/* Put a character */
void wnd_putchar( wnd_t *wnd, wnd_print_flags_t flags, dword ch )
{
	/* Print special character */
	if (ch < 0x20)
	{
		switch (ch)
		{
		/* New line */
		case '\n':
			wnd_move(wnd, WND_MOVE_RELATIVE, 0, 1);
			wnd_move(wnd, (flags & WND_PRINT_NONCLIENT) ? WND_MOVE_ABSOLUTE :
					WND_MOVE_NORMAL, 0, WND_MOVE_DONT_CHANGE);
			break;
		/* Tabulation */
		case '\t':
			do
			{
				wnd_putchar(wnd, flags, ' ');
			} while (wnd->m_cursor_x % 8);
			break;
		}
		return;
	}

	/* Check cursor position */
	if (!(flags & WND_PRINT_NONCLIENT) && !wnd_cursor_in_client(wnd))
	{
		wnd->m_cursor_x ++;
		return;
	}
	wnd_putc(wnd, ch);
} /* End of 'wnd_putchar' function */

/* Print a string */
void wnd_putstring( wnd_t *wnd, wnd_print_flags_t flags, int right_border,
		char *str )
{
	mbstate_t state;

	assert(wnd);
	assert(str);

	/* Convert border to client coordinates */
	if (flags & WND_PRINT_NONCLIENT)
	{
		if (right_border <= 0 || right_border >= wnd->m_width)
			right_border = wnd->m_width - 1;
		right_border -= wnd->m_client_x;
	}
	/* Set border if it is not specified */
	else if (right_border <= 0 || right_border >= wnd->m_client_w)
		right_border = wnd->m_client_w - 1;

	/* Print string */
	memset(&state, 0, sizeof(state));
	mbsinit(&state);
	for ( ;; )
	{
		wchar_t ch;
		size_t nbytes;

		nbytes = mbrtowc(&ch, str, MB_CUR_MAX, &state);
		if ((size_t)(-1) == nbytes || (size_t)(-2) == nbytes)
		{
			ch = '?';
			nbytes = 1;
		}
		if (0 == nbytes)
			break;
		else
			str += nbytes;

		/* In case of new line - clear the rest of the current */
		if (ch == '\n')
		{
			while (wnd->m_cursor_x <= right_border)
				wnd_putchar(wnd, flags, ' ');
			wnd_putchar(wnd, flags, ch);
			continue;
		}
		
		/* Border is OK, so simply print this char */
		if (wnd->m_cursor_x <= right_border)
		{
			wnd_putchar(wnd, flags, ch);

			/* Move to the next line */
			if (wnd->m_cursor_x > right_border && (flags & WND_PRINT_NOCLIP))
			{
				wnd_move(wnd, 0, 0, wnd->m_cursor_y + 1);
			}
			continue;
		}

		/* If the very next char is new line, that's OK */
		if (*str == '\n')
			continue;

		/* Put ellipses */
		if (flags & WND_PRINT_ELLIPSES)
		{
			wnd_move(wnd, WND_MOVE_NORMAL, right_border - 2, 
					wnd->m_cursor_y);
			wnd_putchar(wnd, flags, '.');
			wnd_putchar(wnd, flags, '.');
			wnd_putchar(wnd, flags, '.');
		}

		/* Skip to the end of this line */
		bool_t finished = FALSE;
		for ( ;; )
		{
			nbytes = mbrtowc(&ch, str, MB_CUR_MAX, &state);
			if ((size_t)(-1) == nbytes || (size_t)(-2) == nbytes)
			{
				nbytes = 1;
				ch = '?';
			}
			if (0 == nbytes)
			{
				finished = TRUE;
				break;
			}
			else 
				str += nbytes;

			if ('\n' == ch)
				break;
		}
		if (finished)
			break;
		else
			wnd_putchar(wnd, flags, ch);
	}
} /* End of 'wnd_putstring' function */

/* Print a formatted string */
int wnd_printf( wnd_t *wnd, wnd_print_flags_t flags, int right_border,
		char *format, ... )
{
	char *str;
	int size = 100, n;

	assert(wnd);
	assert(format);

	/* Format the string */
	str = (char *)malloc(size);
	if (str == NULL)
		return 0;
	for ( ;; )
	{
		va_list ap;

		/* Try using current size */
		va_start(ap, format);
		n = vsnprintf(str, size, format, ap);
		va_end(ap);
		if (n > -1 && n < size)
			break;

		/* Try to allocate string of greater size */
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
		str = (char *)realloc(str, size);
		if (str == NULL)
			return 0;
	}

	/* Now print this string */
	wnd_putstring(wnd, flags, right_border, str);
	free(str);
	return n;
} /* End of 'wnd_printf' function */

/* Check that cursor is inside client area */
bool_t wnd_cursor_in_client( wnd_t *wnd )
{
	return (wnd->m_cursor_x >= 0 && wnd->m_cursor_x < wnd->m_client_w &&
			wnd->m_cursor_y >= 0 && wnd->m_cursor_y < wnd->m_client_h);
} /* End of 'wnd_cursor_in_client' function */

/* Check that position inside the window is visible */
bool_t wnd_pos_visible( wnd_t *wnd, int x, int y, 
		struct wnd_display_buf_symbol_t **pos )
{
	int scrx, scry;
	struct wnd_display_buf_t *db = &WND_DISPLAY_BUF(wnd);
	static int was_width = 0, was_x = 0, was_y = 0, was_index = 0;
	int index;

	/* Check that this position belongs to the window */
	scrx = WND_CLIENT2SCREEN_X(wnd, x);
	scry = WND_CLIENT2SCREEN_Y(wnd, y);
	if (scrx < 0 || scry < 0 || scrx >= COLS || scry >= LINES)
		return FALSE;

	/* Some optimization: if 'y' is the same as the last time, we
	 * should not calculate address in such a complicated way */
	if (scry == was_y && db->m_width == was_width)
		index = was_index + (scrx - was_x);
	else
		index = scry * db->m_width + scrx;

	/* Save some values for use in future calls */
	was_width = db->m_width;
	was_x = scrx;
	was_y = scry;
	was_index = index;

	/* Check that this position is not covered by other windows */
	*pos = &db->m_data[index];
	return (*pos)->m_wnd == wnd;
} /* End of 'wnd_pos_visible' function */

/* End of 'wnd_print.c' file */

