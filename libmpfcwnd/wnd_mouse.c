/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_mouse.c
 * PURPOSE     : MPFC Window Library. Mouse functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.08.2004
 * NOTE        : Module prefix 'wnd_mouse'.
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
#ifdef HAVE_LIBGPM
#include <gpm.h>
#endif
#include <stdlib.h>
#include "wnd.h"

/* Initialize mouse */
wnd_mouse_data_t *wnd_mouse_init( wnd_global_data_t *global )
{
	wnd_mouse_data_t *data;
	bool_t ret = TRUE;

	/* Create data */
	data = (wnd_mouse_data_t *)malloc(sizeof(*data));
	if (data == NULL)
		return NULL;

	/* Determine the driver */
	data->m_driver = wnd_mouse_get_driver(global->m_root_cfg->m_parent);
	data->m_root_wnd = global->m_root;
	
	/* Make driver-specific initialization */
	switch (data->m_driver)
	{
#ifdef HAVE_LIBGPM
	case WND_MOUSE_GPM:
		ret = wnd_mouse_init_gpm(data);
		break;
#endif
	case WND_MOUSE_XTERM:
		ret = wnd_mouse_init_xterm(data);
		break;
	}
	if (!ret)
	{
		free(data);
		return NULL;
	}
	return data;
} /* End of 'wnd_mouse_init' function */

#ifdef HAVE_LIBGPM
/* Initialize mouse with GPM driver */
bool_t wnd_mouse_init_gpm( wnd_mouse_data_t *data )
{
	Gpm_Connect conn;

	/* Initialize GPM */
	memset(&conn, 0, sizeof(conn));
	conn.eventMask = GPM_DOWN | GPM_DOUBLE | GPM_DRAG | GPM_UP;
	conn.defaultMask = ~GPM_HARD;
	conn.minMod = 0;
	conn.maxMod = 0;
	if (Gpm_Open(&conn, 0) == -1)
		return FALSE;
	gpm_zerobased = TRUE;

	/* Start thread */
	data->m_end_thread = FALSE;
	if (pthread_create(&data->m_tid, NULL, wnd_mouse_thread, data))
		return FALSE;
	return TRUE;
} /* End of 'wnd_mouse_init_gpm' function */
#endif

/* Initialize mouse with xterm driver */
bool_t wnd_mouse_init_xterm( wnd_mouse_data_t *data )
{
	printf("\033[?9h");
	return TRUE;
} /* End of 'wnd_mouse_init_xterm' function */

/* Free mouse-related stuff */
void wnd_mouse_free( wnd_mouse_data_t *data )
{
	assert(data);
	switch (data->m_driver)
	{
#ifdef HAVE_LIBGPM
	case WND_MOUSE_GPM:
		wnd_mouse_free_gpm(data);
		break;
#endif
	case WND_MOUSE_XTERM:
		wnd_mouse_free_xterm(data);
		break;
	}
	free(data);
} /* End of 'wnd_mouse_free' function */

/* Free mouse in GPM mode */
void wnd_mouse_free_gpm( wnd_mouse_data_t *data )
{
	/* Terminate mouse thread */
	data->m_end_thread = TRUE;
	pthread_join(data->m_tid, NULL);
} /* End of 'wnd_mouse_free_gpm' function */

/* Free mouse in xterm mode */
void wnd_mouse_free_xterm( wnd_mouse_data_t *data )
{
} /* End of 'wnd_mouse_free_xterm' function */

/* Get window under which mouse cursor is */
wnd_t *wnd_mouse_find_cursor_wnd( wnd_mouse_data_t *data, int x, int y )
{
	return wnd_mouse_find_cursor_child(data->m_root_wnd, x, y);
} /* End of 'wnd_get_wnd_under_cursor' function */

/* Find a given window child that is under the cursor */
wnd_t *wnd_mouse_find_cursor_child( wnd_t *wnd, int x, int y )
{
	wnd_t *child, *found_child = NULL;

	/* Find the child */
	for ( child = wnd->m_focus_child; child != NULL; 
			child = child->m_lower_sibling )
	{
		if (x >= child->m_x && y >= child->m_y && 
				x < child->m_x + child->m_width &&
				y < child->m_y + child->m_height)
		{
			found_child = child;
			break;
		}
	}

	/* No one of the children satisfies - return us */
	if (found_child == NULL)
		return wnd;

	/* Now go recursively into the found child */
	return wnd_mouse_find_cursor_child(found_child, x - found_child->m_x,
			y - found_child->m_y);
} /* End of 'wnd_mouse_find_cursor_child' function */

