/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_def_handlers.h
 * PURPOSE     : MPFC Window Library. Interface for default 
 *               message handlers. 
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

#ifndef __SG_MPFC_WND_DEF_HANDLERS_H__
#define __SG_MPFC_WND_DEF_HANDLERS_H__

#include "types.h"
#include "wnd_kbd.h"
#include "wnd_msg.h"

/* Further declarations */
struct tag_wnd_t;

/* Default WND_MSG_DISPLAY message handler */
wnd_msg_retcode_t wnd_default_on_display( struct tag_wnd_t *wnd );

/* Default WND_MSG_KEYDOWN message handler */
wnd_msg_retcode_t wnd_default_on_keydown( struct tag_wnd_t *wnd, 
		wnd_key_t *keycode );

/* Default WND_MSG_CLOSE message handler */
wnd_msg_retcode_t wnd_default_on_close( struct tag_wnd_t *wnd );

/* Default WND_MSG_ERASE_BACK message handler */
wnd_msg_retcode_t wnd_default_on_erase_back( struct tag_wnd_t *wnd );

/* Default WND_MSG_UPDATE_SCREEN message handler */
wnd_msg_retcode_t wnd_default_on_update_screen( struct tag_wnd_t *wnd );

/* Default WND_MSG_PARENT_REPOS message handler */
wnd_msg_retcode_t wnd_default_on_parent_repos( struct tag_wnd_t *wnd,
		int px, int py, int pw, int ph, int nx, int ny, int nw, int nh );

/* Default mouse messages handler */
wnd_msg_retcode_t wnd_default_on_mouse( struct tag_wnd_t *wnd,
		int x, int y, wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Default window destructor */
void wnd_default_destructor( struct tag_wnd_t *wnd );

#endif

/* End of 'wnd_def_handlers.h' file */

