/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_common_msg.c
 * PURPOSE     : MPFC Window Library. Common messages 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.07.2004
 * NOTE        : Module prefix 'wnd_msg'.
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
#include <stdlib.h>
#include "types.h"
#include "wnd.h"
#include "wnd_common_msg.h"
#include "wnd_msg.h"

/* Construct a display message data */
wnd_msg_data_t wnd_msg_data_display_new( void )
{
	wnd_msg_data_t msg_data;
	msg_data.m_data = NULL;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_display_new' function */

/* Construct a key-related message data */
wnd_msg_data_t wnd_msg_data_key_new( wnd_key_t *keycode )
{
	wnd_msg_data_t msg_data;
	wnd_msg_data_key_t *data;

	data = (wnd_msg_data_key_t *)malloc(sizeof(*data));
	data->m_keycode = *keycode;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_key_new' function */

/* Construct a close message data */
wnd_msg_data_t wnd_msg_data_close_new( void )
{
	wnd_msg_data_t msg_data;
	msg_data.m_data = NULL;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_close_new' function */

/* Construct a erase background message data */
wnd_msg_data_t wnd_msg_data_erase_back_new( void )
{
	wnd_msg_data_t msg_data;
	msg_data.m_data = NULL;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_erase_back' function */

/* Construct a update screen message data */
wnd_msg_data_t wnd_msg_data_update_screen_new( void )
{
	wnd_msg_data_t msg_data;
	msg_data.m_data = NULL;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_update_screen' function */

/* Callback for WND_MSG_DISPLAY */
wnd_msg_retcode_t wnd_callback_display( wnd_t *wnd, wnd_msg_handler_t *h,
		wnd_msg_data_t *data )
{
	if (h == NULL)
		return wnd_default_on_display(wnd);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *))(h->m_func))(wnd);
} /* End of 'wnd_callback_display' function */

/* Callback for WND_MSG_CLOSE */
wnd_msg_retcode_t wnd_callback_close( wnd_t *wnd, wnd_msg_handler_t *h,
		wnd_msg_data_t *data )
{
	if (h == NULL)
		return wnd_default_on_close(wnd);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *))(h->m_func))(wnd);
} /* End of 'wnd_callback_close' function */

/* Callback for WND_MSG_KEYDOWN */
wnd_msg_retcode_t wnd_callback_keydown( wnd_t *wnd, wnd_msg_handler_t *h,
		wnd_msg_data_t *data )
{
	wnd_key_t *key = &((wnd_msg_data_key_t *)(data->m_data))->m_keycode;

	if (h == NULL)
		return wnd_default_on_keydown(wnd, key);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *, wnd_key_t *))(h->m_func))
			(wnd, key);
} /* End of 'wnd_callback_keydown' function */

/* Callback for WND_MSG_ERASE_BACK */
wnd_msg_retcode_t wnd_callback_erase_back( wnd_t *wnd, wnd_msg_handler_t *h,
		wnd_msg_data_t *data )
{
	if (h == NULL)
		return wnd_default_on_erase_back(wnd);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *))(h->m_func))(wnd);
} /* End of 'wnd_callback_erase_back' function */

/* Callback for WND_MSG_UPDATE_SCREEN */
wnd_msg_retcode_t wnd_callback_update_screen( wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data )
{
	if (h == NULL)
		return wnd_default_on_update_screen(wnd);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *))(h->m_func))(wnd);
} /* End of 'wnd_callback_update_screen' function */

/* Construct a parent reposition message data */
wnd_msg_data_t wnd_msg_data_parent_repos_new( int px, int py, int pw, 
		int ph, int nx, int ny, int nw, int nh )
{
	wnd_msg_data_t msg_data;
	wnd_msg_data_parent_repos_t *data;

	data = (wnd_msg_data_parent_repos_t *)malloc(sizeof(*data));
	data->m_prev_x = px;
	data->m_prev_y = py;
	data->m_prev_width = pw;
	data->m_prev_height = ph;
	data->m_new_x = nx;
	data->m_new_y = ny;
	data->m_new_width = nw;
	data->m_new_height = nh;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_parent_repos_new' function */

/* Callback for WND_MSG_PARENT_REPOS */
wnd_msg_retcode_t wnd_callback_parent_repos( struct tag_wnd_t *wnd, 
		wnd_msg_handler_t *h, wnd_msg_data_t *data )
{
	wnd_msg_data_parent_repos_t *d = 
		(wnd_msg_data_parent_repos_t *)(data->m_data);
	if (h == NULL)
		return wnd_default_on_parent_repos(wnd, d->m_prev_x, d->m_prev_y,
				d->m_prev_width, d->m_prev_height, d->m_new_x,
				d->m_new_y, d->m_new_width, d->m_new_height);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *, int, int, int, int,
						int, int, int, int))(h->m_func))(wnd, 
					d->m_prev_x, d->m_prev_y, d->m_prev_width, 
					d->m_prev_height, d->m_new_x, d->m_new_y, 
					d->m_new_width, d->m_new_height);
} /* End of 'wnd_callback_parent_repos' function */

/* Construct a mouse message data */
wnd_msg_data_t wnd_msg_data_mouse_new( int x, int y, wnd_mouse_button_t btn,
		wnd_mouse_event_t type )
{
	wnd_msg_data_t msg_data;
	wnd_msg_data_mouse_t *data;

	data = (wnd_msg_data_mouse_t *)malloc(sizeof(*data));
	data->m_x = x;
	data->m_y = y;
	data->m_button = btn;
	data->m_type = type;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_data_mouse_new' function */

/* Callback for mouse messages */
wnd_msg_retcode_t wnd_callback_mouse( struct tag_wnd_t *wnd,
		wnd_msg_handler_t *h, wnd_msg_data_t *data )
{
	wnd_msg_data_mouse_t *md = (wnd_msg_data_mouse_t *)(data->m_data);
	if (h == NULL)
		return wnd_default_on_mouse(wnd, md->m_x, md->m_y,
				md->m_button, md->m_type);
	else
		return ((wnd_msg_retcode_t (*)(wnd_t *, int, int, 
						wnd_mouse_button_t, wnd_mouse_event_t))(h->m_func))(
					wnd, md->m_x, md->m_y, md->m_button, md->m_type);
} /* End of 'wnd_callback_mouse_ldown' function */

/* End of 'wnd_common_msg.c' file */

