/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_checkbox.c
 * PURPOSE     : MPFC Window Library. Checkbox functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 26.08.2004
 * NOTE        : Module prefix 'checkbox'.
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

/* Check box constructor */
bool_t checkbox_construct( checkbox_t *cb, wnd_t *parent, char *title, 
		char *id, char letter, bool_t checked )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(cb), parent, title, id, 
				checkbox_get_desired_size, NULL, letter, 0))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(cb), "keydown", checkbox_on_keydown);
	wnd_msg_add_handler(WND_OBJ(cb), "display", checkbox_on_display);
	wnd_msg_add_handler(WND_OBJ(cb), "mouse_ldown", checkbox_on_mouse);

	/* Set check box fields */
	cb->m_checked = checked;
	return TRUE;
} /* End of 'checkbox_construct' function */

/* 'keydown' message handler */
wnd_msg_retcode_t checkbox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	checkbox_t *cb = CHECKBOX_OBJ(wnd);

	/* Change state */
	if (key == ' ')
	{
		checkbox_toggle(cb);
	}
	else
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_keydown' function */

/* 'display' message handler */
wnd_msg_retcode_t checkbox_on_display( wnd_t *wnd )
{
	checkbox_t *cb = CHECKBOX_OBJ(wnd);

	wnd_move(wnd, 0, 0, 0);
	wnd_set_fg_color(wnd, WND_COLOR_WHITE);
	wnd_set_bg_color(wnd, WND_COLOR_BLACK);
	wnd_push_state(wnd, WND_STATE_COLOR);
	if (WND_FOCUS(wnd) == wnd)
	{
		wnd_set_fg_color(wnd, WND_COLOR_WHITE);
		wnd_set_bg_color(wnd, WND_COLOR_BLUE);
	}
	wnd_printf(wnd, 0, 0, "[%c]", cb->m_checked ? 'X' : ' ');
	wnd_pop_state(wnd);
	wnd_putchar(wnd, 0, ' ');
	label_display_text(wnd, wnd->m_title, WND_COLOR_WHITE, WND_COLOR_BLACK, 0);
	wnd_move(wnd, 0, 1, 0);
} /* End of 'checkbox_on_display' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t checkbox_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	checkbox_toggle(CHECKBOX_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'checkbox_on_mouse' function */

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
	*width = label_text_len(WND_OBJ(di)) + 5;
	*height = 1;
} /* End of 'checkbox_get_desired_size' function */

/* Initialize checkbox class */
wnd_class_t *checkbox_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "checkbox", wnd_basic_class_init(global),
			checkbox_get_msg_info);
} /* End of 'checkbox_class_init' function */

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

/* End of 'wnd_checkbox.c' file */

