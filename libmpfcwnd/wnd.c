/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd.c
 * PURPOSE     : MPFC Window Library. Window functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 24.07.2004
 * NOTE        : Module prefix 'wnd'.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "types.h"
#include "cfg.h"
#include "wnd.h"

/* Initialize window system and create root window */
wnd_t *wnd_init( cfg_node_t *cfg_list )
{
	WINDOW *wnd;
	wnd_t *wnd_root;
	cfg_node_t *cfg_wnd;
	wnd_global_data_t *global;
	wnd_kbd_data_t *kbd_data;
	wnd_msg_queue_t *msg_queue;
	wnd_mouse_driver_t mouse_type;
	int i, len;

	/* Initialize NCURSES */
	wnd = initscr();
	if (wnd == NULL)
		return NULL;
	start_color();
	cbreak();
	noecho();
	keypad(wnd, TRUE);
	nodelay(wnd, TRUE);

	/* Initialize color pairs */
	wnd_init_pairs();

	/* Create root window */
	wnd_root = (wnd_t *)malloc(sizeof(wnd_t));
	if (wnd_root == NULL)
	{
		endwin();
		return NULL;
	}
	memset(wnd_root, 0, sizeof(*wnd_root));
	wnd_root->m_title = strdup("root");
	wnd_root->m_flags = WND_FLAG_ROOT | WND_FLAG_OWN_DECOR;
	wnd_root->m_child = wnd_root->m_next = wnd_root->m_prev = 
		wnd_root->m_parent = NULL;
	wnd_root->m_x = wnd_root->m_screen_x = 0;
	wnd_root->m_y = wnd_root->m_screen_y = 0;
	wnd_root->m_width = COLS;
	wnd_root->m_height = LINES;
	wnd_root->m_client_x = 0;
	wnd_root->m_client_y = 0;
	wnd_root->m_client_w = wnd_root->m_width;
	wnd_root->m_client_h = wnd_root->m_height - 1;
	wnd_root->m_cursor_x = wnd_root->m_cursor_y = 0;
	wnd_root->m_fg_color = WND_COLOR_WHITE;
	wnd_root->m_bg_color = WND_COLOR_BLACK;
	wnd_root->m_attrib = 0;
	wnd_root->m_zval = 0;
	wnd_root->m_num_children = 0;
	wnd_root->m_focus_child = NULL;
	wnd_root->m_mode = WND_MODE_NORMAL;
	wnd_root->m_cursor_hidden = TRUE;

	/* Set root window specific handlers and the destructor */
	wnd_msg_add_handler(&wnd_root->m_on_keydown, wnd_root_on_key);
	wnd_msg_add_handler(&wnd_root->m_on_display, wnd_root_on_display);
	wnd_msg_add_handler(&wnd_root->m_on_close, wnd_root_on_close);
	wnd_msg_add_handler(&wnd_root->m_destructor, wnd_root_destructor);

	/* Initialize message queue */
	msg_queue = wnd_msg_queue_init();

	/* Initialize keyboard module */
	kbd_data = wnd_kbd_init(wnd_root);

	/* Initialize global data */
	global = (wnd_global_data_t *)malloc(sizeof(wnd_global_data_t));
	global->m_root = wnd_root;
	global->m_curses_wnd = wnd;
	global->m_focus = wnd_root;
	global->m_last_id = 0;
	global->m_states_stack_pos = 0;
	global->m_kbd_data = kbd_data;
	global->m_msg_queue = msg_queue;
	global->m_mouse_data.m_root_wnd = wnd_root;
	wnd_root->m_global = global;

	/* Initialize display buffer */
	global->m_display_buf.m_width = COLS;
	global->m_display_buf.m_height = LINES;
	len = COLS * LINES;
	global->m_display_buf.m_data = (struct wnd_display_buf_symbol_t *)malloc(
			len * sizeof(*global->m_display_buf.m_data));
	for ( i = 0; i < len; i ++ )
	{
		global->m_display_buf.m_data[i].m_char = ' ';
		global->m_display_buf.m_data[i].m_attr = 0;
	}

	/* Initialize mouse */
	global->m_mouse_data.m_driver = wnd_get_mouse_type(cfg_list);
	wnd_mouse_init(&global->m_mouse_data);

	/* Initialize configuration */
	cfg_wnd = cfg_new_list(cfg_list, "windows", CFG_NODE_MEDIUM_LIST |
			CFG_NODE_RUNTIME, 0);
	cfg_set_var(cfg_wnd, "caption-style", "green:black:bold");
	cfg_set_var(cfg_wnd, "border-style", "white:black:bold");
	cfg_set_var(cfg_wnd, "repos-border-style", "green:black:bold");
	cfg_set_var(cfg_wnd, "maximize-box-style", "red:black:bold");
	cfg_set_var(cfg_wnd, "close-box-style", "red:black:bold");
	cfg_set_var(cfg_wnd, "wndbar-style", "black:white");
	cfg_set_var(cfg_wnd, "wndbar-focus-style", "black:green");
	wnd_root->m_cfg_list = cfg_new_list(cfg_wnd, "0", 
			CFG_NODE_MEDIUM_LIST | CFG_NODE_RUNTIME, 0);

	/* Send display message to this window */
	wnd_invalidate(wnd_root);
	return wnd_root;
} /* End of 'wnd_init' function */

