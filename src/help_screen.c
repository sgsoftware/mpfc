/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Help screen functions implementation.
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
#include <string.h>
#include "types.h"
#include "help_screen.h"
#include "wnd.h"

/* Calculate the screen size */
#define HELP_SCREEN_SIZE(wnd)	(WND_HEIGHT(wnd) - 2)

/* Create new help screen */
help_screen_t *help_new( wnd_t *parent, int type )
{
	help_screen_t *help;

	/* Allocate memory */
	help = (help_screen_t *)malloc(sizeof(help_screen_t));
	if (help == NULL)
		return NULL;
	memset(help, 0, sizeof(*help));
	WND_OBJ(help)->m_class = help_class_init(WND_GLOBAL(parent));

	/* Initialize help screen */
	if (!help_construct(help, parent, type))
	{
		free(help);
		return NULL;
	}
	wnd_postinit(help);
	return help;
} /* End of 'help_new' function */

/* Initialize help screen */
bool_t help_construct( help_screen_t *help, wnd_t *parent, int type )
{
	wnd_t *wnd = (wnd_t *)help;
	
	/* Initialize window part */
	if (!wnd_construct(wnd, parent, type == HELP_PLAYER ? 
				_("MPFC Default Key Bindings") : _("File Browser Key Bindings"), 
				0, 0, 0, 0, WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED))
		return FALSE;

	/* Register handlers */
	wnd_msg_add_handler(wnd, "display", help_on_display);
	wnd_msg_add_handler(wnd, "action", help_on_action);
	wnd_msg_add_handler(wnd, "destructor", help_destructor);

	/* Set fields */
	help->m_screen = 0;
	help->m_num_items = 0;
	help->m_items = NULL;
	wnd->m_cursor_hidden = TRUE;

	/* Initialize items */
	switch (type)
	{
	case HELP_PLAYER:
		help_init_player(help);
		break;
	case HELP_BROWSER:
		help_init_browser(help);
		break;
	}
	help->m_type = type;
	return TRUE;
} /* End of 'help_init' function */

/* Help screen destructor */
void help_destructor( wnd_t *wnd )
{
	help_screen_t *h = (help_screen_t *)wnd;
	int i;

	assert(h);
	
	if (h->m_items != NULL)
	{
		for ( i = 0; i < h->m_num_items; i ++ )
			free(h->m_items[i]);	
		free(h->m_items);
	}
} /* End of 'help_free' function */

/* Handle display message */
wnd_msg_retcode_t help_on_display( wnd_t *wnd )
{
	help_screen_t *h = (help_screen_t *)wnd;
	int i, j;
	
	/* Print keys */
	wnd_apply_style(wnd, "item-style");
	for ( i = HELP_SCREEN_SIZE(h) * h->m_screen, j = 0; 
			i < h->m_num_items && i < HELP_SCREEN_SIZE(h) * (h->m_screen + 1);
	   		i ++, j ++ )  
	{
		wnd_move(wnd, 0, 0, j);
		wnd_printf(wnd, 0, 0, "%s", h->m_items[i]);
	}

	/* Print prompt */
	wnd_apply_style(wnd, "prompt-style");
	wnd_move(wnd, 0, 0, WND_HEIGHT(wnd) - 1);
	wnd_printf(wnd, 0, 0, 
			_("Press <Space> to see next screen and <q> to exit\n"));
	return WND_MSG_RETCODE_OK;
} /* End of 'help_display' function */

