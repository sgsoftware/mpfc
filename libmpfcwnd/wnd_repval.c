/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * MPFC Window Library. Repeat value input dialog.
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

#include "types.h"
#include "cfg.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"
#include "wnd_repval.h"
#include "wnd_label.h"

/* Create a repeat value dialog */
dialog_t *wnd_repval_new( wnd_t *parent, void *on_ok, int dig )
{
	dialog_t *dlg;
	hbox_t *hbox;
	editbox_t *eb;
	char text[2];
	assert(dig >= 0 && dig <= 9);

	dlg = dialog_new(parent, _("Repeat value"));
	hbox = hbox_new(WND_OBJ(dlg->m_vbox), NULL, 0);
	label_new(WND_OBJ(hbox), _("Enter count &value for the next command: "),
			NULL, 0);
	text[0] = dig + '0';
	text[1] = 0;
	cfg_set_var_int(WND_OBJ(dlg)->m_cfg_list, "last-key", 0);
	eb = editbox_new(WND_OBJ(hbox), "count", text, 'v', 10);
	wnd_msg_add_handler(WND_OBJ(eb), "keydown", wnd_repval_on_keydown);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", on_ok);
	dialog_arrange_children(dlg);
	return dlg;
} /* End of 'wnd_repval_new' function */

/* 'keydown' handler for repeat value dialog */
wnd_msg_retcode_t wnd_repval_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Let only digits be entered */
	if ((key >= ' ' && key < '0') || (key > '9' && key <= 0xFF))
	{
		cfg_set_var_int(WND_OBJ(DLGITEM_OBJ(wnd)->m_dialog)->m_cfg_list, 
				"last-key", key);
		wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "ok_clicked", 
				dialog_msg_ok_new());
		return WND_MSG_RETCODE_STOP;
	}
	else if (key == KEY_BACKSPACE && EDITBOX_OBJ(wnd)->m_cursor == 0)
	{
		wnd_msg_send(DLGITEM_OBJ(wnd)->m_dialog, "cancel_clicked",
				dialog_msg_cancel_new());
		return WND_MSG_RETCODE_STOP;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_repval_on_keydown' function */

/* End of 'wnd_repval.c' file */

