/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_radio.c
 * PURPOSE     : MPFC Window Library. Radio button functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 19.08.2004
 * NOTE        : Module prefix 'radio'.
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
#include "wnd_dlgitem.h"
#include "wnd_radio.h"

/* Create a new radio button */
radio_t *radio_new( wnd_t *parent, char *title, char *id, bool_t checked )
{
	radio_t *r;

	/* Allocate memory */
	r = (radio_t *)malloc(sizeof(*r));
	if (r == NULL)
		return NULL;
	memset(r, 0, sizeof(*r));
	WND_OBJ(r)->m_class = wnd_basic_class_init(WND_GLOBAL(parent));

	/* Initialize radio button */
	if (!radio_construct(r, parent, title, id, checked))
	{
		free(r);
		return NULL;
	}
	WND_FLAGS(r) |= WND_FLAG_INITIALIZED;
	return r;
} /* End of 'radio_new' function */

/* Radio button constructor */
bool_t radio_construct( radio_t *r, wnd_t *parent, char *title, char *id, 
		bool_t checked )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(r), parent, title, id, 
				radio_get_desired_size, NULL, 0))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(r), "keydown", radio_on_keydown);
	wnd_msg_add_handler(WND_OBJ(r), "display", radio_on_display);
	wnd_msg_add_handler(WND_OBJ(r), "mouse_ldown", radio_on_mouse);

	/* Set fields */
	r->m_checked = checked;
	return TRUE;
} /* End of 'radio_construct' function */

/* 'keydown' message handler */
wnd_msg_retcode_t radio_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Check radio button */
	if (key == ' ')
	{
		radio_check(RADIO_OBJ(wnd));
	}
	/* Move to the next item */
	else if (key == 'k' || key == KEY_CTRL_P || key == KEY_UP)
	{
		wnd_prev_focus(wnd->m_parent);
	}
	/* Move to the previous item */
	else if (key == 'j' || key == KEY_CTRL_N || key == KEY_DOWN)
	{
		wnd_next_focus(wnd->m_parent);
	}
	else
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_keydown' function */

/* 'display' message handler */
wnd_msg_retcode_t radio_on_display( wnd_t *wnd )
{
	radio_t *r = RADIO_OBJ(wnd);

	wnd_move(wnd, 0, 0, 0);
	wnd_set_fg_color(wnd, WND_COLOR_WHITE);
	wnd_set_bg_color(wnd, WND_COLOR_BLACK);
	wnd_push_state(wnd, WND_STATE_COLOR);
	if (WND_FOCUS(wnd) == wnd)
	{
		wnd_set_fg_color(wnd, WND_COLOR_WHITE);
		wnd_set_bg_color(wnd, WND_COLOR_BLUE);
	}
	wnd_printf(wnd, 0, 0, "(%c)", r->m_checked ? 'X' : ' ');
	wnd_pop_state(wnd);
	wnd_printf(wnd, 0, 0, " %s", wnd->m_title);
	wnd_move(wnd, 0, 1, 0);
} /* End of 'radio_on_display' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t radio_on_mouse( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t mb, wnd_mouse_event_t type )
{
	radio_check(RADIO_OBJ(wnd));
	return WND_MSG_RETCODE_OK;
} /* End of 'radio_on_mouse' function */

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
	*width = strlen(WND_OBJ(di)->m_title) + 5;
	*height = 1;
} /* End of 'radio_get_desired_size' function */

/* End of 'wnd_radio.c' file */

