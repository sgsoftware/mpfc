/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_scrollable.h
 * PURPOSE     : SG MPFC. Interface for scrollable windows
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 18.10.2004
 * NOTE        : Module prefix 'scrollable'.
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

#ifndef __SG_MPFC_WND_SCROLLABLE_H__
#define __SG_MPFC_WND_SCROLLABLE_H__

#include "types.h"
#include "wnd.h"

/* Scrollable type */
typedef enum
{
	SCROLLABLE_VERTICAL,
	SCROLLABLE_HORIZONTAL
} scrollable_type_t;

/* Scrollable window type */
typedef struct tag_scrollable_t
{
	/* Common window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_scrolled;

	/* Scrollable type (vertical or horizontal) */
	scrollable_type_t m_type;

	/* Size of the list being scrolled */
	int m_list_size;

	/* Difference between client size and window size */
	int m_diff;

	/* Scroll value */
	int m_scroll;

	/* Get scroll range */
	int (*m_get_range)( struct tag_scrollable_t *scr );
} scrollable_t;

/* Convert window object to scrollable type */
#define SCROLLABLE_OBJ(wnd)	((scrollable_t *)wnd)

/* Get window size */
#define SCROLLABLE_WND_SIZE(scr) ((((scr)->m_type) == SCROLLABLE_VERTICAL ? \
		 WND_HEIGHT(scr) : WND_WIDTH(scr)) - (scr)->m_diff)

/* Get the scroll range */
#define SCROLLABLE_RANGE(scr) ((((scr)->m_get_range) != NULL) ? \
		((scr)->m_get_range(scr)) : \
		((scr)->m_list_size - SCROLLABLE_WND_SIZE(scr)))

/* Create a new scrollable window */
scrollable_t *scrollable_new( wnd_t *parent, char *title, int x, int y, int w,
		int h, scrollable_type_t type, wnd_flags_t flags );

/* Initialize scrollable */
bool_t scrollable_construct( scrollable_t *scr, wnd_t *parent, char *title, 
		int x, int y, int w, int h, scrollable_type_t type, wnd_flags_t flags );

/* 'display' message handler */
wnd_msg_retcode_t scrollable_on_display( wnd_t *wnd );

/* Scroll window */
void scrollable_scroll( scrollable_t *scr, int value, bool_t absolute );

/* Change list size */
void scrollable_set_size( scrollable_t *scr, int new_size );

/* 
 * Class functions
 */

/* Initialize scrollable window class */
wnd_class_t *scrollable_class_init( wnd_global_data_t *global );

/* Set default styles */
void scrollable_class_set_default_styles( cfg_node_t *node );

/* Get message information */
wnd_msg_handler_t **scrollable_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void scrollable_free_handlers( wnd_t *wnd );

/*
 * Types and functions for 'scrolled' message
 */

/* Type */
typedef struct
{
	int m_offset;
} scrollable_msg_scrolled_t;

/* Create data for 'scrolled' message */
wnd_msg_data_t scrollable_msg_scrolled_new( int offset );

/* Callback for this message */
wnd_msg_retcode_t scrollable_callback_scrolled( wnd_t *wnd,
		wnd_msg_handler_t *handler, wnd_msg_data_t *msg_data );

/* Convert handler pointer to the proper type */
#define SCROLLABLE_MSG_SCROLLED_HANDLER(h) \
	((wnd_msg_retcode_t (*)(wnd_t *, int))(h->m_func))

#endif

/* End of 'wnd_scrollable.h' file */

