/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_root.c
 * PURPOSE     : MPFC Window Library. Root window functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.09.2004
 * NOTE        : Module prefix 'wnd_root'.
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
#include "wnd_root.h"

/* Register root window class */
wnd_class_t *wnd_root_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "root", wnd_basic_class_init(global),
			wnd_root_get_msg_info, NULL);
} /* End of 'wnd_root_class_init' function */

/* Get message information for root window class */
wnd_msg_handler_t **wnd_root_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	/* Search in root window messages */
	if (!strcmp(msg_name, "update_screen"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &(WND_ROOT_OBJ(wnd)->m_on_update_screen);
	}
	return NULL;
} /* End of 'wnd_root_get_msg_info' function */

/* 'keydown' message handler */
wnd_msg_retcode_t wnd_root_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Close window on 'q' */
	if (key == 'q' || key == 'Q')
		wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_root_on_keydown' function */

/* 'display' message handler */
wnd_msg_retcode_t wnd_root_on_display( wnd_t *wnd )
{
	/* Display windows bar */
	wnd_display_wnd_bar(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_root_on_display' function */

/* 'close' message handler */
wnd_msg_retcode_t wnd_root_on_close( wnd_t *wnd )
{
	/* Call destructor and exit window system */
	wnd_call_destructor(wnd);
	return WND_MSG_RETCODE_EXIT;
} /* End of 'wnd_root_on_close' function */

/* 'update_screen' message handler */
wnd_msg_retcode_t wnd_root_on_update_screen( wnd_t *wnd )
{
	wnd_sync_screen(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_root_on_update_screen' function */

/* 'mouse_ldown' message handler */
wnd_msg_retcode_t wnd_root_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	/* Move focus to the respective child if we clicked the window bar */
	if (y == wnd->m_height - 1 && (wnd->m_num_children > 0))
	{
		int left, right, i;
		wnd_t *child;

		left = 0;
		for ( child = wnd->m_child, i = 0; child != NULL;
				child = child->m_next, i ++ )
		{
			right = (i + 1) * wnd->m_width / wnd->m_num_children;
			if (x >= left && x <= right)
			{
				wnd_set_focus(child);
				break;
			}
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_root_on_mouse' function */

/* Destructor */
void wnd_root_destructor( wnd_t *wnd )
{
	wnd_class_t *klass;

	assert(wnd);

	/* Free modules */
	wnd_mouse_free(WND_MOUSE_DATA(wnd));
	wnd_kbind_free(WND_KBIND_DATA(wnd));
	wnd_kbd_free(WND_KBD_DATA(wnd));
	wnd_msg_queue_free(WND_MSG_QUEUE(wnd));

	/* Free window classes */
	for ( klass = WND_CLASSES(wnd); klass != NULL; )
	{
		wnd_class_t *next = klass->m_next;
		wnd_class_free(klass);
		klass = next;
	}

	/* Free display buffer */
	pthread_mutex_destroy(&WND_DISPLAY_BUF(wnd).m_mutex);
	free(WND_DISPLAY_BUF(wnd).m_data);

	/* Free global data */
	free(WND_GLOBAL(wnd));
	
	/* Uninitialize NCURSES */
	endwin();
} /* End of 'wnd_root_destructor' function */

/* End of 'wnd_root.c' file */