/* Determine the mouse driver type */
wnd_mouse_driver_t wnd_mouse_get_driver( cfg_node_t *cfg )
{
	char *driver, *term;

	assert(cfg);
	
	/* Check variable that may force mouse type */
	driver = cfg_get_var(cfg, "mouse");
	if (driver != NULL)
	{
		if (!strcmp(driver, "xterm"))
			return WND_MOUSE_XTERM;
#ifdef HAVE_LIBGPM
		else if (!strcmp(driver, "gpm"))
			return WND_MOUSE_GPM;
#endif
		return WND_MOUSE_NONE;
	}

	/* Check TERM variable */
	term = getenv("TERM");
	if (term == NULL || !strcmp(term, ""))
		return WND_MOUSE_NONE;
	else if (!strcmp(term, "xterm") || !strcmp(term, "rxvt") || 
				!strcmp(term, "Eterm"))
		return WND_MOUSE_XTERM;
#ifdef HAVE_LIBGPM
	else
		return WND_MOUSE_GPM;
#endif
	return WND_MOUSE_NONE;
} /* End of 'wnd_get_mouse_type' function */

#ifdef HAVE_LIBGPM
/* GPM Mouse thread function */
void *wnd_mouse_thread( void *arg )
{
	fd_set readset;
	struct timeval tv;
	wnd_mouse_data_t *data = (wnd_mouse_data_t *)arg;

	/* Loop */
	for ( ; !data->m_end_thread; )
	{
		Gpm_Event event;
		int ret;
		
		if (gpm_fd >= 0)
		{
			/* Initialize stuff for select */
			FD_ZERO(&readset);
			FD_SET(gpm_fd, &readset);
			memset(&tv, 0, sizeof(tv));
			
			/* Check for events */
			if ((ret = select(gpm_fd + 1, &readset, NULL, NULL, &tv)) > 0)
			{
				if (Gpm_GetEvent(&event) > 0)
				{
					wnd_mouse_button_t btn;
					wnd_mouse_event_t type = -1;

					if (event.buttons & GPM_B_LEFT)
						btn = WND_MOUSE_LEFT;
					else if (event.buttons & GPM_B_RIGHT)
						btn = WND_MOUSE_RIGHT;
					else if (event.buttons & GPM_B_MIDDLE)
						btn = WND_MOUSE_MIDDLE;
					if (event.type & GPM_DOWN)
					{
						if (event.type & GPM_SINGLE)
							type = WND_MOUSE_DOWN;
						else if (event.type & GPM_DOUBLE)
							type = WND_MOUSE_DOUBLE;
					}
					wnd_mouse_handle_event(data, event.x, event.y, 
							btn, type, &event);
				}
			}
		}

		/* Wait a little */
		util_wait();
	}
	return NULL;
} /* End of 'wnd_mouse_thread' function */
#endif

/* Handle the mouse event (send respective message) */
void wnd_mouse_handle_event( wnd_mouse_data_t *data, 
		int x, int y, wnd_mouse_button_t btn, wnd_mouse_event_t type,
		void *additional )
{
	wnd_t *wnd;
	char *msg = NULL;

	/* Determine window to which this event is addressed */
	wnd = wnd_mouse_find_cursor_wnd(data, x, y);
	x = x - wnd->m_screen_x - wnd->m_client_x;
	y = y - wnd->m_screen_y - wnd->m_client_y;

	/* Send respective message to it */
	if (type == WND_MOUSE_DOUBLE) 
	{
		if (btn == WND_MOUSE_LEFT)
			msg = "mouse_ldouble";
		else if (btn == WND_MOUSE_RIGHT)
			msg = "mouse_rdouble";
		else if (btn == WND_MOUSE_MIDDLE)
			msg = "mouse_mdouble";
	}
	else if (type == WND_MOUSE_DOWN) 
	{
		if (btn == WND_MOUSE_LEFT)
			msg = "mouse_ldown";
		else if (btn == WND_MOUSE_RIGHT)
			msg = "mouse_rdown";
		else if (btn == WND_MOUSE_MIDDLE)
			msg = "mouse_mdown";
	}		
	if (msg != NULL)
	{
		wnd_msg_send(wnd, msg, wnd_msg_mouse_new(x, y, btn, type));
	}
//	if (wnd != wnd_focus && msg >= 0)
//		wnd_send_msg(wnd_focus, WND_MSG_MOUSE_OUTSIDE_FOCUS, 0);

	/* Display cursor */
#ifdef HAVE_GPM_H
	if (data->m_driver == WND_MOUSE_GPM)
		GPM_DRAWPOINTER((Gpm_Event *)additional);
#endif
} /* End of 'wnd_mouse_handle_event' function */

/* End of 'wnd_mouse.c' file */