/* Handle 'action' message */
wnd_msg_retcode_t help_on_action( wnd_t *wnd, char *action )
{
	help_screen_t *h = (help_screen_t *)wnd;

	if (!strcasecmp(action, "quit"))
	{
		wnd_close(wnd);
	}
	else if (!strcasecmp(action, "next_page"))
	{
		h->m_screen ++;
		if ((h->m_screen) * HELP_SCREEN_SIZE(h) - 1 >= h->m_num_items)
			h->m_screen = 0;
		wnd_invalidate(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'help_handle_key' function */

/* Add item */
void help_add( help_screen_t *h, char *name )
{
	h->m_items = (char **)realloc(h->m_items, sizeof(char *) * 
			(h->m_num_items + 1));
	h->m_items[h->m_num_items ++] = strdup(name);
} /* End of 'help_add' function */

/* Initialize help screen in player mode */
void help_init_player( help_screen_t *help )
{
	help_add(help, _("q:\t\t Quit program"));
	help_add(help, _("j or <Down>:\t Move one line down"));
	help_add(help, _("k or <Up>:\t Move one line up"));
	help_add(help, _("u or <PgUp>:\t Move one screen up"));
	help_add(help, _("d or <PgDn>:\t Move one screen down"));
	help_add(help, _("gg:\t\t Move to play list begin"));
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
	help_add(help, _("<:\t\t Scroll song 10 seconds back"));
	help_add(help, _(">:\t\t Scroll song 10 seconds forth"));
	help_add(help, _("<Number>gt:\t Go to <Number>th second in song"));
	help_add(help, _("+:\t\t Increase volume"));
	help_add(help, _("-:\t\t Decrease volume"));
	help_add(help, _("]:\t\t Increase balance"));
	help_add(help, _("[:\t\t Decrease balance"));
	help_add(help, _("i:\t\t Song info editor"));
	help_add(help, _("a:\t\t Add songs"));
	help_add(help, _("B:\t\t Launch file browser"));
	help_add(help, _("A:\t\t Audio output setup"));
	help_add(help, _("r:\t\t Remove songs"));
	help_add(help, _("s:\t\t Save play list"));
	help_add(help, _("S:\t\t Sort play list"));
	help_add(help, _("P:\t\t Launch plugins manager"));
	help_add(help, _("V:\t\t Start visual mode"));
	help_add(help, _("C:\t\t Centrize view"));
	help_add(help, _("/:\t\t Search"));
	help_add(help, _("\\:\t\t Advanced search"));
	help_add(help, _("n:\t\t Go to next search match"));
	help_add(help, _("N:\t\t Go to previous search match"));
	help_add(help, _("R:\t\t Set/unset shuffle play mode"));
	help_add(help, _("L:\t\t Set/unset loop play mode"));
	help_add(help, _("o:\t\t Variables manager"));
	help_add(help, _("O:\t\t Show logger window"));
	help_add(help, _("U:\t\t Undo"));
	help_add(help, _("D:\t\t Redo"));
	help_add(help, _("I:\t\t Reload songs information"));
	help_add(help, _("ps:\t\t Set play bounds"));
	help_add(help, _("p<Ret>:\t\t Set play bounds and play"));
	help_add(help, _("pc:\t\t Clear play bounds"));
	help_add(help, _("!:\t\t Execute command"));
	help_add(help, _("m<Letter>:\t Set mark <Letter>"));
	help_add(help, _("`<Letter>:\t Go to mark <Letter>"));
	help_add(help, _("``:\t\t Go to previous position"));
	help_add(help, _("<Backspace>:\t Go to previous time"));
	help_add(help, "");
	help_add(help, _("Window library bindings"));
	help_add(help, _("^l:\t\t Redisplay screen"));
	help_add(help, _("<Alt>-,:\t Set focus to previous window"));
	help_add(help, _("<Alt>-.:\t Set focus to next window"));
	help_add(help, _("<Alt>-c:\t Close window"));
	help_add(help, _("<Alt>-m:\t Maximize/minimize window"));
	help_add(help, _("<Alt>-p:\t Change window position"));
	help_add(help, _("<Alt>-s:\t Change window size"));
	help_add(help, _("<Alt>-<letter>:\t Set focus to a dialog item marked by"
				" letter"));
	help_add(help, _("<Ctrl>-g:\t Close dialog window"));
	help_add(help, "");
	help_add(help, _("?:\t\t This help screen"));
} /* End of 'help_init_player' function */

/* Initialize help screen in browser mode */
void help_init_browser( help_screen_t *help )
{
	help_add(help, _("q:\t\t Return to player"));
	help_add(help, _("j or <Down>:\t Move one line down"));
	help_add(help, _("k or <Up>:\t Move one line up"));
	help_add(help, _("u or <PgUp>:\t Move one screen up"));
	help_add(help, _("d or <PgDn>:\t Move one screen down"));
	help_add(help, _("<Ret>:\t\t Go to the highlighted directory"));
	help_add(help, _("h:\t\t Go to home directory"));
	help_add(help, _("<Backspace>:\t Move to parent directory"));
	help_add(help, _("<Insert>:\t Select/deselect file"));
	help_add(help, _("+:\t\t Select files matching a pattern"));
	help_add(help, _("-:\t\t Deselect files matching a pattern"));
	help_add(help, _("a:\t\t Add selected files to playlist"));
	help_add(help, _("r:\t\t Replace playlist files with the selected"));
	help_add(help, _("c:\t\t Change directory"));
	help_add(help, _("i:\t\t Toggle song info mode"));
	help_add(help, _("s:\t\t Toggle search mode"));
	help_add(help, _("?:\t\t This help screen"));
} /* End of 'help_init_browser' function */

/* Initialize help screen class */
wnd_class_t *help_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "help",
			wnd_basic_class_init(global), NULL, NULL,
			help_class_set_default_styles);
	return klass;
} /* End of 'help_class_init' function */

/* Set help screen class default styles */
void help_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "item-style", "white:black");
	cfg_set_var(list, "prompt-style", "red:black");
	cfg_set_var(list, "kbind.quit", "q;<Escape>");
	cfg_set_var(list, "kbind.next_page", "<Enter>;<Space>");
} /* End of 'help_class_set_default_styles' function */

/* End of 'help_screen.c' file */

