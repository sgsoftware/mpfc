/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Interface for multiview dialog box 
 * functions.
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

#ifndef __SG_MPFC_WND_MULTIVIEW_DIALOG_H__
#define __SG_MPFC_WND_MULTIVIEW_DIALOG_H__

#include "types.h"
#include "wnd_dialog.h"
#include "wnd_hbox.h"
#include "wnd_vbox.h"
#include "wnd_views.h"

/* Multiview dialog window */
typedef struct
{
	/* Common dialog part */
	dialog_t m_wnd;

	/* Views switching buttons bar */
	hbox_t *m_switches;

	/* Views list */
	views_t *m_views;
} mview_dialog_t;

/* Convert a window object to multiview dialog type */
#define MVIEW_DIALOG_OBJ(wnd)	((mview_dialog_t *)wnd)

/* Create a new multiview dialog */
mview_dialog_t *mview_dialog_new( wnd_t *parent, char *title );

/* Construct multiview dialog */
bool_t mview_dialog_construct( mview_dialog_t *mvd, wnd_t *parent, 
		char *title );

/* Add a view */
void mview_dialog_add_view( mview_dialog_t *dlg, vbox_t *view, 
		char *title, char letter );

/* Handle pressing switching button */
void mview_dialog_on_switch_clicked( wnd_t *wnd );

/* Initialize class */
wnd_class_t *mview_dialog_class_init( wnd_global_data_t *global );

#endif

/* End of 'wnd_multiview_dialog.h' file */

