/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_editbox.c
 * PURPOSE     : MPFC Window Library. Edit box functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2004
 * NOTE        : Module prefix 'editbox'.
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
#include "wnd_dlgitem.h"
#include "wnd_editbox.h"

/* Create a new edit box */
editbox_t *editbox_new( wnd_t *parent, char *id, int x, int y, int width, 
		int height )
{
	editbox_t *eb;

	/* Allocate memory */
	eb = (editbox_t *)malloc(sizeof(*eb));
	if (eb == NULL)
		return NULL;
	memset(eb, 0, sizeof(*eb));
	WND_OBJ(eb)->m_class = wnd_basic_class_init(WND_GLOBAL(parent));

	/* Initialize edit box */
	if (!editbox_construct(eb, parent, id, x, y, width, height))
	{
		free(eb);
		return NULL;
	}
	wnd_postinit(eb);
	return eb;
} /* End of 'editbox_new' function */

/* Edit box constructor */
bool_t editbox_construct( editbox_t *eb, wnd_t *parent, char *id, int x, int y, 
		int width, int height )
{
	wnd_t *wnd = WND_OBJ(eb);

	/* Initialize window part */
	if (!dlgitem_construct(DLGITEM_OBJ(eb), "", id, parent, x, y, 
				width, height))
		return FALSE;

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "display", editbox_on_display);
	wnd_msg_add_handler(wnd, "keydown", editbox_on_keydown);

	/* Initialize edit box fields */
	eb->m_text = (char *)malloc(256);
	strcpy(eb->m_text, "");
	eb->m_cursor = 0;
	eb->m_len = 0;
	return TRUE;
} /* End of 'editbox_construct' function */

/* Destructor */
void editbox_destructor( wnd_t *wnd )
{
	free(EDITBOX_OBJ(wnd)->m_text);
} /* End of 'editbox_destructor' function */

/* Set edit box text */
void editbox_set_text( editbox_t *eb, char *text )
{
	assert(eb);
	assert(text);
	strcpy(eb->m_text, text);
	wnd_invalidate(WND_OBJ(eb));
} /* End of 'editbox_set_text' function */

/* 'display' message handler */
wnd_msg_retcode_t editbox_on_display( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);

	assert(wnd);

	/* Print text */
	wnd_move(wnd, 0, 0, 0);
	wnd_printf(wnd, 0, 0, "%s\n", eb->m_text);

	/* Move cursor */
	wnd_move(wnd, 0, eb->m_cursor, 0);
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_display' function */

/* 'keydown' message handler */
wnd_msg_retcode_t editbox_on_keydown( wnd_t *wnd, wnd_key_t key )
{
	editbox_t *eb = EDITBOX_OBJ(wnd);
	char *text = eb->m_text;

	/* Append char to the text */
	if (key >= ' ' && key <= 0xFF)
	{
		memmove(&text[eb->m_cursor + 1], &text[eb->m_cursor],
				eb->m_len - eb->m_cursor + 1);
		text[eb->m_cursor] = key;
		eb->m_len ++;
		eb->m_cursor ++;
		wnd_invalidate(wnd);
	}
	/* Delete previous char */
	else if (key == KEY_BACKSPACE)
	{
		if (eb->m_cursor != 0)
		{
			memmove(&text[eb->m_cursor - 1], &text[eb->m_cursor],
					eb->m_len - eb->m_cursor + 1);
			eb->m_len --;
			eb->m_cursor --;
			wnd_invalidate(wnd);
		}
	}
	/* Move cursor */
	else if (key == KEY_RIGHT)
	{
		if (eb->m_cursor < eb->m_len)
		{
			eb->m_cursor ++;
			wnd_invalidate(wnd);
		}
	}
	else if (key == KEY_LEFT)
	{
		if (eb->m_cursor > 0)
		{
			eb->m_cursor --;
			wnd_invalidate(wnd);
		}
	}
	else if (key == KEY_HOME)
	{
		eb->m_cursor = 0;
		wnd_invalidate(wnd);
	}
	else if (key == KEY_DC)
	{
		eb->m_cursor = eb->m_len;
		wnd_invalidate(wnd);
	}
	else
		return WND_MSG_RETCODE_PASS_TO_PARENT;
	return WND_MSG_RETCODE_OK;
} /* End of 'editbox_on_keydown' function */

/* End of 'wnd_editbox.c' file */