/* Create a window */
wnd_t *wnd_new( char *title, wnd_t *parent, int x, int y, 
				int width, int height, dword flags )
{
	wnd_t *wnd;

	/* Allocate memory */
	wnd = (wnd_t *)malloc(sizeof(wnd_t));
	if (wnd == NULL)
		return NULL;

	/* Initialize window */
	if (!wnd_construct(wnd, title, parent, x, y, width, height, flags))
	{
		free(wnd);
		return NULL;
	}
	return wnd;
} /* End of 'wnd_new' function */

/* Initialize window fields
 * Main job for creating window is done here. Constructors for
 * various window classes should call this function to initialize 
 * common window part. */
bool_t wnd_construct( wnd_t *wnd, char *title, wnd_t *parent, int x, int y,
		int width, int height, dword flags )
{
	wnd_t *cur_focus;
	int sx, sy;
	char cfg_name[32];
	wnd_t *child;

	assert(wnd);
	assert(parent);
	assert(!(flags & WND_FLAG_ROOT));

	/* Obtain window screen coordinates */
	sx = parent->m_screen_x + parent->m_client_x + x;
	sy = parent->m_screen_y + parent->m_client_y + y;

	/* Set window fields */
	memset(wnd, 0, sizeof(*wnd));
	wnd->m_title = (title == NULL ? strdup("") : strdup(title));
	wnd->m_flags = flags;
	wnd->m_child = NULL;
	wnd->m_next = NULL;
	wnd->m_prev = NULL;
	wnd->m_num_children = 0;
	wnd->m_parent = parent;
	wnd->m_x = x;
	wnd->m_y = y;
	wnd->m_screen_x = sx;
	wnd->m_screen_y = sy;
	wnd->m_width = width;
	wnd->m_height = height;
	wnd->m_client_x = 0;
	wnd->m_client_y = 0;
	wnd->m_client_w = width;
	wnd->m_client_h = height;
	wnd->m_fg_color = WND_COLOR_WHITE;
	wnd->m_bg_color = WND_COLOR_BLACK;
	wnd->m_attrib = 0;
	wnd->m_cursor_x = wnd->m_cursor_y = 0;
	wnd->m_global = parent->m_global;
	wnd->m_id = ++wnd->m_global->m_last_id;
	wnd->m_mode = WND_MODE_NORMAL;
	wnd->m_cursor_hidden = FALSE;

	/* Calculate client area depending on the window flags */
	if (wnd->m_flags & WND_FLAG_BORDER)
	{
		wnd->m_client_x ++;
		wnd->m_client_y ++;
		wnd->m_client_w -= 2;
		wnd->m_client_h -= 2;
	}
	else if (wnd->m_flags & WND_FLAG_CAPTION)
	{
		wnd->m_client_y ++;
		wnd->m_client_h --;
	}

	/* Set z-value and focus information */
	wnd->m_zval = wnd->m_parent->m_num_children;
	wnd->m_lower_sibling = wnd->m_parent->m_focus_child;
	wnd->m_parent->m_focus_child = wnd;

	/* Initialize configuration list */
	snprintf(cfg_name, sizeof(cfg_name), "%d", wnd->m_id);
	wnd->m_cfg_list = cfg_new_list(wnd->m_parent->m_cfg_list->m_parent, 
			cfg_name, CFG_NODE_MEDIUM_LIST | CFG_NODE_RUNTIME, 0);

	/* Write information of us into the windows hierarchy */
	if (parent->m_child == NULL)
		parent->m_child = wnd;
	else
	{
		for ( child = parent->m_child; child->m_next != NULL; 
				child = child->m_next );
		child->m_next = wnd;
		wnd->m_prev = child;
	}
	parent->m_num_children ++;

	/* Each new window gains focus */
	WND_FOCUS(wnd) = wnd;

	/* Maximize window if need */
	if (wnd->m_flags & WND_FLAG_MAXIMIZED)
	{
		wnd->m_flags &= ~WND_FLAG_MAXIMIZED;
		wnd_toggle_maximize(wnd);
		wnd->m_pos_before_max.x = wnd->m_x;
		wnd->m_pos_before_max.y = wnd->m_y;
		wnd->m_pos_before_max.w = wnd->m_width;
		wnd->m_pos_before_max.h = wnd->m_height;
	}
	
	/* Send display message to this window */
	wnd_invalidate(wnd);
	return TRUE;
} /* End of 'wnd_construct' function */

