/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Multiview dialog box functions 
 * implementation.
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
#include "wnd_button.h"
#include "wnd_dialog.h"
#include "wnd_hbox.h"
#include "wnd_multiview_dialog.h"
#include "wnd_vbox.h"
#include "wnd_views.h"

/* Create a new multiview dialog */
mview_dialog_t *mview_dialog_new( wnd_t *parent, char *title )
{
	mview_dialog_t *mvd;

	/* Allocate memory */
	mvd = (mview_dialog_t *)malloc(sizeof(*mvd));
	if (mvd == NULL)
		return NULL;
	memset(mvd, 0, sizeof(*mvd));
	WND_OBJ(mvd)->m_class = mview_dialog_class_init(WND_GLOBAL(parent));

	/* Construct dialog */
	if (!mview_dialog_construct(mvd, parent, title))
	{
		free(mvd);
		return NULL;
	}
	WND_FLAGS(mvd) |= WND_FLAG_INITIALIZED;
	return mvd;
} /* End of 'mview_dialog_new' function */

/* Construct multiview dialog */
bool_t mview_dialog_construct( mview_dialog_t *mvd, wnd_t *parent, 
		char *title )
{
	/* Construct dialog part */
	if (!dialog_construct(DIALOG_OBJ(mvd), parent, title))
		return FALSE;

	/* Create switches bar */
	mvd->m_switches = hbox_new(WND_OBJ(DIALOG_OBJ(mvd)->m_vbox), NULL, 1);

	/* Create views list */
	mvd->m_views = views_new(WND_OBJ(DIALOG_OBJ(mvd)->m_vbox), NULL);
	DIALOG_OBJ(mvd)->m_first_branch = WND_OBJ(mvd->m_views);
	return TRUE;
} /* End of 'mview_dialog_construct' function */

/* Add a view */
void mview_dialog_add_view( mview_dialog_t *mvd, vbox_t *view, 
		char *title, char letter )
{
	button_t *btn;

	/* Create switch button */
	btn = button_new(WND_OBJ(mvd->m_switches), title, NULL, letter);
	cfg_set_var_ptr(WND_OBJ(btn)->m_cfg_list, "connected_view", view);
	wnd_msg_add_handler(WND_OBJ(btn), "clicked",
			mview_dialog_on_switch_clicked);
} /* End of 'mview_dialog_add_view' function */

/* Handle pressing switching button */
void mview_dialog_on_switch_clicked( wnd_t *wnd )
{
	wnd_t *view;
	button_t *btn = BUTTON_OBJ(wnd);

	/* Get the connected view */
	view = (wnd_t *)cfg_get_var_ptr(wnd->m_cfg_list, "connected_view");
	assert(view);
	wnd_set_focus(view);
} /* End of 'mview_dialog_on_switch_clicked' function */

/* Initialize class */
wnd_class_t *mview_dialog_class_init( wnd_global_data_t *global )
{
	return wnd_class_new(global, "multiview_dialog", 
			dialog_class_init(global), NULL, NULL, NULL);
} /* End of 'mview_dialog_class_init' function */

/* End of 'wnd_multiview_dialog.c' file */

