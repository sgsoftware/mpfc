/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : help_screen.c
 * PURPOSE     : SG MPFC. Help screen functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 16.08.2003
 * NOTE        : Module prefix 'help'.
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
#include <string.h>
#include "types.h"
#include "colors.h"
#include "error.h"
#include "help_screen.h"
#include "window.h"

/* Create new help screen */
help_screen_t *help_new( wnd_t *parent, int x, int y, int w, int h )
{
	help_screen_t *help;

	/* Allocate memory */
	help = (help_screen_t *)malloc(sizeof(help_screen_t));
	if (help == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize help screen */
	if (!help_init(help, parent, x, y, w, h))
	{
		free(help);
		return NULL;
	}
	return help;
} /* End of 'help_new' function */

/* Initialize help screen */
bool_t help_init( help_screen_t *help, wnd_t *parent, int x, int y, 
	   int w, int h )
{
	wnd_t *wnd = (wnd_t *)help;
	
	/* Initialize window part */
	if (!wnd_init(wnd, parent, x, y, w, h))
		return FALSE;

	/* Register handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, help_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, help_handle_key);

	/* Set fields */
	help->m_screen = 0;
	help->m_num_items = 0;
	help->m_screen_size = wnd->m_height - 4;
	help->m_num_screens = 0;
	wnd->m_wnd_destroy = help_free;

	/* Initialize items */
	help_add(help, _("q:\t\t Quit program"));
	help_add(help, _("j or <Down>:\t Move one line down"));
	help_add(help, _("k or <Up>:\t Move one line up"));
	help_add(help, _("u or <PgUp>:\t Move one screen up"));
	help_add(help, _("d or <PgDn>:\t Move one screen down"));
	help_add(help, _("G:\t\t Move to play list end"));
	help_add(help, _("<Number>G:\t Move to line <Number>"));
	help_add(help, _("J:\t\t Move play list one line down"));
	help_add(help, _("K:\t\t Move play list one line up"));
	help_add(help, _("<Number>M:\t Move play list to line <Number>"));
	help_add(help, _("<Ret>:\t\t Start playing"));
	help_add(help, _("x:\t\t Play stopped song or start playing from"
				" begin"));
	help_add(help, _("c:\t\t Pause/unpause"));
	help_add(help, _("v:\t\t Stop playing"));
	help_add(help, _("b:\t\t Next song"));
	help_add(help, _("z:\t\t Previous song"));
	help_add(help, _("<:\t\t Scroll song 10 seconds up"));
	help_add(help, _(">:\t\t Scroll song 10 seconds down"));
	help_add(help, _("<Number>t:\t Go to <Number>th second in song"));
	help_add(help, _("+:\t\t Increase volume"));
	help_add(help, _("-:\t\t Decrease volume"));
	help_add(help, _("]:\t\t Increase balance"));
	help_add(help, _("[:\t\t Decrease balance"));
	help_add(help, _("i:\t\t Song info editor"));
	help_add(help, _("a:\t\t Add songs"));
	help_add(help, _("A:\t\t Add object"));
	help_add(help, _("r:\t\t Remove songs"));
	help_add(help, _("s:\t\t Save play list"));
	help_add(help, _("S:\t\t Sort play list"));
	help_add(help, _("V:\t\t Start visual mode"));
	help_add(help, _("C:\t\t Centrize view"));
	help_add(help, _("/:\t\t Search"));
	help_add(help, _("\\:\t\t Advanced search"));
	help_add(help, _("n:\t\t Go to next search match"));
	help_add(help, _("N:\t\t Go to previous search match"));
	help_add(help, _("e:\t\t Launch equalizer window"));
	help_add(help, _("R:\t\t Set/unset shuffle play mode"));
	help_add(help, _("L:\t\t Set/unset loop play mode"));
	help_add(help, _("o:\t\t Variables mini-manager"));
	help_add(help, _("O:\t\t Advanced variables manager"));
	help_add(help, _("U:\t\t Undo"));
	help_add(help, _("R:\t\t Redo"));
	help_add(help, _("I:\t\t Reload songs information"));
	help_add(help, _("Ps:\t\t Set play bounds"));
	help_add(help, _("P<Ret>:\t\t Set play bounds and play"));
	help_add(help, _("Pc:\t\t Clear play bounds"));
	help_add(help, _("!:\t\t Execute command"));
	help_add(help, _("m<Letter>:\t Set mark <Letter>"));
	help_add(help, _("`<Letter>:\t Go to mark <Letter>"));
	help_add(help, _("``:\t\t Go to previous position"));
	help_add(help, _("<Backspace>:\t Go to previous time"));
	help_add(help, _("^l:\t\t Redisplay screen"));
	help_add(help, _("?:\t\t This help screen"));
	return TRUE;
} /* End of 'help_init' function */

/* Destroy help screen */
void help_free( wnd_t *wnd )
{
	wnd_destroy_func(wnd);
} /* End of 'help_free' function */

/* Handle display message */
void help_display( wnd_t *wnd, dword data )
{
	help_screen_t *h = (help_screen_t *)wnd;
	char title[80];
	int i;
	
	/* Print title */
	wnd_clear(wnd, FALSE);
	col_set_color(wnd, COL_EL_HELP_TITLE);
	strcpy(title, _("MPFC Default Key Bindings"));
	wnd_move(wnd, (wnd->m_width - strlen(title)) / 2, 0);
	wnd_printf(wnd, "%s\n\n", title);
	col_set_color(wnd, COL_EL_DEFAULT);

	/* Print keys */
	col_set_color(wnd, COL_EL_HELP_STRINGS);
	for ( i = h->m_screen_size * h->m_screen; 
			i < h->m_num_items && i < h->m_screen_size * (h->m_screen + 1);
	   		i ++ )  
		wnd_printf(wnd, "%s\n", h->m_items[i]);
	col_set_color(wnd, COL_EL_DEFAULT);

	wnd_move(wnd, 0, wnd->m_height - 1);
	col_set_color(wnd, COL_EL_STATUS);
	wnd_printf(wnd, "Press <Space> to see next screen and <q> to exit\n");
	col_set_color(wnd, COL_EL_DEFAULT);
	
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'help_display' function */

/* Handle key message */
void help_handle_key( wnd_t *wnd, dword data )
{
	int key = (int)data;
	help_screen_t *h = (help_screen_t *)wnd;

	switch (key)
	{
	case 'q':
	case 27:
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;
	case ' ':
	case '\n':
		if (h->m_num_screens)
		{
			h->m_screen ++;
			h->m_screen %= h->m_num_screens;
		}
		break;
	}
} /* End of 'help_handle_key' function */

/* Add item */
void help_add( help_screen_t *h, char *name )
{
	if (h->m_num_items >= HELP_MAX_ITEMS)
		return;

	strcpy(h->m_items[h->m_num_items ++], name);
	if ((h->m_num_items % h->m_screen_size) == 1)
		h->m_num_screens ++;
} /* End of 'help_add' function */

/* End of 'help_screen.c' file */