/* Run main window loop */
void wnd_main( wnd_t *wnd_root )
{
	wnd_msg_t msg;
	int was_width, was_height;

	assert(wnd_root);

	for ( was_width = wnd_root->m_width, was_height = wnd_root->m_height;; )
	{
		struct winsize winsz;

		/* Check if screen size is changed */
		winsz.ws_col = winsz.ws_row = 0;
		ioctl(0, TIOCGWINSZ, &winsz);
		if (winsz.ws_col != was_width || winsz.ws_row != was_height)
		{
			struct wnd_display_buf_t *buf = &WND_DISPLAY_BUF(wnd_root);
			int size;

			/* Rearrange all the windows */
			was_width = winsz.ws_col;
			was_height = winsz.ws_row;
			wnd_repos(wnd_root, 0, 0, winsz.ws_col, winsz.ws_row);
			resizeterm(winsz.ws_row, winsz.ws_col);

			/* Reallocate display buffer */
			buf->m_dirty = TRUE;
			buf->m_width = wnd_root->m_width;
			buf->m_height = wnd_root->m_height;
			free(buf->m_data);
			size = buf->m_width * buf->m_height * sizeof(*buf->m_data);
			buf->m_data = (struct wnd_display_buf_symbol_t *)malloc(size);
			memset(buf->m_data, 0, size);
		}

		/* Get message from queue */
		if (wnd_msg_get(WND_MSG_QUEUE(wnd_root), &msg))
		{
			/* Handle it */
			wnd_msg_callback_t callback = NULL;
			wnd_t *target;
			wnd_msg_retcode_t ret;
			wnd_msg_handler_t *h;

			/* Choose appropriate callback for calling handler */
			target = msg.m_wnd;
			assert(target);
			if (msg.m_type == WND_MSG_DISPLAY)
			{
				callback = wnd_callback_display;
				h = target->m_on_display;
			}
			else if (msg.m_type == WND_MSG_KEYDOWN)
			{
				callback = wnd_callback_keydown;
				h = target->m_on_keydown;
			}
			else if (msg.m_type == WND_MSG_CLOSE)
			{
				callback = wnd_callback_close;
				h = target->m_on_close;
			}
			else if (msg.m_type == WND_MSG_ERASE_BACK)
			{
				callback = wnd_callback_erase_back;
				h = target->m_on_erase_back;
			}
			else if (msg.m_type == WND_MSG_UPDATE_SCREEN)
			{
				callback = wnd_callback_update_screen;
				h = target->m_on_update_screen;
			}
			else if (msg.m_type == WND_MSG_PARENT_REPOS)
			{
				callback = wnd_callback_parent_repos;
				h = target->m_on_parent_repos;
			}
			else if (msg.m_type == WND_MSG_MOUSE_LDOWN)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_ldown;
			}
			else if (msg.m_type == WND_MSG_MOUSE_MDOWN)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_mdown;
			}
			else if (msg.m_type == WND_MSG_MOUSE_RDOWN)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_rdown;
			}
			else if (msg.m_type == WND_MSG_MOUSE_LDOUBLE)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_ldouble;
			}
			else if (msg.m_type == WND_MSG_MOUSE_MDOUBLE)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_mdouble;
			}
			else if (msg.m_type == WND_MSG_MOUSE_RDOUBLE)
			{
				callback = wnd_callback_mouse;
				h = target->m_on_mouse_rdouble;
			}

			/* Call handler */
			ret = wnd_call_handler(target, h, callback, &msg.m_data);
			wnd_msg_free(&msg);
			if (ret == WND_MSG_RETCODE_EXIT)
				break;
		}

		/* Wait a little */
		util_wait();
	}
} /* End of 'wnd_main' function */

/* Initialize color pairs array */
void wnd_init_pairs( void )
{
	int i;

	/* Initialize pairs */
	for ( i = 0; i < COLOR_PAIRS; i ++ )
	{
		short cfg, cbg;
		wnd_color_t fg = i / WND_COLOR_NUMBER;
		wnd_color_t bg = i % WND_COLOR_NUMBER;

		/* Convert our colors to NCURSES */
		cfg = wnd_color_our2curses(fg);
		cbg = wnd_color_our2curses(bg);

		/* Initialize this color pair */
		init_pair(i, cfg, cbg);
	}
} /* End of 'wnd_init_pairs' function */

