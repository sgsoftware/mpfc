/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : logger_view.c
 * PURPOSE     : SG MPFC. Logger view window functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 26.09.2004
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
#include "wnd_scrollable.h"

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
	scrollable_set_size(SCROLLABLE_OBJ(lv), logger->m_num_messages);
	return lv;
} /* End of 'logview_new' function */

/* Initialize logger window */
bool_t logview_construct( logger_view_t *lv, wnd_t *parent, logger_t *logger )
{
	scrollable_t *scr = SCROLLABLE_OBJ(lv);

	/* Initialize window part */
	if (!scrollable_construct(scr, parent, _("Log"), 0, 0, 0, 0,
				SCROLLABLE_VERTICAL, WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED))
		return FALSE;

	/* Set message map */
	wnd_msg_add_handler(WND_OBJ(lv), "display", logview_on_display);
	wnd_msg_add_handler(WND_OBJ(lv), "action", logview_on_action);
	wnd_msg_add_handler(WND_OBJ(lv), "scrolled", logview_on_scrolled);

	/* Set fields */
	scr->m_get_range = logview_get_scroll_range;
	lv->m_logger = logger;
	lv->m_top_message = logger->m_head;
	return TRUE;
} /* End of 'logview_construct' function */

/* Display logger window */
wnd_msg_retcode_t logview_on_display( wnd_t *wnd )
{
	logger_view_t *lv = LOGGER_VIEW(wnd);
	struct logger_message_t *msg;

	wnd_move(wnd, 0, 0, 0);
	wnd_apply_default_style(wnd);
	for ( msg = lv->m_top_message; msg != NULL; msg = msg->m_next )
	{
		logger_msg_type_t type = msg->m_type;
		int level = msg->m_level;
		if (level == LOGGER_LEVEL_DEBUG)
			wnd_apply_style(wnd, "logger-debug-style");
		else if (type == LOGGER_MSG_NORMAL)
			wnd_apply_style(wnd, "logger-normal-style");
		else if (type == LOGGER_MSG_STATUS)
			wnd_apply_style(wnd, "logger-status-style");
		else if (type == LOGGER_MSG_WARNING)
			wnd_apply_style(wnd, "logger-warning-style");
		else if (type == LOGGER_MSG_ERROR)
			wnd_apply_style(wnd, "logger-error-style");
		else if (type == LOGGER_MSG_FATAL)
			wnd_apply_style(wnd, "logger-fatal-style");
		wnd_printf(wnd, WND_PRINT_NOCLIP, 0, "%s\n", msg->m_message);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'logview_on_display' function */

/* 'action' message handler */
wnd_msg_retcode_t logview_on_action( wnd_t *wnd, char *action )
{
	scrollable_t *scr = SCROLLABLE_OBJ(wnd);

	/* Scroll */
	if (!strcasecmp(action, "scroll_down"))
	{
		scrollable_scroll(scr, 1, FALSE);
	}
	else if (!strcasecmp(action, "scroll_up"))
	{
		scrollable_scroll(scr, -1, FALSE);
	}
	else if (!strcasecmp(action, "screen_down"))
	{
		logview_move_pages(scr, 1);
	}
	else if (!strcasecmp(action, "screen_up"))
	{
		logview_move_pages(scr, -1);
	}
	else if (!strcasecmp(action, "scroll_to_start"))
	{
		scrollable_scroll(scr, 0, TRUE);
	}
	else if (!strcasecmp(action, "scroll_to_end"))
	{
		scrollable_scroll(scr, LOGGER_VIEW(wnd)->m_logger->m_num_messages,
				TRUE);
	}
	/* Close window */
	else if (!strcasecmp(action, "quit"))
	{
		wnd_close(wnd);
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'logview_on_action' function */

/* 'scrolled' message handler */
wnd_msg_retcode_t logview_on_scrolled( wnd_t *wnd, int offset )
{
	logger_view_t *lv = LOGGER_VIEW(wnd);

	/* Scroll logger message */
	if (offset > 0)
	{
		for ( ; offset > 0 && lv->m_top_message != NULL; offset -- )
			lv->m_top_message = lv->m_top_message->m_next;
	}
	else if (offset < 0)
	{
		for ( ; offset < 0 && lv->m_top_message != NULL; offset ++ )
			lv->m_top_message = lv->m_top_message->m_prev;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'logview_on_scrolled' function */

/* Scroll specified number of pages */
void logview_move_pages( scrollable_t *scr, int pages )
{
	int dir = (pages > 0) ? 1 : -1, delta;
	int max_dist = SCROLLABLE_WND_SIZE(scr) * abs(pages);
	int real_lines;
	struct logger_message_t *msg;
	logger_view_t *lv = LOGGER_VIEW(scr);
	
	/* Scroll number of items so that distance between previous and new
	 * top items is not more than (page size) * pages */
	for ( delta = 0, real_lines = 0, msg = lv->m_top_message; 
			real_lines <= max_dist && msg != NULL; delta ++, 
			((dir > 0) ? (msg = msg->m_next) : (msg = msg->m_prev)) )
		real_lines += logview_get_msg_lines(scr, msg);
	delta --;

	/* Do scroll */
	scrollable_scroll(scr, delta * dir, FALSE);
} /* End of 'logview_move_pages' function */

/* Get scroll range */
int logview_get_scroll_range( scrollable_t *scr )
{
	logger_view_t *lv = LOGGER_VIEW(scr);
	int range = lv->m_logger->m_num_messages;
	int lines;
	struct logger_message_t *msg;

	for ( lines = 0, msg = lv->m_logger->m_tail; 
			lines < SCROLLABLE_WND_SIZE(scr) && msg != NULL; 
			range --, msg = msg->m_prev )
	{
		lines += logview_get_msg_lines(scr, msg);
	}
	range ++;
	return range;
} /* End of 'logview_get_scroll_range' function */

/* Get number of lines a message occupies */
int logview_get_msg_lines( scrollable_t *scr, struct logger_message_t *msg )
{
	int x;
	int lines;
	char *text = msg->m_message;

	for ( x = 0, lines = 1; *text; text ++ )
	{
		if ((x >= WND_WIDTH(scr)) || ((*text) == '\n'))
		{
			x = 0;
			lines ++;
		}
		else
			x ++;
	}
	return lines;
} /* End of 'logview_get_msg_lines' function */

/* Initialize logger view class */
wnd_class_t *logview_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "logger_view",
			scrollable_class_init(global), NULL, 
			logview_class_set_default_styles);
	return klass;
} /* End of 'logview_class_init' function */

/* Set default styles */
void logview_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "logger-normal-style", "white:black");
	cfg_set_var(list, "logger-status-style", "cyan:black:bold");
	cfg_set_var(list, "logger-warning-style", "magenta:black:bold");
	cfg_set_var(list, "logger-error-style", "red:black:bold");
	cfg_set_var(list, "logger-fatal-style", "red:black:bold,blink");
	cfg_set_var(list, "logger-debug-style", "yellow:black");

	/* Set kbinds */
	cfg_set_var(list, "kbind.quit", "q;<Escape>");
	cfg_set_var(list, "kbind.scroll_up", "k;<Up>;<Ctrl-p>");
	cfg_set_var(list, "kbind.scroll_down", "j;<Down>;<Ctrl-n>");
	cfg_set_var(list, "kbind.screen_up", "u;<PageUp>;<Alt-v>");
	cfg_set_var(list, "kbind.screen_down", "<Space>;d;<PageDown>;<Ctrl-v>");
	cfg_set_var(list, "kbind.scroll_to_start", "g;<Ctrl-a>;<Home>");
	cfg_set_var(list, "kbind.scroll_to_end", "G;<Ctrl-e>;<End>");
} /* End of 'logview_class_set_default_styles' function */

/* End of 'logger_view.c' file */

