/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_def_handlers.c
 * PURPOSE     : MPFC Window Library. Default message handlers
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 24.07.2004
 * NOTE        : Module prefix 'wnd_default'.
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

#include <assert.h>
#include "types.h"
#include "wnd.h"
#include "wnd_def_handlers.h"
#include "wnd_msg.h"

/* Default WND_MSG_DISPLAY message handler */
wnd_msg_retcode_t wnd_default_on_display( wnd_t *wnd )
{
	if (!(WND_FLAGS(wnd) & WND_FLAG_OWN_DECOR))
		wnd_draw_decorations(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_display' function */

/* Default WND_MSG_KEYDOWN message handler */
wnd_msg_retcode_t wnd_default_on_keydown( wnd_t *wnd, wnd_key_t *keycode )
{
	/* Top-level windows focus switch */
	if (keycode->m_key == '.' && keycode->m_alt)
		wnd_next_focus(WND_ROOT(wnd));
	else if (keycode->m_key == ',' && keycode->m_alt)
		wnd_prev_focus(WND_ROOT(wnd));
	/* Start window change position/size modes */
	else if (keycode->m_key == 'p' && keycode->m_alt)
		wnd_set_mode(wnd, WND_MODE_REPOSITION);
	else if (keycode->m_key == 's' && keycode->m_alt)
		wnd_set_mode(wnd, WND_MODE_RESIZE);
	/* Close window */
	else if (keycode->m_key == 'c' && keycode->m_alt && 
			(WND_FLAGS(wnd) & WND_FLAG_CLOSE_BOX))
		wnd_msg_send(wnd, WND_MSG_CLOSE, wnd_msg_data_close_new());
	/* Maximize/minimize window */
	else if (keycode->m_key == 'm' && keycode->m_alt && 
			(WND_FLAGS(wnd) & WND_FLAG_CLOSE_BOX))
		wnd_toggle_maximize(wnd);
	/* Redisplay screen */
	else if (keycode->m_key == KEY_CTRL_L)
		wnd_redisplay(wnd);
	return WND_MSG_RETCODE_STOP;
} /* End of 'wnd_default_on_keydown' function */

/* Default WND_MSG_CLOSE message handler */
wnd_msg_retcode_t wnd_default_on_close( wnd_t *wnd )
{
	wnd_t *parent;

	/* Remove the window from the parent */
	parent = wnd->m_parent;
	if (parent != NULL)
	{
		wnd_t *child, *new_focus = NULL;

		/* Remove window from children list */
		if (wnd->m_next != NULL)
			wnd->m_next->m_prev = wnd->m_prev;
		if (wnd->m_prev != NULL)
			wnd->m_prev->m_next = wnd->m_next;
		else
			parent->m_child = wnd->m_next;
		parent->m_num_children --;

		/* Rearrange windows */
		for ( child = parent->m_child; child != NULL; child = child->m_next )
		{
			if (child->m_zval > wnd->m_zval)
				child->m_zval --;
			if (child->m_zval == parent->m_num_children - 1)
				new_focus = child;
		}
		if (parent->m_focus_child == wnd)
			parent->m_focus_child = new_focus;
		wnd_set_global_focus(WND_GLOBAL(wnd));
		wnd_invalidate(parent);
	}

	/* Remove from the queue all messages addressing to this window */
	wnd_msg_queue_remove_by_window(WND_MSG_QUEUE(wnd), wnd, TRUE);
	
	/* Call destructor */
	wnd_call_destructor(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_close' function */

/* Default WND_MSG_ERASE_BACK message handler */
wnd_msg_retcode_t wnd_default_on_erase_back( wnd_t *wnd )
{
	int i, j;
	wnd_push_state(wnd, WND_STATE_COLOR | WND_STATE_CURSOR);
	wnd_set_color(wnd, WND_COLOR_WHITE, WND_COLOR_BLACK);
	wnd_set_attrib(wnd, 0);
	for ( i = 0; i < wnd->m_height; i ++ )
	{
		wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, i);
		for ( j = 0; j < wnd->m_width; j ++ )
			wnd_putc(wnd, ' ');
	}
	wnd_pop_state(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_erase_back' function */

/* Default WND_MSG_UPDATE_SCREEN message handler */
wnd_msg_retcode_t wnd_default_on_update_screen( wnd_t *wnd )
{
	wnd_sync_screen(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_update_screen' function */

/* Default window destructor */
void wnd_default_destructor( wnd_t *wnd )
{
	wnd_t *child;

	assert(wnd);

	/* Call destructor for all children */
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
		wnd_call_destructor(child);

	/* Free memory */
	free(wnd->m_title);
	free(wnd);
} /* End of 'wnd_default_destructor' function */

/* Default WND_MSG_PARENT_REPOS message handler */
wnd_msg_retcode_t wnd_default_on_parent_repos( wnd_t *wnd,
		int px, int py, int pw, int ph, int nx, int ny, int nw, int nh )
{
	wnd_t *child;
	int was_x, was_y, was_w, was_h;

	assert(wnd);

	/* Adjust window screen coordinates */
	was_x = wnd->m_x;
	was_y = wnd->m_y;
	was_w = wnd->m_width;
	was_h = wnd->m_height;
	wnd->m_screen_x += (nx - px);
	wnd->m_screen_y += (ny - py);

	/* And also size for maximized windows */
	if (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED)
	{
		wnd->m_width += (nw - pw);
		wnd->m_height += (nh - ph);
		wnd->m_client_w += (nw - pw);
		wnd->m_client_h += (nh - ph);
	}

	/* Inform our children too */
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
		wnd_msg_send(child, WND_MSG_PARENT_REPOS,
				wnd_msg_data_parent_repos_new(was_x, was_y, was_w, was_h,
					wnd->m_x, wnd->m_y, wnd->m_width, wnd->m_height));
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_parent_repos' function */

/* Default mouse messages handler */
wnd_msg_retcode_t wnd_default_on_mouse( wnd_t *wnd,
		int x, int y, wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_mouse' function */

/* End of 'wnd_def_handlers.c' file */

