/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Combo box functions implementation.
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
#include "wnd.h"
#include "wnd_combobox.h"
#include "wnd_dlgitem.h"
#include "wnd_editbox.h"
#include "wnd_hbox.h"
#include "wnd_label.h"
#include "util.h"

/* Create a new combo box */
combo_t *combo_new( wnd_t *parent, char *id, char *text, char letter, 
		int width, int height )
{
	combo_t *combo;

	/* Allocate memory */
	combo = (combo_t *)malloc(sizeof(*combo));
	if (combo == NULL)
		return NULL;
	memset(combo, 0, sizeof(*combo));
	WND_OBJ(combo)->m_class = combo_class_init(WND_GLOBAL(parent));

	/* Initialize combo box */
	if (!combo_construct(combo, parent, id, text, letter, width, height))
	{
		free(combo);
		return NULL;
	}
	WND_FLAGS(combo) |= WND_FLAG_INITIALIZED;
	return combo;
} /* End of 'combo_new' function */

/* Create a new combo box with a label */
combo_t *combo_new_with_label( wnd_t *parent, char *title, 
		char *id, char *text, char letter, int width, int height )
{
	hbox_t *hbox = hbox_new(parent, NULL, 0);
	label_new(WND_OBJ(hbox), title, "", 0);
	return combo_new(WND_OBJ(hbox), id, text, letter, width - utf8_width(title), 
			height);
} /* End of 'combo_new_with_label' function */

/* Combo box constructor */
bool_t combo_construct( combo_t *combo, wnd_t *parent, char *id, 
		char *text, char letter, int width, int height )
{
	/* Initialize edit box part */
	if (!editbox_construct(EDITBOX_OBJ(combo), parent, id, text, letter, width))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(combo), "action", combo_on_action);
	wnd_msg_add_handler(WND_OBJ(combo), "display", combo_on_display);
	wnd_msg_add_handler(WND_OBJ(combo), "mouse_ldown", combo_on_mouse);
	wnd_msg_add_handler(WND_OBJ(combo), "changed", combo_on_changed);
	wnd_msg_add_handler(WND_OBJ(combo), "loose_focus", combo_on_loose_focus);
	wnd_msg_add_handler(WND_OBJ(combo), "destructor", combo_destructor);

	/* Set combo box fields */
	combo->m_height = height;
	//combo_add_item(combo, "");
	return TRUE;
} /* End of 'combo_construct' function */

/* Destructor */
void combo_destructor( wnd_t *wnd )
{
	combo_t *combo = COMBO_OBJ(wnd);
	if (combo->m_list != NULL)
	{
		for ( ; combo->m_list_size > 0; combo->m_list_size -- )
			free(combo->m_list[combo->m_list_size - 1]);
		free(combo->m_list);
	}
} /* End of 'combo_destructor' function */

/* Add an item to the list */
void combo_add_item( combo_t *combo, char *item )
{
	assert(combo);
	combo->m_list = (char **)realloc(combo->m_list, 
			sizeof(char *) * (combo->m_list_size + 1));
	assert(combo->m_list);
	combo->m_list[combo->m_list_size ++] = strdup(item);
} /* End of 'combo_add_item' function */

/* St new cursor position in the list */
void combo_move_cursor( combo_t *combo, int pos, bool_t synchronize_text )
{
	int start, end;

	assert(combo);

	/* Set position */
	combo->m_cursor = pos;
	if (combo->m_cursor < 0)
		combo->m_cursor = 0;
	else if (combo->m_cursor >= combo->m_list_size)
		combo->m_cursor = combo->m_list_size - 1;

	/* Scroll the list */
	start = combo->m_scrolled;
	end = combo->m_scrolled + combo->m_height;
	if (combo->m_cursor < start)
		combo->m_scrolled = combo->m_cursor;
	else if (combo->m_cursor >= end)
		combo->m_scrolled += (combo->m_cursor - end + 1);

	/* Synchronize text */
	if (synchronize_text)
		editbox_set_text(EDITBOX_OBJ(combo), combo->m_list[combo->m_cursor]);
	wnd_invalidate(WND_OBJ(combo));
} /* End of 'combo_move_cursor' function */

