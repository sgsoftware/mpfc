/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : logger_view.c
 * PURPOSE     : SG MPFC. Logger view window functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 22.09.2004
 * NOTE        : Module prefix 'logview'.
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
#include "logger.h"
#include "logger_view.h"
#include "wnd.h"

/* Create new logger view */
logger_view_t *logview_new( wnd_t *parent, logger_t *logger )
{
	logger_view_t *lv;
	
	/* Allocate memory */
	lv = (logger_view_t *)malloc(sizeof(*lv));
	if (lv == NULL)
		return NULL;
	memset(lv, 0, sizeof(*lv));
	WND_OBJ(lv)->m_class = logview_class_init(WND_GLOBAL(parent));

	/* Initialize window */
	if (!logview_construct(lv, parent, logger))
	{
		free(lv);
		return NULL;
	}
	wnd_postinit(lv);
	return lv;
} /* End of 'logview_new' function */

/* Initialize logger window */
bool_t logview_construct( logger_view_t *lv, wnd_t *parent, logger_t *logger )
{
	/* Initialize window part */
	if (!wnd_construct(WND_OBJ(lv), parent, _("Log"), 0, 0, 0, 0,
				WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(lv), "display", logview_on_display);
	wnd_msg_add_handler(WND_OBJ(lv), "destructor", logview_destructor);

	/* Set fields */
	lv->m_logger = logger;
	return TRUE;
} /* End of 'logview_construct' function */

/* Destructor */
void logview_destructor( wnd_t *wnd )
{
} /* End of 'logview_destructor' function */

/* Display logger window */
wnd_msg_retcode_t logview_on_display( wnd_t *wnd )
{
	logger_view_t *lv = LOGGER_VIEW(wnd);
	logger_t *logger = lv->m_logger;
	struct logger_message_t *msg;

	wnd_move(wnd, 0, 0, 0);
	for ( msg = logger->m_head; msg != NULL; msg = msg->m_next )
	{
		wnd_printf(wnd, 0, 0, "%s\n", msg->m_message);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'logview_on_display' function */

/* Initialize logger view class */
wnd_class_t *logview_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "logger_view",
			wnd_basic_class_init(global), NULL, 
			logview_class_set_default_styles);
	return klass;
} /* End of 'logview_class_init' function */

/* Set default styles */
void logview_class_set_default_styles( cfg_node_t *list )
{
} /* End of 'logview_class_set_default_styles' function */

/* End of 'logger_view.c' file */

