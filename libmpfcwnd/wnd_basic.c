/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_basic.c
 * PURPOSE     : MPFC Window Library. 'basic' window class
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.08.2004
 * NOTE        : Module prefix 'wnd_basic'.
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

#include "types.h"
#include "wnd.h"

/* Initialize basic window class */
wnd_class_t *wnd_basic_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "basic", NULL, wnd_basic_get_msg_info);
} /* End of 'wnd_basic_class_init' function */

/* Get message handler and callback */
wnd_msg_handler_t **wnd_basic_get_msg_info( wnd_t *wnd, char *name,
		wnd_class_msg_callback_t *callback )
{
	assert(wnd);
	assert(name);

	/* Walk through supported messages list */
	if (!strcmp(name, "display"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &wnd->m_on_display;
	}
	else if (!strcmp(name, "destructor"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_destructor;
		return &wnd->m_destructor;
	}
	else if (!strcmp(name, "keydown"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_key;
		return &wnd->m_on_keydown;
	}
	else if (!strcmp(name, "erase_back"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &wnd->m_on_erase_back;
	}
	else if (!strcmp(name, "close"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &wnd->m_on_close;
	}
	else if (!strcmp(name, "parent_repos"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_parent_repos;
		return &wnd->m_on_parent_repos;
	}
	else if (!strcmp(name, "mouse_ldown"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_ldown;
	}
	else if (!strcmp(name, "mouse_mdown"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_mdown;
	}
	else if (!strcmp(name, "mouse_rdown"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_rdown;
	}
	else if (!strcmp(name, "mouse_ldouble"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_ldouble;
	}
	else if (!strcmp(name, "mouse_mdouble"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_mdouble;
	}
	else if (!strcmp(name, "mouse_rdouble"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_mouse;
		return &wnd->m_on_mouse_rdouble;
	}
	return NULL;
} /* End of 'wnd_basic_get_msg_info' function */

/* Create data for no-arguments message (base function) */
wnd_msg_data_t wnd_msg_noargs_new( void )
{
	wnd_msg_data_t msg_data;
	msg_data.m_data = NULL;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_noargs_new' function */

/* Callback function for no-arguments messages */
wnd_msg_retcode_t wnd_basic_callback_noargs( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	return ((wnd_msg_retcode_t (*)(wnd_t *))(handler->m_func))(wnd);
} /* End of 'wnd_basic_callback_noargs' function */

/* Create data for key-related message */
wnd_msg_data_t wnd_msg_key_new( wnd_key_t *keycode )
{
	wnd_msg_data_t msg_data;
	wnd_msg_key_t *data;

	data = (wnd_msg_key_t *)malloc(sizeof(*data));
	data->m_keycode = *keycode;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_key_new' function */

/* Callback function for keyboard messages */
wnd_msg_retcode_t wnd_basic_callback_key( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	wnd_msg_key_t *d = (wnd_msg_key_t *)(msg_data->m_data);
	return ((wnd_msg_retcode_t (*)(wnd_t *, wnd_key_t *))(handler->m_func))
		(wnd, &d->m_keycode);
} /* End of 'wnd_basic_callback_key' function */

/* Create data for parent reposition message */
wnd_msg_data_t wnd_msg_parent_repos_new( int px, int py, int pw, int ph,
		int nx, int ny, int nw, int nh )
{
	wnd_msg_data_t msg_data;
	wnd_msg_parent_repos_t *data;

	data = (wnd_msg_parent_repos_t *)malloc(sizeof(*data));
	data->m_prev_x = px;
	data->m_prev_y = py;
	data->m_prev_w = pw;
	data->m_prev_h = ph;
	data->m_new_x = nx;
	data->m_new_y = ny;
	data->m_new_w = nw;
	data->m_new_h = nh;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_parent_repos_new' function */

/* Callback function for parent reposition message */
wnd_msg_retcode_t wnd_basic_callback_parent_repos( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	wnd_msg_parent_repos_t *d = (wnd_msg_parent_repos_t *)(msg_data->m_data);
	return ((wnd_msg_retcode_t (*)(wnd_t *, int, int, int, int, 
				int, int, int, int))(handler->m_func))(wnd,
					d->m_prev_x, d->m_prev_y, d->m_prev_w, d->m_prev_h,
					d->m_new_x, d->m_new_y, d->m_new_w, d->m_new_h);
} /* End of 'wnd_basic_callback_parent_repos' function */

/* Create mouse message data */
wnd_msg_data_t wnd_msg_mouse_new( int x, int y, wnd_mouse_event_t type,
		wnd_mouse_button_t button )
{
	wnd_msg_data_t msg_data;
	wnd_msg_mouse_t *data;

	data = (wnd_msg_mouse_t *)malloc(sizeof(*data));
	data->m_x = x;
	data->m_y = y;
	data->m_type = type;
	data->m_button = button;
	msg_data.m_data = data;
	msg_data.m_destructor = NULL;
	return msg_data;
} /* End of 'wnd_msg_mouse_new' function */

/* Callback function */
wnd_msg_retcode_t wnd_basic_callback_mouse( wnd_t *wnd, 
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	wnd_msg_mouse_t *d = (wnd_msg_mouse_t *)(msg_data->m_data);
	return ((wnd_msg_retcode_t (*)(wnd_t *, int, int, wnd_mouse_event_t,
				wnd_mouse_button_t))(handler->m_func))(wnd,
				d->m_x, d->m_y, d->m_type, d->m_button);
} /* End of 'wnd_basic_callback_mouse' function */

/* Callback for destructor */
wnd_msg_retcode_t wnd_basic_callback_destructor( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data )
{
	((void (*)(wnd_t *))(handler->m_func))(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_basic_callback_destructor' function */

/* End of 'wnd_basic.h' file */