/* Convert color is our format to color is NCURSES format */
int wnd_color_our2curses( wnd_color_t col )
{
	switch (col)
	{
	case WND_COLOR_BLACK:
		return COLOR_BLACK;
	case WND_COLOR_RED:
		return COLOR_RED;
	case WND_COLOR_GREEN:
		return COLOR_GREEN;
	case WND_COLOR_BLUE:
		return COLOR_BLUE;
	case WND_COLOR_YELLOW:
		return COLOR_YELLOW;
	case WND_COLOR_MAGENTA:
		return COLOR_MAGENTA;
	case WND_COLOR_CYAN:
		return COLOR_CYAN;
	case WND_COLOR_WHITE:
		return COLOR_WHITE;
	}
	return COLOR_WHITE;
} /* End of 'wnd_color_our2curses' function */

/* Draw window decorations */
void wnd_draw_decorations( wnd_t *wnd )
{
	int i;

	assert(wnd);
	assert(wnd->m_title);

	/* Save window state */
	wnd_push_state(wnd, WND_STATE_COLOR | WND_STATE_ATTRIB | WND_STATE_CURSOR);

	/* Display border */
	if (WND_FLAGS(wnd) & WND_FLAG_BORDER)
	{
		int text_pos;
		char *border_style = wnd->m_mode == WND_MODE_NORMAL ? "border-style" :
				"repos-border-style";

		/* Set style */
		wnd_set_style(wnd, border_style);
		
		/* Print top border */
		wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, 0);
		wnd_putc(wnd, ACS_ULCORNER);
		for ( i = 1; i < wnd->m_width - 1; i ++ )
			wnd_putc(wnd, ACS_HLINE);
		wnd_putc(wnd, ACS_URCORNER);
		
		/* Print caption */
		if (WND_FLAGS(wnd) & WND_FLAG_CAPTION)
		{
			/* Determine title position */
			text_pos = (int)(wnd->m_width - strlen(wnd->m_title) - 2) / 2;
			if (text_pos <= 0)
				text_pos = 1;

			/* Display window title */
			wnd_set_style(wnd, "caption-style");
			wnd_move(wnd, WND_MOVE_ABSOLUTE, text_pos, 0);
			wnd_putc(wnd, ' ');
			wnd_putstring(wnd, WND_PRINT_NONCLIENT | WND_PRINT_ELLIPSES,
					wnd->m_width - text_pos - 2, wnd->m_title);
			wnd_putc(wnd, ' ');
		}

		/* Print left and right borders */
		wnd_set_style(wnd, border_style);
		for ( i = 1; i < wnd->m_height - 1; i ++ )
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, i);
			wnd_putc(wnd, ACS_VLINE);
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 1, i);
			wnd_putc(wnd, ACS_VLINE);
		}

		/* Print bottom border */
		wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, wnd->m_height - 1);
		wnd_putc(wnd, ACS_LLCORNER);
		for ( i = 1; i < wnd->m_width - 1; i ++ )
			wnd_putc(wnd, ACS_HLINE);
		wnd_putc(wnd, ACS_LRCORNER);

		/* Print maximize and close boxes */
		if (WND_FLAGS(wnd) & WND_FLAG_MAX_BOX)
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 3, 0);
			wnd_set_style(wnd, "maximize-box-style");
			wnd_putc(wnd, (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED) ? 'o' : 'O');
		}
		if (WND_FLAGS(wnd) & WND_FLAG_CLOSE_BOX)
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 2, 0);
			wnd_set_style(wnd, "close-box-style");
			wnd_putc(wnd, 'X');
		}
	}

	/* Display window caption */
	if (WND_FLAGS(wnd) & WND_FLAG_CAPTION)
	{
		/* Set style */
		wnd_set_style(wnd, "border-style");

	}

	/* Restore window state */
	wnd_pop_state(wnd);
} /* End of 'wnd_draw_decorations' function */

/* Push window states */
void wnd_push_state( wnd_t *wnd, wnd_state_t mask )
{
	struct wnd_state_data_t *data;

	assert(wnd);
	assert(WND_GLOBAL(wnd));
	assert(WND_STATES_POS(wnd) < WND_STATES_STACK_SIZE);
	
	data = &WND_STATES_STACK(wnd)[WND_STATES_POS(wnd) ++];
	data->m_wnd = wnd;
	data->m_mask = mask;
	if (mask & WND_STATE_FG_COLOR)
		data->m_fg_color = wnd->m_fg_color;
	if (mask & WND_STATE_BG_COLOR)
		data->m_bg_color = wnd->m_bg_color;
	if (mask & WND_STATE_ATTRIB)
		data->m_attrib = wnd->m_attrib;
	if (mask & WND_STATE_CURSOR)
	{
		data->m_cursor_x = wnd->m_cursor_x;
		data->m_cursor_y = wnd->m_cursor_y;
	}
} /* End of 'wnd_push_state' function */

