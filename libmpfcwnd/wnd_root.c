/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_root.c
 * PURPOSE     : MPFC Window Library. Root window handlers
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 24.07.2004
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
#include "wnd_msg.h"
#include "wnd_root.h"

/* WND_MSG_DISPLAY message handler */
wnd_msg_retcode_t wnd_root_on_display( wnd_t *wnd )
{
	/* Display windows bar */
	wnd_display_wnd_bar(wnd);
} /* End of 'wnd_root_on_display' function */

/* WND_MSG_CLOSE message handler */
wnd_msg_retcode_t wnd_root_on_close( wnd_t *wnd )
{
	/* Call destructor and exit window system */
	wnd_call_destructor(wnd);
	return WND_MSG_RETCODE_EXIT;
} /* End of 'wnd_root_on_close' function */

/* WND_MSG_KEYDOWN message handler */
wnd_msg_retcode_t wnd_root_on_key( wnd_t *wnd, wnd_key_t *keycode )
{
	/* Close window on 'q' */
	if (keycode->m_key == 'q' || keycode->m_key == 'Q')
		wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_root_on_close' function */

/* Root window destructor */
void wnd_root_destructor( wnd_t *wnd )
{
	assert(wnd);

	/* Free modules */
	wnd_mouse_free(WND_MOUSE_DATA(wnd));
	wnd_kbd_free(WND_KBD_DATA(wnd));
	wnd_msg_queue_free(WND_MSG_QUEUE(wnd));

	/* Free display buffer */
	free(WND_DISPLAY_BUF(wnd).m_data);

	/* Free global data */
	free(WND_GLOBAL(wnd));
	
	/* Uninitialize NCURSES */
	endwin();
} /* End of 'wnd_root_destructor' function */

/* End of 'wnd_root.c' file */

