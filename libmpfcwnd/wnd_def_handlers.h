/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for default *message handlers. 
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

#ifndef __SG_MPFC_WND_DEF_HANDLERS_H__
#define __SG_MPFC_WND_DEF_HANDLERS_H__

#include "types.h"
#include "wnd_kbd.h"
#include "wnd_msg.h"
#include "wnd_types.h"

/* Default 'display' message handler */
wnd_msg_retcode_t wnd_default_on_display( wnd_t *wnd );

/* Default 'keydown' message handler */
wnd_msg_retcode_t wnd_default_on_keydown( wnd_t *wnd, wnd_key_t key );

/* Default 'mouse_ldown' message handler */
wnd_msg_retcode_t wnd_default_on_mouse( wnd_t *wnd, int x, int y, 
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Default 'action' message handler */
wnd_msg_retcode_t wnd_default_on_action( wnd_t *wnd, char *action );

/* Default 'close' message handler */
wnd_msg_retcode_t wnd_default_on_close( wnd_t *wnd );

/* Default 'erase_back' message handler */
wnd_msg_retcode_t wnd_default_on_erase_back( wnd_t *wnd );

/* Default 'parent_repos' message handler */
wnd_msg_retcode_t wnd_default_on_parent_repos( wnd_t *wnd,
		int px, int py, int pw, int ph, int nx, int ny, int nw, int nh );

/* Default window destructor */
void wnd_default_destructor( wnd_t *wnd );

#endif

/* End of 'wnd_def_handlers.h' file */

