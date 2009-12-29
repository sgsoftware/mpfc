/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * MPFC Window Library. Repeat value input dialog.
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

#ifndef __SG_MPFC_WND_REPVAL_H__
#define __SG_MPFC_WND_REPVAL_H__

#include "types.h"
#include "wnd_dialog.h"

/* Create a repeat value dialog */
dialog_t *wnd_repval_new( wnd_t *parent, void *on_ok, int dig );

/* 'keydown' handler for repeat value dialog */
wnd_msg_retcode_t wnd_repval_on_keydown( wnd_t *wnd, wnd_key_t key );

#endif

/* End of 'wnd_repval.h' file */

