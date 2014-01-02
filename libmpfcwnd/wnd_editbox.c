/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Edit box functions implementation.
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
#include <wctype.h>
#include "types.h"
#include "mystring.h"
#include "wnd.h"
#include "wnd_dlgitem.h"
#include "wnd_editbox.h"
#include "wnd_hbox.h"
#include "wnd_label.h"
#include "util.h"

/* Create a new edit box */
editbox_t *editbox_new( wnd_t *parent, char *id, char *text, char letter,
		int width )
{
	editbox_t *eb;
	wnd_class_t *klass;

	/* Allocate memory */
	eb = (editbox_t *)malloc(sizeof(*eb));
	if (eb == NULL)
		return NULL;
	memset(eb, 0, sizeof(*eb));

	/* Initialize class */
	klass = editbox_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
	{
		free(eb);
		return NULL;
	}
	WND_OBJ(eb)->m_class = klass;

	/* Initialize edit box */
	if (!editbox_construct(eb, parent, id, text, letter, width))
	{
		free(eb);
		return NULL;
	}
	WND_OBJ(eb)->m_flags |= WND_FLAG_INITIALIZED;
	return eb;
} /* End of 'editbox_new' function */

/* Edit box constructor */
bool_t editbox_construct( editbox_t *eb, wnd_t *parent, char *id, char *text,
		char letter, int width )
{
	wnd_t *wnd = WND_OBJ(eb);

	/* Initialize window part */
	if (!dlgitem_construct(DLGITEM_OBJ(eb), parent, "", id, 
				editbox_get_desired_size, NULL, letter, 0))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "display", editbox_on_display);
	wnd_msg_add_handler(wnd, "keydown", editbox_on_keydown);
	wnd_msg_add_handler(wnd, "action", editbox_on_action);
	wnd_msg_add_handler(wnd, "mouse_ldown", editbox_on_mouse);
	wnd_msg_add_handler(wnd, "destructor", editbox_destructor);

	/* Initialize edit box fields */
	eb->m_text = str_new(text == NULL ? "" : text);
	eb->m_width = width;
	eb->m_editable = TRUE;
	int len = str_width(eb->m_text);
	editbox_move(eb, len > eb->m_width ? 0 : len);
	eb->m_text_before_hist = str_new("");
	return TRUE;
} /* End of 'editbox_construct' function */

/* Create an edit box with label */
editbox_t *editbox_new_with_label( wnd_t *parent, char *title, char *id,
		char *text, char letter, int width )
{
	hbox_t *hbox;
	hbox = hbox_new(parent, NULL, 0);
	label_new(WND_OBJ(hbox), title, "", 0);
	return editbox_new(WND_OBJ(hbox), id, text, letter, width - utf8_width(title));
} /* End of 'editbox_new_with_label' function */

/* Destructor */
void editbox_destructor( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);
	assert(eb);

	/* Save text in history */
	if (eb->m_history != NULL)
	{
		if (eb->m_modified)
			editbox_history_add(eb->m_history, STR_TO_CPTR(eb->m_text));
		eb->m_history->m_cur = NULL;
	}

	/* Free memory */
	str_free(eb->m_text);
	str_free(eb->m_text_before_hist);
} /* End of 'editbox_destructor' function */

/* Get edit box desired size */
void editbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	editbox_t *eb = EDITBOX_OBJ(di);
	*width = eb->m_width;
	*height = 1;
} /* End of 'editbox_get_desired_size' function */

/* Set edit box text */
void editbox_set_text( editbox_t *eb, const char *text )
{
	assert(eb);
	str_copy_cptr(eb->m_text, text == NULL ? "" : text);
	int len = str_width(eb->m_text);
	eb->m_cursor = 0;
	eb->m_cursor_byte = 0;
	eb->m_scrolled = 0;
	eb->m_scrolled_byte = 0;
	editbox_move(eb, len > eb->m_width ? 0 : len);
	eb->m_modified = TRUE;
	wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
	wnd_invalidate(WND_OBJ(eb));
} /* End of 'editbox_set_text' function */

/* Add character into the current cursor position */
void editbox_addch( editbox_t *eb, char ch )
{
	assert(eb);
	int inserted_width = str_insert_char(eb->m_text, ch, eb->m_cursor_byte);
	if (inserted_width >= 0)
		/* If == 0, we still want to correct cursor_byte */
		editbox_move(eb, eb->m_cursor + inserted_width);
	eb->m_modified = TRUE;
} /* End of 'editbox_addch' function */

/* Delete character from the current or previous cursor position */
void editbox_delch( editbox_t *eb, bool_t before_cursor )
{
	assert(eb);

	if (before_cursor)
		if (!editbox_move(eb, eb->m_cursor - 1))
			return;

	str_delete_char(eb->m_text, eb->m_cursor_byte, FALSE);
	eb->m_modified = TRUE;
} /* End of 'editbox_delch' function */