/* Pop window states */
void wnd_pop_state( wnd_t *wnd )
{
	struct wnd_state_data_t *data;
	wnd_state_t mask;

	assert(wnd);
	assert(WND_GLOBAL(wnd));
	assert(WND_STATES_POS(wnd) > 0);

	data = &WND_STATES_STACK(wnd)[--WND_STATES_POS(wnd)];
	mask = data->m_mask;
	if (mask & WND_STATE_FG_COLOR)
		wnd->m_fg_color = data->m_fg_color; 
	if (mask & WND_STATE_BG_COLOR)
		wnd->m_bg_color = data->m_bg_color; 
	if (mask & WND_STATE_ATTRIB)
		wnd->m_attrib = data->m_attrib; 
	if (mask & WND_STATE_CURSOR)
	{
		wnd->m_cursor_x = data->m_cursor_x; 
		wnd->m_cursor_y = data->m_cursor_y; 
	}
} /* End of 'wnd_pop_state' function */

/* Display windows bar (in root window) */
void wnd_display_wnd_bar( wnd_t *wnd_root )
{
	assert(wnd_root);
	
	/* Set style */
	wnd_push_state(wnd_root, WND_STATE_ALL);
	wnd_move(wnd_root, WND_MOVE_ABSOLUTE, 0, wnd_root->m_height - 1);
	wnd_set_style(wnd_root, "wndbar-style");

	/* Print items for top-level windows */
	if (wnd_root->m_child == NULL)
	{
		wnd_printf(wnd_root, WND_PRINT_NONCLIENT, 0, "No children\n");
	}
	else
	{
		wnd_t *child;
		int right_pos = 0;

		/* Print items for children */
		wnd_move(wnd_root, WND_MOVE_ABSOLUTE, 0, wnd_root->m_height - 1);
		for ( child = wnd_root->m_child; child != NULL; child = child->m_next )
		{
			right_pos += wnd_root->m_width / wnd_root->m_num_children;

			/* Print focus child with another style */
			wnd_push_state(wnd_root, WND_STATE_COLOR | WND_STATE_ATTRIB);
			if (child == wnd_root->m_focus_child)
				wnd_set_style(wnd_root, "wndbar-focus-style");
			wnd_printf(wnd_root, WND_PRINT_NONCLIENT | WND_PRINT_ELLIPSES, 
					right_pos, "%s\n", child->m_title);
			wnd_pop_state(wnd_root);

			/* Print delimiter */
			wnd_move(wnd_root, WND_MOVE_ABSOLUTE, right_pos + 1, 
					wnd_root->m_height - 1);
			wnd_putc(wnd_root, '|');
		}
	}
	
	wnd_pop_state(wnd_root);
} /* End of 'wnd_display_wnd_bar' function */

/* Get window setting value */
char *wnd_get_setting( wnd_t *wnd, char *name )
{
	char *val;

	assert(wnd);
	assert(name);

	/* First look up window's own configuration list */
	val = cfg_get_var(wnd->m_cfg_list, name);

	/* Then look up common windows setting list */
	if (val == NULL)
		val = cfg_get_var(wnd->m_cfg_list->m_parent, name);
	return val;
} /* End of 'wnd_get_setting' function */

/* Set window style */
void wnd_set_style( wnd_t *wnd, char *name )
{
	char *val;
	wnd_color_t fg_color, bg_color;
	int attrib;

	assert(wnd);
	assert(name);

	/* First set the default style */
	wnd_set_color(wnd, WND_COLOR_WHITE, WND_COLOR_BLACK);
	wnd_set_attrib(wnd, 0);

	/* Get style value */
	val = wnd_get_setting(wnd, name);

	/* Parse style and set it */
	wnd_parse_style(val, &fg_color, &bg_color, &attrib);
	wnd_set_color(wnd, fg_color, bg_color);
	wnd_set_attrib(wnd, attrib);
} /* End of 'wnd_set_style' function */

