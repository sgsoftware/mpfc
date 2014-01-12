/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Default message handlers implementation.
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

#include <strings.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "wnd.h"
#include "wnd_def_handlers.h"
#include "wnd_kbind.h"
#include "wnd_msg.h"

/* Default WND_MSG_DISPLAY message handler */
wnd_msg_retcode_t wnd_default_on_display( wnd_t *wnd )
{
	if (!(WND_FLAGS(wnd) & WND_FLAG_OWN_DECOR))
		wnd_draw_decorations(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_display' function */

/* Default WND_MSG_KEYDOWN message handler */
wnd_msg_retcode_t wnd_default_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	wnd_kbind_key2buf(wnd, key);
	return WND_MSG_RETCODE_STOP;
} /* End of 'wnd_default_on_keydown' function */

/* Default 'mouse_ldown' message handler */
wnd_msg_retcode_t wnd_default_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Handle click on the close/maximize box */
	if (!(WND_FLAGS(wnd) & WND_FLAG_OWN_DECOR) && (WND_FLAGS(wnd) & WND_FLAG_BORDER))
	{
		if (y == -1)
		{
			if (x == wnd->m_width - 4)
				if (WND_FLAGS(wnd) & WND_FLAG_MAX_BOX)
					wnd_toggle_maximize(wnd);

			if (x == wnd->m_width - 3)
				if (WND_FLAGS(wnd) & WND_FLAG_CLOSE_BOX)
					wnd_close(wnd);
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_mouse' function */

/* Default 'action' message handler */
wnd_msg_retcode_t wnd_default_on_action( wnd_t *wnd, char *action )
{
	/* Top-level windows focus switch */
	if (!strcasecmp(action, "next_focus"))
		wnd_next_focus(WND_ROOT(wnd));
	else if (!strcasecmp(action, "prev_focus"))
		wnd_prev_focus(WND_ROOT(wnd));
	/* Start window change position/size modes */
	else if (!strcasecmp(action, "move_window"))
		wnd_set_mode(wnd, WND_MODE_REPOSITION);
	else if (!strcasecmp(action, "resize_window"))
		wnd_set_mode(wnd, WND_MODE_RESIZE);
	/* Close window */
	else if (!strcasecmp(action, "close_window"))
	{
		wnd_t *anc = wnd_get_top_level_ancestor(wnd);
		if (WND_FLAGS(anc) & WND_FLAG_CLOSE_BOX)
			wnd_close(anc);
	}
	/* Maximize/minimize window */
	else if (!strcasecmp(action, "maximize_window"))
	{
		wnd_t *anc = wnd_get_top_level_ancestor(wnd);
		if (WND_FLAGS(anc) & WND_FLAG_MAX_BOX)
			wnd_toggle_maximize(anc);
	}
	/* Redisplay screen */
	else if (!strcasecmp(action, "refresh_screen"))
		wnd_redisplay(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_action' function */

/* Default WND_MSG_CLOSE message handler */
wnd_msg_retcode_t wnd_default_on_close( wnd_t *wnd )
{
	wnd_t *parent;
	struct wnd_display_buf_t *db = &WND_DISPLAY_BUF(wnd);
	int pos, size;

	/* Remove this window chars from the display buffer */
	wnd_display_buf_lock(db);
	size = db->m_width * db->m_height;
	for ( pos = 0; pos < size; pos ++ )
	{
		wnd_t *w;
		for ( w = db->m_data[pos].m_wnd; w != NULL; w = w->m_parent )
		{
			if (w == wnd)
				db->m_data[pos].m_wnd = NULL;
		}
	}
	wnd_display_buf_unlock(db);

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
		wnd_regen_zvalue_list(parent);
		wnd_set_global_focus(WND_GLOBAL(wnd));
		wnd_invalidate(parent);
	}

	/* Update the visibility information */
	wnd_global_update_visibility(WND_ROOT(wnd));

	/* Remove from the queue all messages addressing to this window */
	wnd_msg_queue_remove_by_window(WND_MSG_QUEUE(wnd), wnd, TRUE);
	
	/* Call destructor */
	wnd_call_destructor(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_close' function */

/* Default WND_MSG_ERASE_BACK message handler */
wnd_msg_retcode_t wnd_default_on_erase_back( wnd_t *wnd )
{
	struct wnd_display_buf_t *db = &WND_DISPLAY_BUF(wnd);
	struct wnd_display_buf_symbol_t *pos;
	int dist, i, j;
	wnd_t *owning;

	/* Clear each window's position */
	wnd_display_buf_lock(db);
	pos = &db->m_data[wnd->m_real_top * db->m_width + wnd->m_real_left];
	dist = db->m_width - (wnd->m_real_right - wnd->m_real_left);
	for ( i = wnd->m_real_bottom - wnd->m_real_top; i != 0; i -- )
	{
		for ( j = wnd->m_real_right - wnd->m_real_left; j != 0; j -- )
		{
			/* Check that the window owning this position is our window's
			 * descendant */
			for ( owning = pos->m_wnd; owning != NULL; 
					owning = owning->m_parent )
			{
				if (owning == wnd)
				{
					memset(&pos->m_char, 0, sizeof(pos->m_char));
					pos->m_char.chars[0] = L' ';
					break;
				}
			}

			/* Move to the next position */
			pos ++;
		}

		/* Move to the next line */
		pos += dist;
	}
	wnd_display_buf_unlock(db);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_erase_back' function */

/* Default window destructor */
void wnd_default_destructor( wnd_t *wnd )
{
	wnd_t *child;

	assert(wnd);

	/* Call destructor for all children */
	for ( child = wnd->m_child; child != NULL; )
	{
		wnd_t *next = child->m_next;
		wnd_call_destructor(child);
		child = next;
	}

	/* Free memory */
	wnd_class_free_handlers(wnd);
	free(wnd->m_title);
	free(wnd);
} /* End of 'wnd_default_destructor' function */

/* Default WND_MSG_PARENT_REPOS message handler */
wnd_msg_retcode_t wnd_default_on_parent_repos( wnd_t *wnd,
		int px, int py, int pw, int ph, int nx, int ny, int nw, int nh )
{
	wnd_t *child;
	int x, y, w, h;

	assert(wnd);

	/* Adjust window size for a maximized window */
	x = wnd->m_x;
	y = wnd->m_y;
	w = wnd->m_width;
	h = wnd->m_height;
	if (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED)
	{
		w += (nw - pw);
		h += (nh - ph);
	}

	/* Reposition */
	wnd_repos_internal(wnd, x, y, w, h);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_default_on_parent_repos' function */

/* End of 'wnd_def_handlers.c' file */

