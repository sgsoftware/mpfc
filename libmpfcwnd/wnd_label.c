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
#include "util.h"

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
	label_text_parse(&l->m_text, text);

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
	wnd_msg_add_handler(WND_OBJ(l), "destructor", label_destructor);
	l->m_flags = flags;
	return TRUE;
} /* End of 'label_construct' function */

/* Destructor */
void label_destructor( wnd_t *wnd )
{
	label_text_free(&LABEL_OBJ(wnd)->m_text);
} /* End of 'combo_destructor' function */

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
	label_text_free(&l->m_text);
	label_text_parse(&l->m_text, text);
	dialog_update_size(DIALOG_OBJ(DLGITEM_OBJ(l)->m_dialog));
} /* End of 'label_set_text' function */

/* Parse label title */
void label_text_parse( label_text_t *text, char *str )
{
	/* Initialize */
	text->pre = strdup(str);
	text->letter = 0;
	text->post = "";

	/* Find the position of letter (marked by '&') and split by it */
	char *p = strchr(text->pre, '&');
	if (p)
	{
		text->letter = *(p + 1);
		(*p) = 0;

		if (text->letter)
			text->post = p + 2;
	}

	/* Calculate width and height */
	int max_width = 0;
	int lines = 0;
	char *s0 = text->pre;
	int w = 0;
	for (char *s = s0;;)
	{
		/* Extract a line */
		char *nl = strchr(s, '\n');
		if (nl)
			(*nl) = 0;

		w += utf8_width(s);
		if (w > max_width)
			max_width = w;

		if (nl)
		{
			(*nl) = '\n';
			s = nl + 1;
			++lines;
			w = 0;
			continue;
		}

		/* This string has ended */
		if (s0 == text->pre)
		{
			/* We may have letter+post */
			if (text->letter)
			{
				++w; /* letter */
				s0 = text->post;
				s = s0;
				continue;
			}
		}

		/* If we reach here, we finished iterating */
		++lines;
		break;
	}
	text->width = max_width;
	text->height = lines;
} /* End of 'label_text_parse' function */

/* Free label title struct */
void label_text_free( label_text_t *text )
{
	/* 'pre' points to the original string */
	if (text->pre)
		free(text->pre);
} /* End of 'label_text_free' function */

/* Calculate size desired by this label */
void label_get_desired_size( dlgitem_t *di, int *width, int *height )
{
	label_t *l = LABEL_OBJ(di);

	*width = l->m_text.width;
	*height = l->m_text.height;
} /* End of 'label_get_desired_size' function */

/* Display a label-like text */
void label_text_display( wnd_t *wnd, label_text_t *text )
{
	wnd_putstring(wnd, 0, 0, text->pre);
	if (text->letter)
	{
		char *color = wnd_get_style(wnd, "letter-color");
		wnd_push_state(wnd, WND_STATE_COLOR);
		if (color != NULL)
			wnd_set_fg_color(wnd, wnd_string2color(color));
		wnd_putchar(wnd, 0, text->letter);
		wnd_pop_state(wnd);

		wnd_putstring(wnd, 0, 0, text->post);
	}
} /* End of 'label_text_display' function */

/* 'display' message handler */
wnd_msg_retcode_t label_on_display( wnd_t *wnd )
{
	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	label_text_display(wnd, &LABEL_OBJ(wnd)->m_text);
    return WND_MSG_RETCODE_OK;
} /* End of 'label_on_display' function */

/* Initialize label class */
wnd_class_t *label_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "label", dlgitem_class_init(global), NULL,
			NULL, NULL);
} /* End of 'label_class_init' function */

/* End of 'wnd_label.c' file */