/* Parse style value */
void wnd_parse_style( char *str, wnd_color_t *fg_color, wnd_color_t *bg_color,
		int *attrib )
{
#define WND_PARSE_STYLE_STATE_FG	0
#define WND_PARSE_STYLE_STATE_BG	1
#define WND_PARSE_STYLE_STATE_ATTR	2
	int state = WND_PARSE_STYLE_STATE_FG;
	char *ptr;
	wnd_color_t *dest = fg_color;

	/* Set default values first */
	*fg_color = WND_COLOR_WHITE;
	*bg_color = WND_COLOR_BLACK;
	*attrib = 0;

	/* Parse string */
	for ( ptr = str;; ptr ++ )
	{
		switch (state)
		{
		case WND_PARSE_STYLE_STATE_FG:
		case WND_PARSE_STYLE_STATE_BG:
			if (*ptr == ':' || *ptr == '\0')
			{
				/* Parse color value */
				char was_ch = *ptr;
				*ptr = 0;
				*dest = wnd_string2color(str);
				*ptr = was_ch;
				str = ptr + 1;

				/* Change state */
				if (state == WND_PARSE_STYLE_STATE_FG)
				{
					state = WND_PARSE_STYLE_STATE_BG;
					dest = bg_color;
				}
				else
					state = WND_PARSE_STYLE_STATE_ATTR;
				}
				break;
		case WND_PARSE_STYLE_STATE_ATTR:
			if (*ptr == ',' || *ptr == '\0')
			{
				char was_ch = *ptr;
				*ptr = 0;
				(*attrib) |= wnd_string2attrib(str);
				*ptr = was_ch;
				str = ptr + 1;
			}
			break;
		}

		if (*ptr == '\0')
			return;
	}
} /* End of 'wnd_parse_style' function */

/* Get color from its textual representation */
wnd_color_t wnd_string2color( char *str )
{
	if (!strcasecmp(str, "white"))
		return WND_COLOR_WHITE;
	else if (!strcasecmp(str, "black"))
		return WND_COLOR_BLACK;
	else if (!strcasecmp(str, "red"))
		return WND_COLOR_RED;
	else if (!strcasecmp(str, "green"))
		return WND_COLOR_GREEN;
	else if (!strcasecmp(str, "blue"))
		return WND_COLOR_BLUE;
	else if (!strcasecmp(str, "yellow"))
		return WND_COLOR_YELLOW;
	else if (!strcasecmp(str, "magenta"))
		return WND_COLOR_MAGENTA;
	else if (!strcasecmp(str, "cyan"))
		return WND_COLOR_CYAN;
	return WND_COLOR_BLACK;
} /* End of 'wnd_string2color' function */

/* Get attribute from its textual representation */
int wnd_string2attrib( char *str )
{
	if (!strcasecmp(str, "standout"))
		return WND_ATTRIB_STANDOUT;
	else if (!strcasecmp(str, "underline"))
		return WND_ATTRIB_UNDERLINE;
	else if (!strcasecmp(str, "reverse"))
		return WND_ATTRIB_REVERSE;
	else if (!strcasecmp(str, "blink"))
		return WND_ATTRIB_BLINK;
	else if (!strcasecmp(str, "dim"))
		return WND_ATTRIB_DIM;
	else if (!strcasecmp(str, "bold"))
		return WND_ATTRIB_BOLD;
	else if (!strcasecmp(str, "protect"))
		return WND_ATTRIB_PROTECT;
	else if (!strcasecmp(str, "invis"))
		return WND_ATTRIB_INVIS;
	else if (!strcasecmp(str, "altcharset"))
		return WND_ATTRIB_ALTCHARSET;
	return 0;
} /* End of 'wnd_string2attrib' function */

/* Set focus window */
void wnd_set_focus( wnd_t *wnd )
{
	wnd_t *parent, *child, *prev = NULL;

	assert(wnd);
	assert(parent = wnd->m_parent);

	/* Rearrange windows */
	for ( child = parent->m_child; child != NULL; child = child->m_next )
	{
		if (child->m_zval > wnd->m_zval)
		{
			child->m_zval --;
			if (child->m_zval == wnd->m_zval + 1)
				prev = child;
		}
	}
	wnd->m_zval = parent->m_num_children - 1;
	if (prev != NULL)
		prev->m_lower_sibling = wnd->m_lower_sibling;
	wnd->m_lower_sibling = parent->m_focus_child;

	/* Set focus to this window */
	parent->m_focus_child = wnd;
	wnd_set_global_focus(WND_GLOBAL(wnd));

	/* Repaint */
	wnd_invalidate(parent);
} /* End of 'wnd_set_focus' function */

/* Set focus to the next child of this window */
void wnd_next_focus( wnd_t *wnd )
{
	assert(wnd);

	/* This function does nothing if there are no children */
	if (wnd->m_child == NULL)
		return;

	/* Set focus */
	wnd_set_focus(wnd->m_focus_child->m_next == NULL ? 
			wnd->m_child : wnd->m_focus_child->m_next);
} /* End of 'wnd_next_focus' function */

/* Set focus to the previous child of this window */
void wnd_prev_focus( wnd_t *wnd )
{
	wnd_t *child;

	assert(wnd);

	/* This function does nothing if there are no children */
	if (wnd->m_child == NULL)
		return;

	/* Set focus */
	if (wnd->m_focus_child->m_prev != NULL)
		wnd_set_focus(wnd->m_focus_child->m_prev);
	else
	{
		wnd_t *child;

		for ( child = wnd->m_child; child->m_next != NULL; 
				child = child->m_next );
		wnd_set_focus(child);
	}
} /* End of 'wnd_prev_focus' function */

