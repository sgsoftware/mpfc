/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_root.h
 * PURPOSE     : MPFC Window Library. Interface root window
 *               specific functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.08.2004
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
#include "wnd.h"
#include "wnd_basic.h"
#include "wnd_class.h"
#include "wnd_msg.h"
#include "wnd_types.h"

/* Root window type */
typedef struct 
{
	/* Common window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_update_screen;
} wnd_root_t;

/* Get root window object */
#define WND_ROOT_OBJ(wnd)	((wnd_root_t *)wnd)

/* Register root window class */
wnd_class_t *wnd_root_class_init( wnd_global_data_t *global );

/* Get message information for root window class */
wnd_msg_handler_t **wnd_root_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/*
 * Root window specific messages stuff
 */

#define wnd_msg_update_screen_new	wnd_msg_noargs_new

/*
 * Root window message handlers
 */

/* 'keydown' message handler */
wnd_msg_retcode_t wnd_root_on_keydown( wnd_t *wnd, wnd_key_t *keycode );

/* 'display' message handler */
wnd_msg_retcode_t wnd_root_on_display( wnd_t *wnd );

/* 'close' message handler */
wnd_msg_retcode_t wnd_root_on_close( wnd_t *wnd );

/* 'update_screen' message handler */
wnd_msg_retcode_t wnd_root_on_update_screen( wnd_t *wnd );

/* Destructor */
void wnd_root_destructor( wnd_t *wnd );

#endif

/* End of 'wnd_root.h' file */

