/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Checkbox functions implementation.
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
#include "wnd_button.h"
#include "wnd_checkbox.h"
#include "wnd_dlgitem.h"

/* Create a new check box */
checkbox_t *checkbox_new( wnd_t *parent, char *title, char *id, 
		char letter, bool_t checked )
{
	checkbox_t *cb;

	/* Allocate memory */
	cb = (checkbox_t *)malloc(sizeof(*cb));
	if (cb == NULL)
		return NULL;
	memset(cb, 0, sizeof(*cb));
	WND_OBJ(cb)->m_class = checkbox_class_init(WND_GLOBAL(parent));

	/* Initialize check box */
	if (!checkbox_construct(cb, parent, title, id, letter, checked))
	{
		free(cb);
		return NULL;
	}
	WND_FLAGS(cb) |= WND_FLAG_INITIALIZED;
	return cb;
} /* End of 'checkbox_new' function */

/* Destructor */
static void checkbox_destructor( wnd_t *wnd )
{
	label_text_free(&CHECKBOX_OBJ(wnd)->m_text);
} /* End of 'checkbox_destructor' function */

/* Check box constructor */
bool_t checkbox_construct( checkbox_t *cb, wnd_t *parent, char *title, 
		char *id, char letter, bool_t checked )
{
	label_text_parse(&cb->m_text, title);

	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(cb), parent, title, id, 
				checkbox_get_desired_size, NULL, letter, 0))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(cb), "action", checkbox_on_action);
	wnd_msg_add_handler(WND_OBJ(cb), "display", checkbox_on_display);
	wnd_msg_add_handler(WND_OBJ(cb), "mouse_ldown", checkbox_on_mouse);
	wnd_msg_add_handler(WND_OBJ(cb), "quick_change_focus", 
			checkbox_on_quick_change_focus);
	wnd_msg_add_handler(WND_OBJ(cb), "destructor", checkbox_destructor);

	/* Set check box fields */
	cb->m_checked = checked;
	return TRUE;
} /* End of 'checkbox_construct' function */

/* 'action' message handler */
wnd_msg_retcode_t checkbox_on_action( wnd_t *wnd, char *action )
{
	checkbox_t *cb = CHECKBOX_OBJ(wnd);

	/* Change state */
	if (!strcasecmp(action, "toggle"))
	{
		checkbox_toggle(cb);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_action' function */

/* 'display' message handler */
wnd_msg_retcode_t checkbox_on_display( wnd_t *wnd )
{
	checkbox_t *cb = CHECKBOX_OBJ(wnd);

	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	wnd_printf(wnd, 0, 0, "[%c]", cb->m_checked ? 'X' : ' ');
	wnd_apply_style(wnd, WND_FOCUS(wnd) == wnd ? "focus-label-style" :
			"label-style");
	wnd_putchar(wnd, 0, ' ');
	label_text_display(wnd, &cb->m_text);
	wnd_move(wnd, 0, 1, 0);
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_display' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t checkbox_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	checkbox_toggle(CHECKBOX_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_mouse' function */

/* 'quick_change_focus' message handler */
wnd_msg_retcode_t checkbox_on_quick_change_focus( wnd_t *wnd )
{
	checkbox_toggle(CHECKBOX_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_quick_change_focus' function */

/* Toggle checked state */
void checkbox_toggle( checkbox_t *cb )
{
	cb->m_checked = !cb->m_checked;
	wnd_msg_send(WND_OBJ(cb), "clicked", button_msg_clicked_new());
	wnd_invalidate(WND_OBJ(cb));
} /* End of 'checkbox_toggle' function */

/* Get size desired by check box */
void checkbox_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	*width = CHECKBOX_OBJ(di)->m_text.width + 5;
	*height = 1;
} /* End of 'checkbox_get_desired_size' function */

/* Initialize checkbox class */
wnd_class_t *checkbox_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "checkbox", 
			dlgitem_class_init(global), checkbox_get_msg_info,
			checkbox_free_handlers, checkbox_class_set_default_styles);
	return klass;
} /* End of 'checkbox_class_init' function */

/* Set check box class default styles */
void checkbox_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "focus-text-style", "white:blue:bold");
	cfg_set_var(list, "label-style", "white:black");
	cfg_set_var(list, "focus-label-style", "white:black");
	cfg_set_var(list, "kbind.toggle", "<Space>");
} /* End of 'checkbox_class_set_default_styles' function */

/* Get message information */
wnd_msg_handler_t **checkbox_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "clicked"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &CHECKBOX_OBJ(wnd)->m_on_clicked;
	}
	return NULL;
} /* End of 'checkbox_get_msg_info' function */

/* Free message handlers */
void checkbox_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(CHECKBOX_OBJ(wnd)->m_on_clicked);
} /* End of 'checkbox_free_handlers' function */

/* End of 'wnd_checkbox.c' file */