/* 'action' message handler */
wnd_msg_retcode_t combo_on_action( wnd_t *wnd, char *action )
{
	combo_t *combo = COMBO_OBJ(wnd);

	/* Move cursor up */
	if (!strcasecmp(action, "move_up"))
	{
		combo_expand(combo);
		combo_move_cursor(combo, combo->m_cursor - 1, TRUE);
	}
	/* Move cursor down */
	else if (!strcasecmp(action, "move_down"))
	{
		combo_expand(combo);
		combo_move_cursor(combo, combo->m_cursor + 1, TRUE);
	}
	/* Move cursor one screen up */
	else if (!strcasecmp(action, "screen_up"))
	{
		combo_expand(combo);
		combo_move_cursor(combo, combo->m_cursor - combo->m_height, TRUE);
	}
	/* Move cursor one screen down */
	else if (!strcasecmp(action, "screen_down"))
	{
		combo_expand(combo);
		combo_move_cursor(combo, combo->m_cursor + combo->m_height, TRUE);
	}
	/* Toggle expansion mode */
	else if (!strcasecmp(action, "toggle_expansion"))
	{
		combo->m_expanded ? combo_unexpand(combo) : combo_expand(combo);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'combo_on_action' function */

/* 'display' message handler */
wnd_msg_retcode_t combo_on_display( wnd_t *wnd )
{
	combo_t *combo = COMBO_OBJ(wnd);

	/* Display list */
	if (combo->m_expanded)
	{
		int i, j;
		wnd_apply_default_style(wnd);
		for ( i = combo->m_scrolled, j = 1; 
				i < combo->m_list_size && j <= combo->m_height; j ++, i ++ )
		{
			wnd_move(wnd, 0, 1, j);
			wnd_push_state(wnd, WND_STATE_COLOR);
			if (i == combo->m_cursor)
			{
				wnd_apply_style(wnd, WND_FOCUS(wnd) == wnd ? 
						"focus-list-style" : "list-style");
			}
			wnd_putstring(wnd, 0, 0, combo->m_list[i]);
			wnd_pop_state(wnd);
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'combo_on_display' function */

/* 'changed' message handler */
wnd_msg_retcode_t combo_on_changed( wnd_t *wnd )
{
	/* Synchronize list with the text */
	combo_synch_list(COMBO_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'combo_on_changed' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t combo_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	return WND_MSG_RETCODE_OK;
} /* End of 'combo_on_mouse' function */

/* Expand combo box */
void combo_expand( combo_t *combo )
{
	wnd_t *wnd = WND_OBJ(combo);

	if (combo->m_expanded)
		return;

	combo->m_expanded = TRUE;

	/* Set respective window parameters */
	wnd->m_flags |= WND_FLAG_NOPARENTCLIP;
	wnd_repos(wnd, wnd->m_x, wnd->m_y, wnd->m_width, wnd->m_height + 
			combo->m_height);
} /* End of 'combo_expand' function */

/* Synchronize list cursor with text */
void combo_synch_list( combo_t *combo )
{
	char *text = EDITBOX_TEXT(combo);
	int len = EDITBOX_BYTE_LEN(combo);
	int pos = 0, i;

	/* Search for the most suitable item */
	for ( i = 0; i < combo->m_list_size; i ++ )
	{
		if (!strncmp(text, combo->m_list[i], len))
		{
			pos = i;
			break;
		}
	}

	/* Set it current */
	combo_move_cursor(combo, pos, FALSE);
} /* End of 'combo_synch_list' function */

/* 'loose_focus' message handler */
wnd_msg_retcode_t combo_on_loose_focus( wnd_t *wnd )
{
	combo_unexpand(COMBO_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'combo_on_loose_focus' function */

/* Unexpand combo box */
void combo_unexpand( combo_t *combo )
{
	wnd_t *wnd = WND_OBJ(combo);

	if (!combo->m_expanded)
		return;

	/* Unexpand */
	combo->m_expanded = FALSE;

	/* Set window parameters */
	wnd->m_flags &= (~WND_FLAG_NOPARENTCLIP);
	wnd_repos(wnd, wnd->m_x, wnd->m_y, wnd->m_width, 1);
	wnd_invalidate(WND_ROOT(wnd));
} /* End of 'combo_unexpand' function */

/* Initialize combo box class */
wnd_class_t *combo_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "combo", 
			editbox_class_init(global), NULL, NULL,
			combo_class_set_default_styles);
	return klass;
} /* End of 'combo_class_init' function */

/* Set combo box class default styles */
void combo_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "list-style", "white:green");
	cfg_set_var(list, "focus-list-style", "white:blue");

	/* Set kbinds */
	cfg_set_var(list, "kbind.move_down", "<Down>;<Ctrl-n>");
	cfg_set_var(list, "kbind.move_up", "<Up>;<Ctrl-p>");
	cfg_set_var(list, "kbind.screen_down", "<PageDown>;<Ctrl-v>");
	cfg_set_var(list, "kbind.screen_up", "<PageUp>;<Alt-v>");
	cfg_set_var(list, "kbind.toggle_expansion", "<Ctrl-e>");
} /* End of 'combo_class_set_default_styles' function */

/* End of 'wnd_combobox.c' file */

