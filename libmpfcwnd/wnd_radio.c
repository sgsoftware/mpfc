/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Radio button functions implementation.
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
#include "wnd_dlgitem.h"
#include "wnd_radio.h"

/* Create a new radio button */
radio_t *radio_new( wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked )
{
	radio_t *r;

	/* Allocate memory */
	r = (radio_t *)malloc(sizeof(*r));
	if (r == NULL)
		return NULL;
	memset(r, 0, sizeof(*r));
	WND_OBJ(r)->m_class = radio_class_init(WND_GLOBAL(parent));

	/* Initialize radio button */
	if (!radio_construct(r, parent, title, id, letter, checked))
	{
		free(r);
		return NULL;
	}
	WND_FLAGS(r) |= WND_FLAG_INITIALIZED;
	return r;
} /* End of 'radio_new' function */

/* Destructor */
static void radio_destructor( wnd_t *wnd )
{
	label_text_free(&RADIO_OBJ(wnd)->m_text);
} /* End of 'radio_destructor' function */

/* Radio button constructor */
bool_t radio_construct( radio_t *r, wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked )
{
	label_text_parse(&r->m_text, title);

	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(r), parent, title, id, 
				radio_get_desired_size, NULL, letter, 0))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(r), "action", radio_on_action);
	wnd_msg_add_handler(WND_OBJ(r), "display", radio_on_display);
	wnd_msg_add_handler(WND_OBJ(r), "mouse_ldown", radio_on_mouse);
	wnd_msg_add_handler(WND_OBJ(r), "quick_change_focus", 
			radio_on_quick_change_focus);
	wnd_msg_add_handler(WND_OBJ(r), "destructor", radio_destructor);

	/* Set fields */
	r->m_checked = checked;
	return TRUE;
} /* End of 'radio_construct' function */

/* 'action' message handler */
wnd_msg_retcode_t radio_on_action( wnd_t *wnd, char *action )
{
	/* Check radio button */
	if (!strcasecmp(action, "check"))
	{
		radio_check(RADIO_OBJ(wnd));
	}
	/* Move to the next item */
	else if (!strcasecmp(action, "move_to_prev"))
	{
		wnd_prev_focus(wnd->m_parent);
	}
	/* Move to the previous item */
	else if (!strcasecmp(action, "move_to_next"))
	{
		wnd_next_focus(wnd->m_parent);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_action' function */

/* 'display' message handler */
wnd_msg_retcode_t radio_on_display( wnd_t *wnd )
{
	radio_t *r = RADIO_OBJ(wnd);

	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	wnd_printf(wnd, 0, 0, "(%c)", r->m_checked ? 'X' : ' ');
	wnd_apply_style(wnd, WND_FOCUS(wnd) == wnd ? "focus-label-style" :
			"label-style");
	wnd_putchar(wnd, 0, ' ');
	label_text_display(wnd, &RADIO_OBJ(wnd)->m_text);
	wnd_move(wnd, 0, 1, 0);
    return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_display' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t radio_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	radio_check(RADIO_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_mouse' function */

/* 'quick_change_focus' message handler */
wnd_msg_retcode_t radio_on_quick_change_focus( wnd_t *wnd )
{
	radio_check(RADIO_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_quick_change_focus' function */

/* Set checked state */
void radio_check( radio_t *r )
{
	wnd_t *wnd = WND_OBJ(r);

	/* Uncheck all the siblings */
	wnd_t *sibling;
	for ( sibling = wnd->m_parent->m_child; sibling != NULL; 
			sibling = sibling->m_next )
		RADIO_OBJ(sibling)->m_checked = FALSE;

	/* Check ourselves */
	r->m_checked = TRUE;
	wnd_invalidate(wnd->m_parent);
} /* End of 'radio_check' function */

/* Get size desired by check box */
void radio_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	*width = RADIO_OBJ(di)->m_text.width + 5;
	*height = 1;
} /* End of 'radio_get_desired_size' function */

/* Initialize radio button class */
wnd_class_t *radio_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "radio", 
			dlgitem_class_init(global), NULL, NULL, 
			radio_class_set_default_styles);
	return klass;
} /* End of 'radio_class_init' function */

/* Set radio button class default styles */
void radio_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "focus-text-style", "white:blue:bold");
	cfg_set_var(list, "label-style", "white:black");
	cfg_set_var(list, "focus-label-style", "white:black");

	/* Set kbinds */
	cfg_set_var(list, "kbind.check", "<Space>");
	cfg_set_var(list, "kbind.move_to_next", "<Down>;<Ctrl-n>;j");
	cfg_set_var(list, "kbind.move_to_prev", "<Up>;<Ctrl-p>;k");
} /* End of 'radio_class_set_default_styles' function */

/* End of 'wnd_radio.c' file */

