/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_dialog.h
 * PURPOSE     : MPFC Window Library. Interface for dialog 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'dialog'.
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

#ifndef __SG_MPFC_WND_DIALOG_H__
#define __SG_MPFC_WND_DIALOG_H__

#include "types.h"
#include "wnd.h"

/* Dialog window type */
typedef struct 
{
	/* Window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_ok,
					  *m_on_cancel;
} dialog_t;

/* Convert some window to dialog type */
#define DIALOG_OBJ(wnd)		((dialog_t *)wnd)

/* Create a new dialog */
dialog_t *dialog_new( char *title, wnd_t *parent, int x, int y,
		int width, int height, dword flags ); 

/* Dialog window constructor */
bool_t dialog_construct( dialog_t *dlg, char *title, wnd_t *parent, 
		int x, int y, int width, int height, dword flags ); 

/* Handle 'keydown' message */
wnd_msg_retcode_t dialog_on_keydown( wnd_t *wnd, wnd_key_t key );

/* Handle 'ok_clicked' message */
wnd_msg_retcode_t dialog_on_ok( wnd_t *wnd );

/* Handle 'cancel_clicked' message */
wnd_msg_retcode_t dialog_on_cancel( wnd_t *wnd );

/* Handle 'clicked' message for OK button */
wnd_msg_retcode_t dialog_ok_on_clicked( wnd_t *wnd );

/* Handle 'clicked' message for Cancel button */
wnd_msg_retcode_t dialog_cancel_on_clicked( wnd_t *wnd );

/*
 * Class functions
 */

/* Create dialog class */
wnd_class_t *dialog_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **dialog_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Aliases for creating message data */
#define dialog_msg_ok_new		wnd_msg_noargs_new
#define dialog_msg_cancel_new	wnd_msg_noargs_new

#endif

/* End of 'wnd_dialog.h' file */

