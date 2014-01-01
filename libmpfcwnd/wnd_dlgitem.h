/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for common dialog item functions.
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

#ifndef __SG_MPFC_WND_DLG_ITEM_H__
#define __SG_MPFC_WND_DLG_ITEM_H__

#include "types.h"
#include "wnd.h"

/* Dialog item flags type */
typedef enum
{
	DLGITEM_NOTABSTOP	= 1 << 0,
	DLGITEM_PACK_END	= 1 << 1,
	DLGITEM_BORDER		= 1 << 2,
} dlgitem_flags_t;

/* Functions for getting desired size for a dialog item and setting
 * its position */
struct tag_dlgitem_t;
typedef void (*dlgitem_get_size_t)( struct tag_dlgitem_t *wnd, int *width, 
		int *height );
typedef void (*dlgitem_set_pos_t)( struct tag_dlgitem_t *wnd, int x, int y,
		int width, int height );

/* Dialog item type */
typedef struct tag_dlgitem_t
{
	/* Window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_quick_change_focus;

	/* Item letter */
	char m_letter;

	/* Item ID string */
	char *m_id;

	/* Dialog this item belongs to */
	wnd_t *m_dialog;

	/* Item flags */
	dlgitem_flags_t m_flags;

	/* Get the desired window size and set its position functions */
	dlgitem_get_size_t m_get_size;
	dlgitem_set_pos_t m_set_pos;
} dlgitem_t;

/* Convert window object to dialog item type */
#define DLGITEM_OBJ(wnd)	((dlgitem_t *)wnd)

/* Access dialog item fields */
#define DLGITEM_FLAGS(wnd)	(DLGITEM_OBJ(wnd)->m_flags)

/* Construct dialog item */
bool_t dlgitem_construct( dlgitem_t *di, wnd_t *parent, char *title, char *id, 
		dlgitem_get_size_t get_size, dlgitem_set_pos_t set_pos, 
		char letter, dlgitem_flags_t flags );

/* Destructor */
void dlgitem_destructor( wnd_t *wnd );

/* Get dialog item desired size */
void dlgitem_get_size( dlgitem_t *di, int *width, int *height );

/* Set dialog item position */
void dlgitem_set_pos( dlgitem_t *di, int x, int y, int width, int height );

/* 'keydown' message handler */
wnd_msg_retcode_t dlgitem_on_keydown( wnd_t *wnd, wnd_key_t key );

/* 'action' message handler */
wnd_msg_retcode_t dlgitem_on_action( wnd_t *wnd, char *action );

/*
 * Class functions
 */

/* Initialize dialog item class */
wnd_class_t *dlgitem_class_init( wnd_global_data_t *global );

/* Set dialog item class default styles */
void dlgitem_class_set_default_styles( cfg_node_t *list );

/* Get message information */
wnd_msg_handler_t **dlgitem_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void dlgitem_free_handlers( wnd_t *wnd );

/* Aliases for message data creating */
#define dlgitem_msg_quick_change_focus_new	wnd_msg_noargs_new

#endif

/* End of 'wnd_dlgitem.h' file */

