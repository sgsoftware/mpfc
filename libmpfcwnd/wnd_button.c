/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_button.c
 * PURPOSE     : MPFC Window Library. Button functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'button'.
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

/* Create a new button */
button_t *button_new( wnd_t *parent, char *title, char *id )
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
	if (!button_construct(btn, parent, title, id))
	{
		free(btn);
		return NULL;
	}
	WND_OBJ(btn)->m_flags |= WND_FLAG_INITIALIZED;
	return btn;
} /* End of 'button_new' function */

/* Button initialization function */
bool_t button_construct( button_t *btn, wnd_t *parent, char *title, char *id )
{
	wnd_t *wnd = WND_OBJ(btn);

	assert(btn);

	/* Initialize window part */
	if (!dlgitem_construct(DLGITEM_OBJ(btn), parent, title, id, 
				button_get_desired_size, NULL, 0))
		return FALSE;

	/* Set message handlers */
	wnd_msg_add_handler(wnd, "display", button_on_display);
	wnd_msg_add_handler(wnd, "keydown", button_on_keydown);
	wnd_msg_add_handler(wnd, "mouse_ldown", button_on_mouse);
	return TRUE;
} /* End of 'button_construct' function */

/* Get button desired size */
void button_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	*width = strlen(WND_OBJ(di)->m_title) + 2;
	*height = 1;
} /* End of 'button_get_desired_size' function */

/* 'display' message handler */
wnd_msg_retcode_t button_on_display( wnd_t *wnd )
{
	wnd_move(wnd, 0, 0, 0);
	wnd_set_fg_color(wnd, WND_COLOR_WHITE);
	wnd_set_bg_color(wnd, WND_FOCUS(wnd) == wnd ?
			WND_COLOR_BLUE : WND_COLOR_GREEN);
	wnd_set_attrib(wnd, WND_ATTRIB_BOLD);
	wnd_printf(wnd, 0, 0, " %s\n", wnd->m_title);
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_display' function */

/* 'keydown' message handler */
wnd_msg_retcode_t button_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Button clicked */
	if (key == ' ')
		wnd_msg_send(wnd, "clicked", button_msg_clicked_new());
	else
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_keydown' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t button_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	wnd_msg_send(wnd, "clicked", button_msg_clicked_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'button_on_mouse' function */

/* Initialize button class */
wnd_class_t *button_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "button", wnd_basic_class_init(global),
			button_get_msg_info);
} /* End of 'button_class_init' function */

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

/* End of 'wnd_button.h' file */