/* Invalidate window */
void wnd_invalidate( wnd_t *wnd )
{
	wnd_t *child;

	if (wnd == NULL)
		return;

	wnd_msg_send(wnd, WND_MSG_ERASE_BACK, wnd_msg_data_erase_back_new());
	wnd_msg_send(wnd, WND_MSG_DISPLAY, wnd_msg_data_display_new());
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		wnd_invalidate(child);
	}
	wnd_msg_send(wnd, WND_MSG_UPDATE_SCREEN, wnd_msg_data_update_screen_new());
} /* End of 'wnd_invalidate' function */

/* Synchronize screen contents with display buffer */
void wnd_sync_screen( wnd_t *wnd )
{
	struct wnd_display_buf_t *buf = &WND_DISPLAY_BUF(wnd);
	int i, x = 0, y = 0;
	wnd_t *wnd_focus;

	/* Clear screen if buffer is dirty */
	if (buf->m_dirty)
		clear();

	/* Copy buffer to screen */
	for ( i = 0;; i ++ )
	{
		/* Set symbol */
		mvaddch(y, x, buf->m_data[i].m_attr | buf->m_data[i].m_char);

		/* Move to next symbol */
		if (x >= buf->m_width - 1)
		{
			x = 0;
			y ++;
			if (y >= buf->m_height)
				break;
		}
		else
			x ++;
	}

	/* Synchronize cursor */
	wnd_focus = WND_FOCUS(wnd);
	if (wnd_focus->m_cursor_hidden || !wnd_cursor_in_client(wnd_focus))
		move(LINES - 1, COLS - 1);
	else
		move(WND_CLIENT2SCREEN_Y(wnd_focus, wnd_focus->m_cursor_y),
				WND_CLIENT2SCREEN_X(wnd_focus, wnd_focus->m_cursor_x));

	/* Refresh screen */
	refresh();
	buf->m_dirty = FALSE;
} /* End of 'wnd_sync_screen' function */

/* Call message handlers chain */
wnd_msg_retcode_t wnd_call_handler( wnd_t *wnd, wnd_msg_handler_t *handler, 
		wnd_msg_callback_t callback, wnd_msg_data_t *data )
{
	wnd_msg_retcode_t ret;
	for ( ;; handler = handler->m_next )
	{
		ret = callback(wnd, handler, data);
		if (handler == NULL || ret == WND_MSG_RETCODE_STOP || 
				ret == WND_MSG_RETCODE_EXIT)
			return ret;
	}
	return ret;
} /* End of 'wnd_call_handler' function */

/* Call window destructor */
void wnd_call_destructor( wnd_t *wnd )
{
	assert(wnd);
	wnd_call_handler(wnd, wnd->m_destructor, wnd_callback_destructor, NULL);
} /* End of 'wnd_call_destructor' function */

