/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : button.c
 * PURPOSE     : SG MPFC. Button functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.02.2004
 * NOTE        : Module prefix 'btn'.
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
#include "button.h"
#include "colors.h"
#include "dlgbox.h"
#include "error.h"
#include "window.h"

/* Create a new button */
button_t *btn_new( wnd_t *parent, int x, int y, int w, char *text )
{
	button_t *btn;

	/* Allocate memory for button */
	btn = (button_t *)malloc(sizeof(button_t));
	if (btn == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize button fields */
	if (!btn_init(btn, parent, x, y, w, text))
	{
		free(btn);
		return NULL;
	}
	return btn;
} /* End of 'btn_new' function */

/* Initialize button */
bool_t btn_init( button_t *btn, wnd_t *parent, int x, int y, int w, char *text )
{
	/* Initialize window width */
	if (w == -1)
		w = strlen(text) + 2;
	
	/* Create common window part */
	if (!wnd_init(&btn->m_wnd, parent, x, y, w, 1))
		return FALSE;

	/* Register message handlers */
	wnd_register_handler(btn, WND_MSG_DISPLAY, btn_display);
	wnd_register_handler(btn, WND_MSG_KEYDOWN, btn_handle_key);
	wnd_register_handler(btn, WND_MSG_MOUSE_LEFT_CLICK, btn_handle_mouse);
	wnd_register_handler(btn, WND_MSG_POSTPONED_NOTIFY, btn_handle_pp_notify);

	/* Set button-specific fields */
	btn->m_text = strdup(text);
	WND_OBJ(btn)->m_wnd_destroy = btn_destroy;
	WND_OBJ(btn)->m_flags |= (WND_ITEM | WND_INITIALIZED);
	return TRUE;
} /* End of 'btn_init' function */

/* Destroy button */
void btn_destroy( wnd_t *wnd )
{
	button_t *btn = (button_t *)wnd;

	if (btn == NULL)
		return;

	if (btn->m_text != NULL)
		free(btn->m_text);

	/* Destroy window */
	wnd_destroy_func(wnd);
} /* End of 'btn_destroy' function */

/* Button display function */
void btn_display( wnd_t *wnd, dword data )
{
	button_t *btn = (button_t *)wnd;
	int num_spaces;
	
	if (wnd == NULL)
		return;

	/* Print button text centrized */
	num_spaces = (WND_WIDTH(wnd) - strlen(btn->m_text)) / 2;
	col_set_color(wnd, wnd_is_focused(wnd) ? COL_EL_BTN_FOCUSED : 
			COL_EL_BTN);
	wnd_move(wnd, 0, 0);
	wnd_printf(wnd, "%*c%s%*c", num_spaces, ' ', btn->m_text, num_spaces, ' ');
} /* End of 'btn_display' function */

/* Button key handler function */
void btn_handle_key( wnd_t *wnd, dword data )
{
	int key = (int)data;

	if (wnd == NULL)
		return;

	/* Click */
	if (key == ' ')
		wnd_send_msg(wnd->m_parent, WND_MSG_NOTIFY, 
				WND_NOTIFY_DATA(wnd->m_id, BTN_CLICKED));
	/* Common dialog box item actions */
	else DLG_ITEM_HANDLE_COMMON(wnd, key)
} /* End of 'btn_handle_key' function */

/* Button left mouse click handler */
void btn_handle_mouse( wnd_t *wnd, dword data )
{
	wnd_postponed_notify_data_t *pp_data;

	if (wnd == NULL)
		return;

	/* Call base dialog item handler */
	if (!dlg_item_handle_mouse(wnd))
		return;

	/* Execute button action */
	pp_data = (wnd_postponed_notify_data_t *)malloc(sizeof(*pp_data));
	pp_data->m_whom = wnd->m_parent;
	pp_data->m_data = WND_NOTIFY_DATA(wnd->m_id, BTN_CLICKED);
	wnd_send_msg(wnd, WND_MSG_POSTPONED_NOTIFY, (dword)pp_data);
} /* End of 'btn_handle_mouse' function */

/* Postponed notify message handler */
void btn_handle_pp_notify( wnd_t *wnd, dword data )
{
	wnd_postponed_notify_data_t *pp_data = (wnd_postponed_notify_data_t *)data;
	wnd_t *whom = pp_data->m_whom;
	dword d = pp_data->m_data;

	/* Free memory */
	free(pp_data);

	/* Send notification message */
	wnd_send_msg(whom, WND_MSG_NOTIFY, d);
} /* End of 'btn_handle_pp_notify' function */

/* End of 'button.c' file */

