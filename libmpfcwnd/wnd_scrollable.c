/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Scrollable windows functions implementation.
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
#include "wnd_scrollable.h"

/* Create a new scrollable window */
scrollable_t *scrollable_new( wnd_t *parent, char *title, int x, int y, int w,
		int h, scrollable_type_t type, wnd_flags_t flags )
{
	scrollable_t *scr;

	/* Allocate memory */
	scr = (scrollable_t *)malloc(sizeof(*scr));
	if (scr == NULL)
		return NULL;
	memset(scr, 0, sizeof(*scr));
	WND_OBJ(scr)->m_class = scrollable_class_init(WND_GLOBAL(parent));

	/* Initialize window */
	if (!scrollable_construct(scr, parent, title, x, y, w, h, type, flags))
	{
		free(scr);
		return NULL;
	}
	wnd_postinit(scr);
	return scr;
} /* End of 'scrollable_new' function */

/* Initialize scrollable */
bool_t scrollable_construct( scrollable_t *scr, wnd_t *parent, char *title, 
		int x, int y, int w, int h, scrollable_type_t type, wnd_flags_t flags )
{
	wnd_t *wnd = WND_OBJ(scr);
	assert(type == SCROLLABLE_VERTICAL || type == SCROLLABLE_HORIZONTAL);

	/* Initialize window part */
	if (!wnd_construct(wnd, parent, title, x, y, w, h, flags))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "display", scrollable_on_display);

	/* Set fields */
	if (type == SCROLLABLE_VERTICAL)
		wnd->m_client_w --;
	else
		wnd->m_client_h --;
	scr->m_type = type;
	return TRUE;
} /* End of 'scrollable_construct' function */

/* 'display' message handler */
wnd_msg_retcode_t scrollable_on_display( wnd_t *wnd )
{
	scrollable_t *scr = SCROLLABLE_OBJ(wnd);
	int i;

	/* Display scroll bar */
	if (scr->m_type == SCROLLABLE_VERTICAL)
	{
		int slider_pos;
		int scroll_range;

		/* Paint up arrow */
		wnd_apply_style(wnd, "scroll-arrow-style");
		wnd_move(wnd, 0, wnd->m_client_w, 0);
		wnd_putchar(wnd, WND_PRINT_NONCLIENT, '^');

		/* Paint scroll bar */
		scroll_range = SCROLLABLE_RANGE(scr);
		if (scroll_range <= 0)
			slider_pos = 0;
		else
			slider_pos = scr->m_scroll * (wnd->m_client_h - 2) / scroll_range;
		wnd_apply_style(wnd, "scroll-bar-style");
		for ( i = 0; i < wnd->m_client_h - 2; i ++ )
		{
			wnd_move(wnd, 0, wnd->m_client_w, i + 1);
			if (i == slider_pos)
			{
				wnd_apply_style(wnd, "scroll-slider-style");
				wnd_put_special(wnd, WND_ACS_CODE(WACS_BLOCK));
				wnd_apply_style(wnd, "scroll-bar-style");
			}
			else
			{
				wnd_put_special(wnd, WND_ACS_CODE(WACS_VLINE));
			}
		}
		
		/* Paint down arrow */
		wnd_move(wnd, 0, wnd->m_client_w, wnd->m_client_h - 1);
		wnd_apply_style(wnd, "scroll-arrow-style");
		wnd_putchar(wnd, WND_PRINT_NONCLIENT, 'v');
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'scrollable_on_display' function */

/* Scroll window */
void scrollable_scroll( scrollable_t *scr, int value, bool_t absolute )
{
	int was_value = scr->m_scroll;

	/* Set new value */
	scr->m_scroll = (absolute ? value : was_value + value);
	if (scr->m_scroll >= SCROLLABLE_RANGE(scr))
		scr->m_scroll = SCROLLABLE_RANGE(scr) - 1;
	if (scr->m_scroll < 0)
		scr->m_scroll = 0;

	/* Send notification message */
	wnd_msg_send(WND_OBJ(scr), "scrolled", 
			scrollable_msg_scrolled_new(scr->m_scroll - was_value));
	wnd_invalidate(WND_OBJ(scr));
} /* End of 'scrollable_scroll' function */

/* Change list size */
void scrollable_set_size( scrollable_t *scr, int new_size )
{
	scr->m_list_size = new_size;

	/* Do automatic scroll to the end */
	scrollable_scroll(scr, SCROLLABLE_RANGE(scr) - 1, TRUE);
} /* End of 'scrollable_set_size' function */

/* Initialize scrollable window class */
wnd_class_t *scrollable_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "scrollable", 
			wnd_basic_class_init(global), scrollable_get_msg_info,
			scrollable_free_handlers, scrollable_class_set_default_styles);
	return klass;
} /* End of 'scrollable_class_init' function */

/* Set default styles */
void scrollable_class_set_default_styles( cfg_node_t *node )
{
	cfg_set_var(node, "scroll-arrow-style", "cyan:black:bold");
	cfg_set_var(node, "scroll-bar-style", "cyan:black:bold");
	cfg_set_var(node, "scroll-slider-style", "green:black:bold");
} /* End of 'scrollable_class_set_default_styles' function */

/* Get message information */
wnd_msg_handler_t **scrollable_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "scrolled"))
	{
		if (callback != NULL)
			(*callback) = scrollable_callback_scrolled;
		return &(SCROLLABLE_OBJ(wnd)->m_on_scrolled);
	}
	return NULL;
} /* End of 'scrollable_get_msg_info' function */

/* Free message handlers */
void scrollable_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(SCROLLABLE_OBJ(wnd)->m_on_scrolled);
} /* End of 'scrollable_free_handlers' function */

/* Create data for 'scrolled' message */
wnd_msg_data_t scrollable_msg_scrolled_new( int offset )
{
	wnd_msg_data_t msg_data;
	scrollable_msg_scrolled_t *data;

	data = (scrollable_msg_scrolled_t *)malloc(sizeof(*data));
	data->m_offset = offset;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'scrollable_msg_scrolled_new' function */

/* Callback for this message */
wnd_msg_retcode_t scrollable_callback_scrolled( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	scrollable_msg_scrolled_t *d = 
		(scrollable_msg_scrolled_t *)(msg_data->m_data);
	return SCROLLABLE_MSG_SCROLLED_HANDLER(handler)(wnd, d->m_offset);
} /* End of 'scrollable_callback_scrolled' function */

/* End of 'wnd_scrollable.c' file */

