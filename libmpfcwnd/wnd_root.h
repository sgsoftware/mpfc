/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_root.h
 * PURPOSE     : MPFC Window Library. Interface for root window
 *               handlers.
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

#ifndef __SG_MPFC_WND_ROOT_H__
#define __SG_MPFC_WND_ROOT_H__

#include "types.h"

/* Further declarations */
struct tag_wnd_t;
enum wnd_msg_retcode_t;

/* WND_MSG_KEYDOWN message handler */
wnd_msg_retcode_t wnd_root_on_key( struct tag_wnd_t *wnd, wnd_key_t *keycode );

/* WND_MSG_DISPLAY message handler */
wnd_msg_retcode_t wnd_root_on_display( struct tag_wnd_t *wnd );

/* WND_MSG_CLOSE message handler */
wnd_msg_retcode_t wnd_root_on_close( struct tag_wnd_t *wnd );

/* Root window destructor */
void wnd_root_destructor( struct tag_wnd_t *wnd );

#endif

/* End of 'wnd_root.h' file */

