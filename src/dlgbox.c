/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : dlgbox.c
 * PURPOSE     : SG MPFC. Dialog box functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.09.2003
 * NOTE        : Module prefix 'dlg'.
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
#include "types.h"
#include "colors.h"
#include "dlgbox.h"
#include "error.h"
#include "util.h"
#include "window.h"

/* Create a new dialog box */
dlgbox_t *dlg_new( wnd_t *parent, int x, int y, int w, int h,
	   				char *caption )
{
	dlgbox_t *dlg;

	/* Allocate memory for dialog box */
	dlg = (dlgbox_t *)malloc(sizeof(dlgbox_t));
	if (dlg == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize dialog box */
	if (!dlg_init(dlg, parent, x, y, w, h, caption))
	{
		free(dlg);
		return NULL;
	}

	return dlg;
} /* End of 'dlg_new' function */

/* Initialize edit box */
bool_t dlg_init( dlgbox_t *dlg, wnd_t *parent, int x, int y, int w, int h,
	   				char *caption  )
{
	/* Initialize window part */
	if (!wnd_init(&dlg->m_wnd, parent, x, y, w, h))
		return FALSE;

	/* Register message handlers */
	wnd_register_handler(dlg, WND_MSG_DISPLAY, dlg_display);
	wnd_register_handler(dlg, WND_MSG_KEYDOWN, dlg_handle_key);
	((wnd_t *)dlg)->m_wnd_destroy = dlg_destroy;
	
	/* Save dialog box parameters */
	strcpy(dlg->m_caption, caption);
	dlg->m_caption[dlg->m_wnd.m_width - 5] = 0;
	dlg->m_cur_focus = NULL;
	dlg->m_ok = FALSE;
	WND_FLAGS(dlg) |= WND_DIALOG;
	return TRUE;
} /* End of 'dlg_init' function */

/* Destroy dialog box */
void dlg_destroy( wnd_t *wnd )
{
	/* Destroy window */
	wnd_destroy_func(wnd);
} /* End of 'dlg_destroy' function */

/* Dialog box display function */
void dlg_display( wnd_t *wnd, dword data )
{
	dlgbox_t *dlg = (dlgbox_t *)wnd;
	int i, j, len = strlen(dlg->m_caption);

	/* Draw window border */
	wnd_move(wnd, 0, 0);

	/* Print first line */
	wnd_set_attrib(wnd, A_BOLD);
	wnd_print_char(wnd, ACS_ULCORNER);
	for ( i = 1; i < (wnd->m_width - 2 - len) / 2 - 1; i ++ )
		wnd_print_char(wnd, ACS_HLINE);
	col_set_color(wnd, COL_EL_DLG_TITLE);
	wnd_printf(wnd, " %s ", dlg->m_caption);
	col_set_color(wnd, COL_EL_DEFAULT);
	wnd_set_attrib(wnd, A_BOLD);
	for ( i = wnd_getx(wnd); i < wnd->m_width - 2; i ++ )
		wnd_print_char(wnd, ACS_HLINE);
	wnd_print_char(wnd, ACS_URCORNER);
	wnd_print_char(wnd, '\n');

	/* Print other lines */
	for ( i = 1; i < wnd->m_height - 1; i ++ )
	{
		wnd_print_char(wnd, ACS_VLINE);
		for ( j = 1; j < wnd->m_width - 2; j ++ )
			wnd_print_char(wnd, ' ');
		wnd_print_char(wnd, ACS_VLINE);
		wnd_print_char(wnd, '\n');
	}

	/* Print last line */
	wnd_print_char(wnd, ACS_LLCORNER);
	for ( i = 1; i < wnd->m_width - 2; i ++ )
		wnd_print_char(wnd, ACS_HLINE);
	wnd_print_char(wnd, ACS_LRCORNER);
	wnd_print_char(wnd, '\n');
} /* End of 'dlg_display' function */

/* Dialog box key handler function  */
void dlg_handle_key( wnd_t *wnd, dword data )
{
	dlgbox_t *dlg = (dlgbox_t *)wnd;
	int key = (int)data;

	switch (key)
	{
	case '\n':
	case 27:
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;
	}
} /* End of 'dlg_handle_key' function */

/* Get item by its ID */
wnd_t *dlg_get_item_by_id( dlgbox_t *dlg, short id )
{
	wnd_t *wnd;
	
	if (dlg == NULL)
		return NULL;

	for ( wnd = ((wnd_t *)dlg)->m_child; wnd != NULL; wnd = wnd->m_next )
		if (wnd->m_id == id)
			return wnd;
	return NULL;
} /* End of 'dlg_get_item_by_id' function */

/* Go to the next child focused window */
void dlg_next_focus( dlgbox_t *dlg )
{
	if (dlg->m_cur_focus == NULL)
		dlg->m_cur_focus = WND_OBJ(dlg)->m_child;
	else
		dlg->m_cur_focus = 
			//((dlg->m_cur_focus->m_next != NULL) ? 
			 	dlg->m_cur_focus->m_next;// : WND_OBJ(dlg)->m_child);
} /* End of 'dlg_next_focus' function */

/* End of 'dlgbox.c' file */

