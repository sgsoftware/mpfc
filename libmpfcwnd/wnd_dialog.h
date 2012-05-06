/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for dialog functions.
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

#ifndef __SG_MPFC_WND_DIALOG_H__
#define __SG_MPFC_WND_DIALOG_H__

#include "types.h"
#include "wnd.h"
#include "wnd_hbox.h"
#include "wnd_vbox.h"

/* Dialog window type */
typedef struct 
{
	/* Window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_ok,
					  *m_on_cancel;

	/* Main controls box */
	vbox_t *m_vbox;

	/* Buttons box */
	hbox_t *m_hbox;

	/* First focus branch */
	wnd_t *m_first_branch;
} dialog_t;

/* Flags for dialog items iteration */
typedef enum
{
	DIALOG_ITERATE_CYCLE = 1 << 0,
	DIALOG_ITERATE_ZORDER = 1 << 1
} dialog_iterate_flags_t;

/* Convert some window to dialog type */
#define DIALOG_OBJ(wnd)		((dialog_t *)wnd)

/* Create a new dialog */
dialog_t *dialog_new( wnd_t *parent, char *title ); 

/* Dialog window constructor */
bool_t dialog_construct( dialog_t *dlg, wnd_t *parent, char *title ); 

/* Find dialog item by its ID */
dlgitem_t *dialog_find_item( dialog_t *dlg, char *id );

/* Update dialog size (after some child changes its child) */
void dialog_update_size( dialog_t *dlg );

/* Arrange dialog items */
void dialog_arrange_children( dialog_t *dlg );

/* Dialog items iterator */
dlgitem_t *dialog_iterate_items( dialog_t *dlg, dlgitem_t *di, 
		dialog_iterate_flags_t flags );

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

/* Free message handlers */
void dialog_free_handlers( wnd_t *wnd );

/* Aliases for creating message data */
#define dialog_msg_ok_new		wnd_msg_noargs_new
#define dialog_msg_cancel_new	wnd_msg_noargs_new

#endif

/* End of 'wnd_dialog.h' file */

