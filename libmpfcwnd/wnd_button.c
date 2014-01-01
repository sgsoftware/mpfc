/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Button functions implementation.
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
#include "wnd_dlgitem.h"
#include "wnd_label.h"

/* Create a new button */
button_t *button_new( wnd_t *parent, char *title, char *id, char letter )
{
	button_t *btn;
	wnd_class_t *klass;

	/* Allocate memory for button */
	btn = (button_t *)malloc(sizeof(*btn));
	if (btn == NULL)
		return NULL;
	memset(btn, 0, sizeof(*btn));

	/* Initialize button class */
	klass = button_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
	{
		free(btn);
		return NULL;
	}
	WND_OBJ(btn)->m_class = klass;

	/* Initialize button */
	if (!button_construct(btn, parent, title, id, letter))
	{
		free(btn);
		return NULL;
	}
	WND_OBJ(btn)->m_flags |= WND_FLAG_INITIALIZED;
	return btn;
} /* End of 'button_new' function */

/* Destructor */
static void button_destructor( wnd_t *wnd )
{
	label_text_free(&BUTTON_OBJ(wnd)->m_text);
} /* End of 'button_destructor' function */

/* Button initialization function */
bool_t button_construct( button_t *btn, wnd_t *parent, char *title, char *id,
		char letter )
{
	wnd_t *wnd = WND_OBJ(btn);

	assert(btn);

	label_text_parse(&btn->m_text, title);

	/* Initialize window part */
	if (!dlgitem_construct(DLGITEM_OBJ(btn), parent, title, id, 
				button_get_desired_size, NULL, letter, 0))
		return FALSE;

	/* Set message handlers */
	wnd_msg_add_handler(wnd, "display", button_on_display);
	wnd_msg_add_handler(wnd, "action", button_on_action);
	wnd_msg_add_handler(wnd, "mouse_ldown", button_on_mouse);
	wnd_msg_add_handler(wnd, "quick_change_focus", 
			button_on_quick_change_focus);
	wnd_msg_add_handler(wnd, "destructor", button_destructor);
	return TRUE;
} /* End of 'button_construct' function */

/* Get button desired size */
void button_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	*width = 2 + BUTTON_OBJ(di)->m_text.width;
	*height = 1;
} /* End of 'button_get_desired_size' function */

/* 'display' message handler */
wnd_msg_retcode_t button_on_display( wnd_t *wnd )
{
	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	wnd_putchar(wnd, 0, ' ');
	label_text_display(wnd, &BUTTON_OBJ(wnd)->m_text);
	wnd_putchar(wnd, 0, ' ');
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_display' function */

/* 'action' message handler */
wnd_msg_retcode_t button_on_action( wnd_t *wnd, char *action )
{
	/* Button clicked */
	if (!strcasecmp(action, "click"))
		wnd_msg_send(wnd, "clicked", button_msg_clicked_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_action' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t button_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	wnd_msg_send(wnd, "clicked", button_msg_clicked_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_mouse' function */

/* 'quick_change_focus' message handler */
wnd_msg_retcode_t button_on_quick_change_focus( wnd_t *wnd )
{
	wnd_msg_send(wnd, "clicked", button_msg_clicked_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_quick_change_focus' function */

/* Initialize button class */
wnd_class_t *button_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "button", 
			dlgitem_class_init(global), button_get_msg_info, 
			button_free_handlers, button_class_set_default_styles);
	return klass;
} /* End of 'button_class_init' function */

/* Set button class default styles */
void button_class_set_default_styles( cfg_node_t *node )
{
	cfg_set_var(node, "text-style", "white:green:bold");
	cfg_set_var(node, "focus-text-style", "white:blue:bold");
	cfg_set_var(node, "kbind.click", "<Space>");
} /* End of 'button_class_set_default_styles' function */

/* Get message information */
wnd_msg_handler_t **button_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "clicked"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &BUTTON_OBJ(wnd)->m_on_clicked;
	}
	return NULL;
} /* End of 'button_get_msg_info' function */

/* Free message handlers */
void button_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(BUTTON_OBJ(wnd)->m_on_clicked);
} /* End of 'button_free_handlers' function */

/* End of 'wnd_button.h' file */

