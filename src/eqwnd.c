/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : eqwnd.c
 * PURPOSE     : SG MPFC. Equalizer window functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 12.07.2003
 * NOTE        : Module prefix 'eqwnd'.
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
#include "types.h"
#include "error.h"
#include "eqwnd.h"
#include "player.h"
#include "window.h"

/* Create a new equalizer window */
eq_wnd_t *eqwnd_new( wnd_t *parent, int x, int y, int w, int h )
{
	eq_wnd_t *eq;

	/* Allocate memory */
	eq = (eq_wnd_t *)malloc(sizeof(eq_wnd_t));
	if (eq == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize equalizer window */
	if (!eqwnd_init(eq, parent, x, y, w, h))
	{
		free(eq);
		return NULL;
	}
	return eq;
} /* End of 'eqwnd_new' function */

/* Initialize equalizer window */
bool eqwnd_init( eq_wnd_t *eq, wnd_t *parent, int x, int y, int w, int h )
{
	wnd_t *wnd = (wnd_t *)eq;

	/* Initialize window part */
	if (!wnd_init(wnd, parent, x, y, w, h))
		return FALSE;

	/* Register handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, eqwnd_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, eqwnd_handle_key);

	/* Set fields */
	eq->m_pos = 0;
	return TRUE;
} /* End of 'eqwnd_init' function */

/* Destroy equalizer window */
void eqwnd_free( wnd_t *wnd )
{
	wnd_destroy(wnd);
} /* End of 'eqwnd_free' function */

/* Handle display message */
void eqwnd_display( wnd_t *wnd, dword data )
{
	char title[80];
	eq_wnd_t *eq = (eq_wnd_t *)wnd;
	int i;
	int x;
	char *str[11] = {"PREAMP", "60HZ", "170HZ", "310HZ", "600HZ",
						"1KHZ", "3KHZ", "6KHZ", "12KHZ", "14KHZ", "16KHZ"};
	
	/* Print title */
	wnd_clear(wnd, FALSE);
	strcpy(title, _("MPFC Equalizer"));
	wnd_move(wnd, (wnd->m_width - strlen(title)) / 2, 0);
	wnd_set_attrib(wnd, A_BOLD);
	wnd_printf(wnd, "%s\n\n", title);
	wnd_set_attrib(wnd, A_NORMAL);

	/* Display bands sliders */
	for ( i = 0, x = 3; i < 11; i ++ )
	{
		float val = (i == 0) ? player_eq_preamp : player_eq_bands[i - 1];
		x += eqwnd_display_slider(wnd, x, 2, 22,
				(i == eq->m_pos), val, str[i]);
		if (i == 0)
		{
			wnd_move(wnd, x, 2);
			wnd_printf(wnd, "+20 dB");
			wnd_move(wnd, x, 12);
			wnd_printf(wnd, "0 dB");
			wnd_move(wnd, x, 22);
			wnd_printf(wnd, "-20 dB");
			x += 10;
		}
	}

	/* Remove cursor */
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'eqwnd_display' function */

/* Handle key message */
void eqwnd_handle_key( wnd_t *wnd, dword data )
{
	eq_wnd_t *eq = (eq_wnd_t *)wnd;
	int key = (int)data;

	switch (key)
	{
	case 'q':
	case 27:
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;
	case 'h':
	case KEY_LEFT:
		eq->m_pos --;
		if (eq->m_pos < 0)
			eq->m_pos = 10;
		break;
	case 'l':
	case KEY_RIGHT:
		eq->m_pos ++;
		if (eq->m_pos > 10)
			eq->m_pos = 0;
		break;
	case 'j':
	case KEY_DOWN:
		if (eq->m_pos == 0)
		{
			player_eq_preamp -= 2.;
			if (player_eq_preamp < -20.)
				player_eq_preamp = -20.;
		}
		else
		{
			player_eq_bands[eq->m_pos - 1] -= 2.;
			if (player_eq_bands[eq->m_pos - 1] < -20.)
				player_eq_bands[eq->m_pos - 1] = -20.;
		}
		player_eq_changed = TRUE;
		break;
	case 'k':
	case KEY_UP:
		if (eq->m_pos == 0)
		{
			player_eq_preamp += 2.;
			if (player_eq_preamp > 20.)
				player_eq_preamp = 20.;
		}
		else
		{
			player_eq_bands[eq->m_pos - 1] += 2.;
			if (player_eq_bands[eq->m_pos - 1] > 20.)
				player_eq_bands[eq->m_pos - 1] = 20.;
		}
		player_eq_changed = TRUE;
		break;
	}
} /* End of 'eqwnd_handle_key' function */

/* Display slider */
int eqwnd_display_slider( wnd_t *wnd, int x, int start_y, int end_y,
							bool hl, float val, char *str )
{
	int pos, i;
	int h;
	
	h = end_y - start_y;
	pos = ((-val) + 20.) * h / 40.;
	for ( i = 0; i <= h; i ++ )
	{
		wnd_move(wnd, x, i + start_y);
		if (i == pos)
		{
			wnd_print_char(wnd, ACS_BLOCK);
			wnd_print_char(wnd, ACS_BLOCK);
		}
		else
		{
			wnd_print_char(wnd, ACS_VLINE);
			wnd_print_char(wnd, ACS_VLINE);
		}
	}
	wnd_move(wnd, x - 1, end_y + 1);
	if (hl)
		wnd_set_attrib(wnd, A_BOLD);
	wnd_printf(wnd, "%s", str);
	wnd_set_attrib(wnd, A_NORMAL);
	return 6;
} /* End of 'eqwnd_display_slider' function */

/* End of 'eqwnd.c' file */

