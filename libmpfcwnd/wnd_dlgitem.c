/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_dlgitem.c
 * PURPOSE     : MPFC Window Library. Common dialog item 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'dlgitem'
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
#include "types.h"
#include "wnd.h"
#include "wnd_dlgitem.h"

/* Construct dialog item */
bool_t dlgitem_construct( dlgitem_t *di, char *title, char *id, wnd_t *parent, 
		int x, int y, int width, int height )
{
	wnd_t *wnd = WND_OBJ(di);
	assert(di);

	/* Initialize window part */
	if (!wnd_construct(wnd, title, parent, x, y, width, height, 
				WND_FLAG_OWN_DECOR))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "keydown", dlgitem_on_keydown);
	wnd_msg_add_handler(wnd, "destructor", dlgitem_destructor);

	/* Initialize dialog item fields */
	di->m_id = (id == NULL ? NULL : strdup(id));
	return TRUE;
} /* End of 'dlgitem_construct' function */

/* Destructor */
void dlgitem_destructor( wnd_t *wnd )
{
	dlgitem_t *di = DLGITEM_OBJ(wnd);
	if (di->m_id != NULL)
		free(di->m_id);
} /* End of 'dlgitem_destructor' function */

/* 'keydown' message handler */
wnd_msg_retcode_t dlgitem_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	/* Pass key to parent if it is in reposition mode */
	wnd_mode_t mode = wnd->m_parent->m_mode;
	if (mode == WND_MODE_REPOSITION || mode == WND_MODE_RESIZE)
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	return WND_MSG_RETCODE_OK;
} /* End of 'dlgitem_on_keydown' function */

/* End of 'wnd_dlgitem.c' file */