/* Callback for destructor */
wnd_msg_retcode_t wnd_callback_destructor( wnd_t *wnd, wnd_msg_handler_t *h,
		wnd_msg_data_t *data )
{
	if (h == NULL)
		wnd_default_destructor(wnd);
	else
		((void (*)(wnd_t *))(h->m_func))(wnd);
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_callback_destructor' function */

/* Set window mode */
void wnd_set_mode( wnd_t *wnd, wnd_mode_t mode )
{
	assert(wnd);

	/* Only windows with border may change position or size */
	if ((mode == WND_MODE_REPOSITION || mode == WND_MODE_RESIZE) &&
			!(wnd->m_flags & WND_FLAG_BORDER))
		return;
	
	/* Set new mode */
	wnd->m_mode = mode;

	/* Install new key handler for window parameters changing */
	wnd_msg_add_handler(&wnd->m_on_keydown, wnd_repos_on_key);
	
	/* Repaint window */
	wnd_invalidate(wnd);
} /* End of 'wnd_set_mode' function */

/* Handler for key pressing when in reposition or resize window mode */
wnd_msg_retcode_t wnd_repos_on_key( wnd_t *wnd, wnd_key_t *keycode )
{
	int x, y, w, h;
	bool_t not_changed = FALSE;

	assert(wnd);
	assert(keycode);
	assert(wnd->m_mode != WND_MODE_NORMAL);

	/* Choose new window parameters */
	x = wnd->m_x;
	y = wnd->m_y;
	w = wnd->m_width;
	h = wnd->m_height;
	if (wnd->m_mode == WND_MODE_REPOSITION)
	{
		if (keycode->m_key == KEY_UP)
			y --;
		else if (keycode->m_key == KEY_DOWN)
			y ++;
		else if (keycode->m_key == KEY_RIGHT)
			x ++;
		else if (keycode->m_key == KEY_LEFT)
			x --;
		else
			not_changed = TRUE;
	}
	else if (wnd->m_mode == WND_MODE_RESIZE)
	{
		if (keycode->m_key == KEY_UP)
			h --;
		else if (keycode->m_key == KEY_DOWN)
			h ++;
		else if (keycode->m_key == KEY_RIGHT)
			w ++;
		else if (keycode->m_key == KEY_LEFT)
			w --;
		else
			not_changed = TRUE;
	}
	else
		not_changed = TRUE;

	/* Unmaximize window */
	if (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED)
	{
		WND_FLAGS(wnd) &= ~WND_FLAG_MAXIMIZED;
	}

	/* Return back to normal mode */
	if (not_changed && keycode->m_key == '\n')
	{
		wnd->m_mode = WND_MODE_NORMAL;
		wnd_msg_rem_handler(&wnd->m_on_keydown);
		wnd_invalidate(wnd);
		return WND_MSG_RETCODE_STOP;
	}

	/* Move window */
	if (!not_changed)
	{
		wnd_repos(wnd, x, y, w, h);
		return WND_MSG_RETCODE_STOP;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_repos_on_key' function */

/* Move and resize window */
void wnd_repos( wnd_t *wnd, int x, int y, int width, int height )
{
	int dx, dy, dw, dh;
	int px, py, pw, ph;
	wnd_t *child;

	assert(wnd);

	/* Set new window position and size */
	px = wnd->m_x;
	py = wnd->m_y;
	pw = wnd->m_width;
	ph = wnd->m_height;
	dx = x - wnd->m_x;
	dy = y - wnd->m_y;
	dw = width - wnd->m_width;
	dh = height - wnd->m_height;
	wnd->m_x = x;
	wnd->m_y = y;
	wnd->m_width = width;
	wnd->m_height = height;
	wnd->m_screen_x += dx;
	wnd->m_screen_y += dy;
	wnd->m_client_w += dw;
	wnd->m_client_h += dh;

	/* Inform children about our repositioning */
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
		wnd_msg_send(child, WND_MSG_PARENT_REPOS, 
				wnd_msg_data_parent_repos_new(px, py, pw, ph, x, y, 
					width, height));

	/* Repaint window */
	wnd_invalidate(wnd->m_parent == NULL ? wnd : wnd->m_parent);
} /* End of 'wnd_repos' function */

/* Toggle the window maximization flag */
void wnd_toggle_maximize( wnd_t *wnd )
{
	assert(wnd);

	/* Maximization make no sense for root window */
	if (WND_IS_ROOT(wnd))
		return;

	/* Change flag to the opposite */
	if (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED)
		WND_FLAGS(wnd) &= ~WND_FLAG_MAXIMIZED;
	else
		WND_FLAGS(wnd) |= WND_FLAG_MAXIMIZED;

	/* Move window */
	if (!(WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED))
	{
		wnd_repos(wnd, wnd->m_pos_before_max.x, wnd->m_pos_before_max.y,
				wnd->m_pos_before_max.w, wnd->m_pos_before_max.h);
	}
	else
	{
		wnd->m_pos_before_max.x = wnd->m_x;
		wnd->m_pos_before_max.y = wnd->m_y;
		wnd->m_pos_before_max.w = wnd->m_width;
		wnd->m_pos_before_max.h = wnd->m_height;
		wnd_repos(wnd, 0, 0, wnd->m_parent->m_client_w, 
				wnd->m_parent->m_client_h);
	}
} /* End of 'wnd_toggle_maximize' function */

/* Check if specified window is another windows descendant */
bool_t wnd_is_descendant( wnd_t *wnd, wnd_t *base )
{
	wnd_t *child;

	assert(base);

	if (wnd == NULL)
		return FALSE;
	
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		if (child == base || wnd_is_descendant(wnd, child))
			return TRUE;
	}
	return FALSE;
} /* End of 'wnd_is_descendant' function */

/* Reset the global focus */
void wnd_set_global_focus( wnd_global_data_t *global )
{
	wnd_t *wnd;

	assert(global);
	for ( wnd = global->m_root; wnd->m_child != NULL; 
			wnd = wnd->m_focus_child );
	global->m_focus = wnd;
} /* End of 'wnd_set_global_focus' function */

/* Redisplay the screen (discarding all the optimization stuff) */
void wnd_redisplay( wnd_t *wnd )
{
	assert(wnd);
	assert(WND_GLOBAL(wnd));

	WND_DISPLAY_BUF(wnd).m_dirty = TRUE;
	wnd_msg_send(WND_ROOT(wnd), WND_MSG_UPDATE_SCREEN,
			wnd_msg_data_update_screen_new());
} /* End of 'wnd_redisplay' function */

/* End of 'wnd.c' file */

