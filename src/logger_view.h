/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : logger_view.h
 * PURPOSE     : SG MPFC. Interface for logger view window 
 *               functions.
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

#ifndef __SG_MPFC_LOGGER_VIEW_H__
#define __SG_MPFC_LOGGER_VIEW_H__

#include "types.h"
#include "logger.h"
#include "wnd.h"
#include "wnd_scrollable.h"

/* Logger view window type */
typedef struct
{
	/* Window part */
	scrollable_t m_wnd;

	/* Corresponding logger object */
	logger_t *m_logger;

	/* Top message */
	struct logger_message_t *m_top_message;
} logger_view_t;

/* Convert window object to logger view type */
#define LOGGER_VIEW(wnd)	((logger_view_t *)wnd)

/* Create new logger view */
logger_view_t *logview_new( wnd_t *parent, logger_t *logger );

/* Initialize logger window */
bool_t logview_construct( logger_view_t *lv, wnd_t *parent, logger_t *logger );

/* Destructor */
void logview_destructor( wnd_t *wnd );

/* Display logger window */
wnd_msg_retcode_t logview_on_display( wnd_t *wnd );

/* 'action' message handler */
wnd_msg_retcode_t logview_on_action( wnd_t *wnd, char *action );

/* 'scrolled' message handler */
wnd_msg_retcode_t logview_on_scrolled( wnd_t *wnd, int offset );

/* Scroll specified number of pages */
void logview_move_pages( scrollable_t *scr, int pages );

/* Get scroll range */
int logview_get_scroll_range( scrollable_t *scr );

/* Get number of lines a message occupies */
int logview_get_msg_lines( scrollable_t *scr, struct logger_message_t *msg );

/*
 * Class functions
 */

/* Initialize logger view class */
wnd_class_t *logview_class_init( wnd_global_data_t *global );

/* Set default styles */
void logview_class_set_default_styles( cfg_node_t *list );

#endif

/* End of 'logger_view.h' file */