/* Move cursor */
bool_t editbox_move( editbox_t *eb, int new_pos )
{
	/* Set cursor position */
	int old_cursor = eb->m_cursor;
	str_skip_positions(eb->m_text, &eb->m_cursor_byte, &eb->m_cursor,
			new_pos - eb->m_cursor);

	/* Handle scrolling */
	int new_scroll = eb->m_scrolled;
	while (eb->m_cursor < new_scroll + 1)
		new_scroll -= 5;
	while (eb->m_cursor >= new_scroll + eb->m_width)
		new_scroll++;
	str_skip_positions(eb->m_text, &eb->m_scrolled_byte, &eb->m_scrolled,
			new_scroll - eb->m_scrolled);

	return (eb->m_cursor != old_cursor);
} /* End of 'editbox_move' function */

/* 'display' message handler */
wnd_msg_retcode_t editbox_on_display( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	assert(wnd);

	/* Print text */
	wnd_move(wnd, 0, 0, 0);
	if (!eb->m_modified && eb->m_gray_non_modified)
		wnd_apply_style(wnd, "gray-style");
	else
		wnd_apply_default_style(wnd);
	wnd_printf(wnd, 0, 0, "%s", STR_TO_CPTR(eb->m_text) + eb->m_scrolled_byte);

	/* Move cursor */
	wnd_move(wnd, 0, eb->m_cursor - eb->m_scrolled, 0);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_display' function */

/* 'keydown' message handler */
wnd_msg_retcode_t editbox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	/* Append char to the text */
	if (key >= ' ' && key <= 0xFF && eb->m_editable)
	{
		editbox_addch(eb, key);
		eb->m_state_changed = TRUE;
		wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
		wnd_invalidate(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_keydown' function */

/* 'action' message handler */
wnd_msg_retcode_t editbox_on_action( wnd_t *wnd, char *action )
{
	bool_t not_changed = FALSE;
	editbox_t *eb = EDITBOX_OBJ(wnd);

	/* Delete previous char */
	if (!strcasecmp(action, "del_left"))
	{
		if (eb->m_editable)
		{
			editbox_delch(eb, TRUE);
			wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
		}
	}
	/* Delete current character */
	else if (!strcasecmp(action, "del_right"))
	{
		if (eb->m_editable)
		{
			editbox_delch(eb, FALSE);
			wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
		}
	}
	/* Kill text from current position to the start */
	else if (!strcasecmp(action, "kill_to_start"))
	{
		if (eb->m_editable)
		{
			while (eb->m_cursor != 0)
				editbox_delch(eb, TRUE);
			wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
		}
	}
	/* Kill text from current position to the end */
	else if (!strcasecmp(action, "kill_to_end"))
	{
		if (eb->m_editable)
		{
			while (STR_TO_CPTR(eb->m_text)[eb->m_cursor_byte])
				editbox_delch(eb, FALSE);
			wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
		}
	}
	/* Move cursor right */
	else if (!strcasecmp(action, "move_right"))
	{
		editbox_move(eb, eb->m_cursor + 1);
		wnd_msg_send(WND_OBJ(eb), "changed", editbox_changed_new());
	}
	/* Move cursor left */
	else if (!strcasecmp(action, "move_left"))
	{
		editbox_move(eb, eb->m_cursor - 1);
	}
	/* Move cursor to text start */
	else if (!strcasecmp(action, "move_to_start"))
	{
		editbox_move(eb, 0);
	}
	/* Move cursor one word left */
	else if (!strcasecmp(action, "word_left"))
	{
		bool_t moved = editbox_move(eb, eb->m_cursor - 1);
		while (moved)
		{
			wchar_t c = str_wchar_at(eb->m_text, eb->m_cursor_byte, NULL);
			if (!iswalnum(c))
				moved = editbox_move(eb, eb->m_cursor - 1);
			else break;
		}
		while (moved)
		{
			wchar_t c = str_wchar_at(eb->m_text, eb->m_cursor_byte, NULL);
			if (iswalnum(c))
				moved = editbox_move(eb, eb->m_cursor - 1);
			else break;
		}
		if (moved)
			editbox_move(eb, eb->m_cursor + 1);
	}
	/* Move cursor one word right */
	else if (!strcasecmp(action, "word_right"))
	{
		bool_t moved = TRUE;
		while (moved)
		{
			wchar_t c = str_wchar_at(eb->m_text, eb->m_cursor_byte, NULL);
			if (!iswalnum(c))
				moved = editbox_move(eb, eb->m_cursor + 1);
			else break;
		}
		while (moved)
		{
			wchar_t c = str_wchar_at(eb->m_text, eb->m_cursor_byte, NULL);
			if (iswalnum(c))
				moved = editbox_move(eb, eb->m_cursor + 1);
			else break;
		}
	}
	/* Move cursor to text end */
	else if (!strcasecmp(action, "move_to_end"))
	{
		editbox_move(eb, str_width(eb->m_text));
	}
	/* History stuff */
	else if (!strcasecmp(action, "history_prev"))
	{
		editbox_hist_move(eb, TRUE);
	}
	else if (!strcasecmp(action, "history_next"))
	{
		editbox_hist_move(eb, FALSE);
	}
	else
		not_changed = TRUE;
	if (!not_changed)
	{
		eb->m_state_changed = TRUE;
		wnd_invalidate(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_action' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t editbox_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	/* Move cursor to the respective position */
	editbox_move(eb, x - eb->m_scrolled);
	wnd_invalidate(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_mouse' function */

/* Initialize history list */
editbox_history_t *editbox_history_new( void )
{
	editbox_history_t *l;

	/* Allocate memory for list */
	l = (editbox_history_t *)malloc(sizeof(*l));
	if (l == NULL)
		return NULL;

	/* Set fields */
	l->m_head = l->m_tail = l->m_cur = NULL;
	return l;
} /* End of 'editbox_history_new' function */

/* Free history list */
void editbox_history_free( editbox_history_t *l )
{
	if (l == NULL)
		return;

	if (l->m_head != NULL)
	{
		struct editbox_history_item_t *t, *t1;
		
		for ( t = l->m_head; t != NULL; )
		{
			t1 = t->m_next;
			free(t->m_text);
			free(t);
			t = t1;
		}
	}
	free(l);
} /* End of 'editbox_history_free' function */

/* Add an item to history list */
void editbox_history_add( editbox_history_t *l, char *text )
{
	struct editbox_history_item_t *t;
	
	if (l == NULL)
		return;

	if (l->m_tail == NULL)
	{
		t = l->m_tail = l->m_head = 
			(struct editbox_history_item_t *)malloc(sizeof(*t));
		t->m_prev = NULL;
	}
	else
	{
		t = l->m_tail->m_next = 
			(struct editbox_history_item_t *)malloc(sizeof(*t));
		t->m_prev = l->m_tail;
	}
	t->m_next = NULL;
	t->m_text = strdup(text);
	l->m_tail = t;
} /* End of 'editbox_history_add' function */

/* Handle history moving */
void editbox_hist_move( editbox_t *eb, bool_t up )
{
	editbox_history_t *l;
	
	if ((l = eb->m_history) != NULL && l->m_tail != NULL)
	{
		if (up)
		{
			if (l->m_cur == NULL)
			{
				l->m_cur = l->m_tail;
				str_copy(eb->m_text_before_hist, eb->m_text);
			}
			else if (l->m_cur->m_prev != NULL)
				l->m_cur = l->m_cur->m_prev;
			else
				return;
		}
		else
		{
			if (l->m_cur == NULL)
				return;
			else
				l->m_cur = l->m_cur->m_next;
		}
		if (l->m_cur != NULL)
			editbox_set_text(eb, l->m_cur->m_text);
		else if (!up)
			editbox_set_text(eb, STR_TO_CPTR(eb->m_text_before_hist));
		eb->m_modified = FALSE;
		editbox_move(eb, str_width(eb->m_text));
	}
} /* End of 'editbox_hist_move' function */

/* Create edit box class */
wnd_class_t *editbox_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "editbox", 
			dlgitem_class_init(global), editbox_get_msg_info,
			editbox_free_handlers, editbox_class_set_default_styles);
	return klass;
} /* End of 'editbox_class_init' function */

/* Free message handlers */
void editbox_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(EDITBOX_OBJ(wnd)->m_on_changed);
} /* End of 'editbox_free_handlers' function */

/* Set edit box class default styles */
void editbox_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "gray-style", "black:black:bold");

	/* Set kbinds */
	cfg_set_var(list, "kbind.move_left", "<Left>;<Ctrl-b>");
	cfg_set_var(list, "kbind.move_right", "<Right>;<Ctrl-f>");
	cfg_set_var(list, "kbind.word_left", "<Alt-b>;<SLeft>");
	cfg_set_var(list, "kbind.word_right", "<Alt-f>;<SRight>");
	cfg_set_var(list, "kbind.del_left", "<Backspace>");
	cfg_set_var(list, "kbind.del_right", "<Del>;<Ctrl-d>");
	cfg_set_var(list, "kbind.kill_to_start", "<Ctrl-u>");
	cfg_set_var(list, "kbind.kill_to_end", "<Ctrl-k>");
	cfg_set_var(list, "kbind.move_to_start", "<Home>;<Ctrl-a>");
	cfg_set_var(list, "kbind.move_to_end", "<End>;<Ctrl-e>");
	cfg_set_var(list, "kbind.history_prev", "<Up>;<Ctrl-p>");
	cfg_set_var(list, "kbind.history_next", "<Down>;<Ctrl-n>");
} /* End of 'editbox_class_set_default_styles' function */

/* Get message information */
wnd_msg_handler_t **editbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "changed"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &EDITBOX_OBJ(wnd)->m_on_changed;
	}
	return NULL;
} /* End of 'editbox_get_msg_info' function */

/* End of 'wnd_editbox.c' file */

