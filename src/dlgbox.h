/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : dlgbox.h
 * PURPOSE     : SG MPFC. Interface for dialog box functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.11.2003
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

#ifndef __SG_MPFC_DLG_BOX_H__
#define __SG_MPFC_DLG_BOX_H__

#include "types.h"
#include "window.h"

/* Common dialog box items key handling routine */
#define DLG_ITEM_HANDLE_COMMON(wnd, key) 								\
	if (((wnd)->m_parent != NULL) && 									\
			((wnd)->m_parent->m_flags & WND_DIALOG)						\
			&& ((key) == '\n' || (key) == '\t' || (key) == 27))			\
	{																	\
		switch (key) 													\
		{																\
		case '\n':														\
			((dlgbox_t *)((wnd)->m_parent))->m_ok = TRUE;				\
		case 27:														\
			wnd_send_msg(wnd, WND_MSG_CLOSE, 0);						\
			break;														\
		case '\t':														\
			wnd_send_msg(wnd, WND_MSG_CHANGE_FOCUS, 0);					\
			break;														\
		}																\
	}

/* Dialog box type */
typedef struct
{
	/* Window object */
	wnd_t m_wnd;

	/* Dialog box caption */
	char *m_caption;

	/* Current focused item */
	wnd_t *m_cur_focus;

	/* Flag of OK button pressing */
	bool_t m_ok;
} dlgbox_t;

/* Create a new dialog box */
dlgbox_t *dlg_new( wnd_t *parent, int x, int y, int w, int h,
	   				char *caption );

/* Initialize edit box */
bool_t dlg_init( dlgbox_t *dlg, wnd_t *parent, int x, int y, int w, int h,
	   				char *caption  );

/* Destroy dialog box */
void dlg_destroy( wnd_t *wnd );

/* Dialog box display function */
void dlg_display( wnd_t *wnd, dword data );

/* Dialog box key handler function  */
void dlg_handle_key( wnd_t *wnd, dword data );

/* Get item by its ID */
wnd_t *dlg_get_item_by_id( dlgbox_t *dlg, short id );

/* Go to the next child focused window */
void dlg_next_focus( dlgbox_t *dlg );

/* A base mouse click handler for dialog items */
bool_t dlg_item_handle_mouse( wnd_t *wnd );

#endif

/* End of 'dlgbox.h' file */

