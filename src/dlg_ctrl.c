/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : dlg_ctrl.c
 * PURPOSE     : SG MPFC. Dialog control items functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.11.2003
 * NOTE        : Module prefix 'dlgctrl'.
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
#include "dlg_ctrl.h"
#include "error.h"
#include "window.h"

/* Create a new dialog control */
dlgctrl_t *dlgctrl_new( wnd_t *parent, int x, int y, int type )
{
	dlgctrl_t *dc;

	/* Allocate memory for control */
	dc = (dlgctrl_t *)malloc(sizeof(dlgctrl_t));
	if (dc == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize control fields */
	if (!dlgctrl_init(dc, parent, x, y, type))
	{
		free(dc);
		return NULL;
	}
	return dc;
} /* End of 'dlgctrl_new' function */

/* Initialize dialog control */
bool_t dlgctrl_init( dlgctrl_t *dc, wnd_t *parent, int x, int y, byte type)
{
	/* Create common window part */
	if (!wnd_init(&dc->m_wnd, parent, x, y, 1, 1))
		return FALSE;

	/* Register message handlers */
	wnd_register_handler(dc, WND_MSG_DISPLAY, dlgctrl_display);
	wnd_register_handler(dc, WND_MSG_MOUSE_LEFT_CLICK, dlgctrl_handle_mouse);

	/* Set control-specific fields */
	dc->m_type = type;
	WND_OBJ(dc)->m_wnd_destroy = dlgctrl_destroy;
	WND_OBJ(dc)->m_flags |= (WND_ITEM | WND_INITIALIZED | WND_NO_FOCUS);
	return TRUE;
} /* End of 'dlgctrl_init' function */

/* Destroy dialog control */
void dlgctrl_destroy( wnd_t *wnd )
{
	/* Destroy window */
	wnd_destroy_func(wnd);
} /* End of 'dlgctrl_destroy' function */

/* Dialog control display function */
void dlgctrl_display( wnd_t *wnd, dword data )
{
	dlgctrl_t *dc = (dlgctrl_t *)wnd;
	
	if (wnd == NULL)
		return;

	wnd_move(wnd, 0, 0);
	wnd_print_char(wnd, dc->m_type == DLGCTRL_OK ? 'O' : 'X');
} /* End of 'dlgctrl_display' function */

/* Dialog control left mouse click handler */
void dlgctrl_handle_mouse( wnd_t *wnd, dword data )
{
	dlgctrl_t *dc = (dlgctrl_t *)wnd;

	/* Send message about respective key pressing to parent */
	if (wnd != NULL && wnd->m_parent != NULL && 
			(wnd->m_parent->m_flags & WND_DIALOG) &&
			wnd_focus != NULL && (wnd_focus == wnd->m_parent || 
				wnd_focus->m_parent == wnd->m_parent))
		wnd_send_msg(wnd_focus, WND_MSG_KEYDOWN, 
				dc->m_type == DLGCTRL_OK ? '\n' : 27);
} /* End of 'dlgctrl_handle_mouse' function */

/* End of 'dlg_ctrl.c' file */

