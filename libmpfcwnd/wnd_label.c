/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Label functions implementation.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_dlgitem.h"
#include "wnd_label.h"

/* Create a new label */
label_t *label_new( wnd_t *parent, char *text, char *id, label_flags_t flags )
{
	label_t *l;

	/* Allocate memory */
	l = (label_t *)malloc(sizeof(*l));
	if (l == NULL)
		return NULL;
	memset(l, 0, sizeof(*l));
	WND_OBJ(l)->m_class = label_class_init(WND_GLOBAL(parent));

	/* Initialize label */
	if (!label_construct(l, parent, text, id, flags))
	{
		free(l);
		return NULL;
	}
	WND_FLAGS(l) |= WND_FLAG_INITIALIZED;
	return l;
} /* End of 'label_new' function */

/* Label constructor */
bool_t label_construct( label_t *l, wnd_t *parent, char *text, char *id,
		label_flags_t flags )
{
	/* Initialize dialog item part */
	if (!dlgitem_construct(DLGITEM_OBJ(l), parent, text, id, 
				label_get_desired_size, NULL, 0, DLGITEM_NOTABSTOP))
		return FALSE;

	/* Set styles */
	if (!(flags & LABEL_NOBOLD))
	{
		cfg_set_var(WND_OBJ(l)->m_cfg_list, "text-style", "white:black:bold");
		cfg_set_var(WND_OBJ(l)->m_cfg_list, "focus-text-style", 
				"white:black:bold");
	}

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(l), "display", label_on_display);
	l->m_text = WND_OBJ(l)->m_title;
	l->m_flags = flags;
	return TRUE;
} /* End of 'label_construct' function */

/* Create a label with another label */
label_t *label_new_with_label( wnd_t *parent, char *title, char *text,
		char *id, label_flags_t flags )
{
	hbox_t *hbox;
	hbox = hbox_new(parent, NULL, 0);
	label_new(WND_OBJ(hbox), title, NULL, 0);
	return label_new(WND_OBJ(hbox), text, id, flags);
} /* End of 'label_new_with_label' function */

/* Set label text */
void label_set_text( label_t *l, char *text )
{
	wnd_set_title(WND_OBJ(l), text);
	l->m_text = text;
	dialog_update_size(DIALOG_OBJ(DLGITEM_OBJ(l)->m_dialog));
} /* End of 'label_set_text' function */

/* Calculate size desired by this label */
void label_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	label_t *l = LABEL_OBJ(di);
	int i, lines = 0, max_width = 0, w = 0;

	for ( i = 0; i <= strlen(l->m_text); i ++ )
	{
		if (l->m_text[i] == '&')
			continue;
		if (l->m_text[i] == '\n' || l->m_text[i] == 0)
		{
			lines ++;
			if (w > max_width)
				max_width = w;
			w = 0;
		}
		else
			w ++;
	}
	*width = max_width;
	*height = lines;
} /* End of 'label_get_desired_size' function */

/* 'display' message handler */
wnd_msg_retcode_t label_on_display( wnd_t *wnd )
{
	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	dlgitem_display_label_text(wnd, wnd->m_title);
    return WND_MSG_RETCODE_OK;
} /* End of 'label_on_display' function */

/* Initialize label class */
wnd_class_t *label_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "label", dlgitem_class_init(global), NULL,
			NULL, NULL);
} /* End of 'label_class_init' function */

/* End of 'wnd_label.c' file */

