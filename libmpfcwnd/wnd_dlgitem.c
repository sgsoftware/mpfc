/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Common dialog item functions implementation.
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

#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_dlgitem.h"

/* Construct dialog item */
bool_t dlgitem_construct( dlgitem_t *di, wnd_t *parent, char *title, char *id, 
		dlgitem_get_size_t get_size, dlgitem_set_pos_t set_pos,
		char letter, dlgitem_flags_t flags )
{
	wnd_t *wnd = WND_OBJ(di);
	wnd_class_t *klass;
	wnd_flags_t wnd_flags = 0;
	assert(di);

	/* Initialize window part */
	if (flags & DLGITEM_BORDER)
	{
		wnd_flags |= WND_FLAG_BORDER;
		if (title != NULL)
			wnd_flags |= WND_FLAG_CAPTION;
	}
	if (!wnd_construct(wnd, parent, title, 0, 0, 0, 0, wnd_flags))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "keydown", dlgitem_on_keydown);
	wnd_msg_add_handler(wnd, "action", dlgitem_on_action);
	wnd_msg_add_handler(wnd, "destructor", dlgitem_destructor);

	/* Initialize dialog item fields */
	di->m_id = (id == NULL ? NULL : strdup(id));
	di->m_get_size = get_size;
	di->m_set_pos = set_pos;
	di->m_letter = letter;
	di->m_flags = flags;
	for ( klass = parent->m_class; klass != NULL; klass = klass->m_parent )
	{
		if (!strcmp(klass->m_name, "dialog"))
		{
			di->m_dialog = parent;
			break;
		}
	}
	if (di->m_dialog == NULL)
		di->m_dialog = DLGITEM_OBJ(parent)->m_dialog;
	return TRUE;
} /* End of 'dlgitem_construct' function */

/* Destructor */
void dlgitem_destructor( wnd_t *wnd )
{
	dlgitem_t *di = DLGITEM_OBJ(wnd);
	if (di->m_id != NULL)
		free(di->m_id);
} /* End of 'dlgitem_destructor' function */

/* Get dialog item desired size */
void dlgitem_get_size( dlgitem_t *di, int *width, int *height )
{
	if (di->m_get_size == NULL)
	{
		*width = 0;
		*height = 0;
	}
	else
	{
		di->m_get_size(di, width, height);
	}
} /* End of 'dlgitem_get_size' function */

/* Set dialog item position */
void dlgitem_set_pos( dlgitem_t *di, int x, int y, int width, int height )
{
	wnd_repos_internal(WND_OBJ(di), x, y, width, height);
	if (di->m_set_pos != NULL)
	{
		di->m_set_pos(di, x, y, width, height);
	}
} /* End of 'dlgitem_set_pos' function */

/* 'keydown' message handler */
wnd_msg_retcode_t dlgitem_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	int without_alt = WND_KEY_IS_WITH_ALT(key);
	wnd_t *dlg = DLGITEM_OBJ(wnd)->m_dialog;

	/* Jumping to child */
	if (without_alt != 0)
	{
		dlgitem_t *child;
		for ( child = DLGITEM_OBJ(dlg->m_child);
				child != NULL; child = dialog_iterate_items(DIALOG_OBJ(dlg),
					child, DIALOG_ITERATE_ZORDER) )
		{
			if (!(child->m_flags & DLGITEM_NOTABSTOP) &&
					(child->m_letter == without_alt))
				break;
		}
		if (child != NULL)
		{
			wnd_set_focus(WND_OBJ(child));
			wnd_msg_send(WND_OBJ(child), "quick_change_focus",
					dlgitem_msg_quick_change_focus_new());
			return WND_MSG_RETCODE_STOP;
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'dlgitem_on_keydown' function */

/* 'action' message handler */
wnd_msg_retcode_t dlgitem_on_action( wnd_t *wnd, char *action )
{
	wnd_t *dlg = DLGITEM_OBJ(wnd)->m_dialog;

	/* Jump to the next window */
	if (!strcasecmp(action, "next"))
	{
		dlgitem_t *starting = DLGITEM_OBJ(WND_FOCUS(wnd)), *child;
		child = starting;
		do
		{
			child = dialog_iterate_items(DIALOG_OBJ(dlg), child, 
					DIALOG_ITERATE_CYCLE);
		} while (child != starting && (child->m_flags & DLGITEM_NOTABSTOP));
		wnd_set_focus(WND_OBJ(child));
	}
	/* Send 'ok_clicked' */
	else if (!strcasecmp(action, "ok"))
	{
		wnd_msg_send(dlg, "ok_clicked", dialog_msg_ok_new());
	}
	/* Send 'cancel_clicked' */
	else if (!strcasecmp(action, "cancel"))
	{
		wnd_msg_send(dlg, "cancel_clicked", dialog_msg_cancel_new());
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'dlgitem_on_action' function */

/* Initialize dialog item class */
wnd_class_t *dlgitem_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "dlgitem", 
			wnd_basic_class_init(global), dlgitem_get_msg_info, 
			dlgitem_free_handlers, dlgitem_class_set_default_styles);
	return klass;
} /* End of 'dlgitem_class_init' function */

/* Set dialog item class default styles */
void dlgitem_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "letter-color", "red");
	cfg_set_var(list, "kbind.next", "<Tab>");
	cfg_set_var(list, "kbind.ok", "<Enter>");
	cfg_set_var(list, "kbind.cancel", "<Escape>;<Ctrl-g>");
} /* End of 'dlgitem_class_set_default_styles' function */

/* Get message information */
wnd_msg_handler_t **dlgitem_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	if (!strcmp(msg_name, "quick_change_focus"))
	{
		if (callback != NULL)
			(*callback) = wnd_basic_callback_noargs;
		return &DLGITEM_OBJ(wnd)->m_on_quick_change_focus;
	}
	return NULL;
} /* End of 'dlgitem_get_msg_info' function */

/* Free message handlers */
void dlgitem_free_handlers( wnd_t *wnd )
{
	wnd_msg_free_handlers(DLGITEM_OBJ(wnd)->m_on_quick_change_focus);
} /* End of 'dlgitem_free_handlers' function */

/* End of 'wnd_dlgitem.c' file */

