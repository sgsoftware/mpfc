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
#include "wnd_hbox.h"
#include "wnd_vbox.h"

/* Create a new dialog */
dialog_t *dialog_new( wnd_t *parent, char *title )
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
	if (!dialog_construct(dlg, parent, title))
	{
		free(dlg);
		return NULL;
	}
	WND_OBJ(dlg)->m_flags |= WND_FLAG_INITIALIZED;
	wnd_postinit(dlg);
	return dlg;
} /* End of 'dialog_new' function */

/* Dialog window constructor */
bool_t dialog_construct( dialog_t *dlg, wnd_t *parent, char *title )
{
	wnd_t *wnd = WND_OBJ(dlg);
	button_t *ok_btn, *cancel_btn;

	assert(dlg);

	/* Do window part initialization */
	if (!wnd_construct(wnd, parent, title, 0, 0, 0, 0, 
				(WND_FLAG_FULL_BORDER & (~WND_FLAG_MAX_BOX)) |
				WND_FLAG_NORESIZE))
		return FALSE;

	/* Install handlers */
	wnd_msg_add_handler(wnd, "ok_clicked", dialog_on_ok);
	wnd_msg_add_handler(wnd, "cancel_clicked", dialog_on_cancel);

	/* Create boxes for items */
	dlg->m_vbox = vbox_new(wnd, NULL, 0);
	dlg->m_hbox = hbox_new(WND_OBJ(dlg->m_vbox), NULL, 1);
	DLGITEM_FLAGS(dlg->m_hbox) |= DLGITEM_PACK_END;

	/* Create OK and Cancel buttons */
	ok_btn = button_new(WND_OBJ(dlg->m_hbox), _("OK"), "", 0);
	wnd_msg_add_handler(WND_OBJ(ok_btn), "clicked", dialog_ok_on_clicked);
	cancel_btn = button_new(WND_OBJ(dlg->m_hbox), _("Cancel"), "", 0);
	wnd_msg_add_handler(WND_OBJ(cancel_btn), "clicked", 
			dialog_cancel_on_clicked);
	return TRUE;
} /* End of 'dialog_construct' function */

/* Find dialog item by its ID */
dlgitem_t *dialog_find_item( dialog_t *dlg, char *id )
{
	dlgitem_t *child;

	assert(dlg);
	assert(id);

	for ( child = DLGITEM_OBJ(WND_OBJ(dlg)->m_child); child != NULL; 
			child = dialog_iterate_items(dlg, child, FALSE) )
	{
		if (child->m_id != NULL && !strcmp(child->m_id, id))
			return child;
	}
	return NULL;
} /* End of 'dialog_find_item' function */

/* Arrange dialog items */
void dialog_arrange_children( dialog_t *dlg )
{
	wnd_t *wnd = WND_OBJ(dlg);
	dlgitem_t *child;
	int width, height;
	int dlg_x, dlg_y, dlg_w, dlg_h;

	/* Dialog has no children */
	assert(dlg);
	if (wnd->m_child == NULL)
		return;

	/* Get the desired window size */
	child = DLGITEM_OBJ(wnd->m_child);
	dlgitem_get_size(child, &width, &height);

	/* Set our size */
	dlg_w = width + 2;
	dlg_h = height + 2;
	dlg_x = (WND_WIDTH(wnd->m_parent) - dlg_w) / 2;
	dlg_y = (WND_HEIGHT(wnd->m_parent) - dlg_h) / 2;
	wnd_repos_internal(wnd, dlg_x, dlg_y, dlg_w, dlg_h);

	/* Set child position */
	dlgitem_set_pos(child, 0, 0, width, height);

	/* Set focus to the first child */
	for ( child = DLGITEM_OBJ(WND_OBJ(dlg->m_vbox)->m_child); child != NULL; 
			child = DLGITEM_OBJ(WND_OBJ(child)->m_next) )
	{
		if (!(DLGITEM_FLAGS(child) & DLGITEM_PACK_END))
			break;
	}
	while (DLGITEM_FLAGS(child) & DLGITEM_NOTABSTOP)
		child = dialog_iterate_items(dlg, child, FALSE);
	wnd_set_focus(WND_OBJ(child));

	/* Repaint dialog */
	wnd_global_update_visibility(WND_ROOT(wnd));
	wnd_invalidate(wnd->m_parent);
} /* End of 'dialog_arrange_children' function */

/* Dialog items iterator */
dlgitem_t *dialog_iterate_items( dialog_t *dlg, dlgitem_t *di, bool_t cycle )
{
	wnd_t *wnd = WND_OBJ(di), *parent;

	assert(dlg);

	/* First try to continue with current item child */
	if (wnd->m_child != NULL)
		return DLGITEM_OBJ(wnd->m_child);

	/* Next try current item sibling */
	if (wnd->m_next != NULL)
		return DLGITEM_OBJ(wnd->m_next);

	/* Next our parent's sibling */
	for ( parent = wnd->m_parent; parent != WND_OBJ(dlg); 
			parent = parent->m_parent )
	{
		if (parent->m_next != NULL)
			return DLGITEM_OBJ(parent->m_next);
	}

	/* Full cycle is done */
	if (cycle)
		return DLGITEM_OBJ(WND_OBJ(dlg)->m_child);
	return NULL;
} /* End of 'dialog_iterate_items' function */

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
	wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "ok_clicked", dialog_msg_ok_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_ok_on_clicked' function */

/* Handle 'clicked' message for Cancel button */
wnd_msg_retcode_t dialog_cancel_on_clicked( wnd_t *wnd )
{
	wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "cancel_clicked", 
			dialog_msg_cancel_new());
	return WND_MSG_RETCODE_OK;
} /* End of 'dialog_cancel_on_clicked' function */

/* Create dialog class */
wnd_class_t *dialog_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "dialog", wnd_basic_class_init(global),
			dialog_get_msg_info, dialog_free_handlers, NULL);
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

/* Free message handlers */
void dialog_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(DIALOG_OBJ(wnd)->m_on_ok);
	wnd_msg_free_handlers(DIALOG_OBJ(wnd)->m_on_cancel);
} /* End of 'dialog_free_handlers' function */

/* End of 'wnd_dialog.c' file */

