/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_dialog.c
 * PURPOSE     : MPFC Window Library. Dialog functions 
 *               implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "wnd.h"
#include "wnd_button.h"
#include "wnd_dialog.h"
#include "wnd_dlgitem.h"

/* Create a new dialog */
dialog_t *dialog_new( char *title, wnd_t *parent, int x, int y,
		int width, int height, dword flags )
{
	dialog_t *dlg;
	wnd_class_t *klass;

	/* Allocate memory for dialog */
	dlg = (dialog_t *)malloc(sizeof(*dlg));
	if (dlg == NULL)
		return NULL;
	memset(dlg, 0, sizeof(*dlg));

	/* Initialize dialog class */
	klass = dialog_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
	{
		free(dlg);
		return NULL;
	}
	WND_OBJ(dlg)->m_class = klass;

	/* Initialize window */
	if (!dialog_construct(dlg, title, parent, x, y, width, height, flags))
	{
		free(dlg);
		return NULL;
	}
	wnd_postinit(dlg);
	return dlg;
} /* End of 'dialog_new' function */

/* Dialog window constructor */
bool_t dialog_construct( dialog_t *dlg, char *title, wnd_t *parent, 
		int x, int y, int width, int height, dword flags )
{
	wnd_t *wnd = WND_OBJ(dlg);
	button_t *ok_btn, *cancel_btn;

	assert(dlg);

	/* Do window part initialization */
	flags |= WND_FLAG_FULL_BORDER;
	if (!wnd_construct(wnd, title, parent, x, y, width, height, flags))
		return FALSE;

	/* Install handlers */
	wnd_msg_add_handler(wnd, "keydown", dialog_on_keydown);
	wnd_msg_add_handler(wnd, "ok_clicked", dialog_on_ok);
	wnd_msg_add_handler(wnd, "cancel_clicked", dialog_on_cancel);

	/* Create OK and Cancel buttons */
	ok_btn = button_new("OK", NULL, wnd, 0, WND_HEIGHT(wnd) - 1, 8, 1);
	wnd_msg_add_handler(WND_OBJ(ok_btn), "clicked", dialog_ok_on_clicked);
	cancel_btn = button_new("Cancel", NULL, wnd, 9, WND_HEIGHT(wnd) - 1, 8, 1);
	wnd_msg_add_handler(WND_OBJ(cancel_btn), "clicked", 
			dialog_cancel_on_clicked);
	return TRUE;
} /* End of 'dialog_construct' function */

/* Find dialog item by its ID */
wnd_t *dialog_find_item( dialog_t *dlg, char *id )
{
	wnd_t *child;

	assert(dlg);
	assert(id);

	for ( child = WND_OBJ(dlg)->m_child; child != NULL; 
			child = child->m_next )
	{
		dlgitem_t *di = DLGITEM_OBJ(child);
		if (di->m_id != NULL && !strcmp(di->m_id, id))
			return child;
	}
	return NULL;
} /* End of 'dialog_find_item' function */

/* Handle 'keydown' message */
wnd_msg_retcode_t dialog_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Tab jumps to the next window */
	if (key == '\t')
	{
		wnd_t *focus = wnd->m_focus_child;
		if (focus != NULL);
		{
			wnd_set_focus(focus->m_next == NULL ? wnd->m_child : focus->m_next);
		}
	}
	/* Enter initiates 'ok_clicked' message sending */
	else if (key == '\n')
	{
		wnd_msg_send(wnd, "ok_clicked", dialog_msg_ok_new());
	}
	/* Escape initiates 'cancel_clicked' message sending */
	else if (key == KEY_ESCAPE)
	{
		wnd_msg_send(wnd, "cancel_clicked", dialog_msg_cancel_new());
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_on_keydown' function */

/* Handle 'ok_clicked' message */
wnd_msg_retcode_t dialog_on_ok( wnd_t *wnd )
{
	wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_on_ok' function */

/* Handle 'cancel_clicked' message */
wnd_msg_retcode_t dialog_on_cancel( wnd_t *wnd )
{
	wnd_close(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_on_cancel' function */

/* Handle 'clicked' message for OK button */
wnd_msg_retcode_t dialog_ok_on_clicked( wnd_t *wnd )
{
	wnd_msg_send(wnd->m_parent, "ok_clicked", dialog_msg_ok_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_ok_on_clicked' function */

/* Handle 'clicked' message for Cancel button */
wnd_msg_retcode_t dialog_cancel_on_clicked( wnd_t *wnd )
{
	wnd_msg_send(wnd->m_parent, "cancel_clicked", dialog_msg_cancel_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_cancel_on_clicked' function */

/* Create dialog class */
wnd_class_t *dialog_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "dialog", wnd_basic_class_init(global),
			dialog_get_msg_info);
} /* End of 'dialog_class_init' function */

/* Get message information */
wnd_msg_handler_t **dialog_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "ok_clicked"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &DIALOG_OBJ(wnd)->m_on_ok;
	}
	else if (!strcmp(msg_name, "cancel_clicked"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &DIALOG_OBJ(wnd)->m_on_cancel;
	}
	return NULL;
} /* End of 'dialog_get_msg_info' function */

/* End of 'wnd_dialog.c' file */

