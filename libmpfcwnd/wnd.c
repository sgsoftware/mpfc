/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * MPFC Window Library. Window functions implementation.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "types.h"
#include "cfg.h"
#include "logger.h"
#include "wnd.h"
#include "wnd_root.h"
#include "util.h"

static const char *wnd_set_title_seq_start = NULL;
static const char *wnd_set_title_seq_end = NULL;

/* Check that this is an xterm or compatible terminal */
static bool_t is_xterm( void )
{
	const char *term = getenv("TERM");
	if (!term)
		return FALSE;
	return !strcmp(term, "xterm") || !strncmp(term, "rxvt", 4);
} /* End of 'is_xterm' function */

/* Initialize window system and create root window */
wnd_t *wnd_init( cfg_node_t *cfg_list, logger_t *log )
{
	WINDOW *wnd = NULL;
	wnd_t *wnd_root = NULL;
	cfg_node_t *cfg_wnd = NULL;
	wnd_global_data_t *global = NULL;
	wnd_kbd_data_t *kbd_data = NULL;
	wnd_kbind_data_t *kbind_data = NULL;
	wnd_mouse_data_t *mouse_data = NULL;
	wnd_msg_queue_t *msg_queue = NULL;
	wnd_class_t *klass = NULL;
	struct wnd_display_buf_symbol_t *db_data = NULL;
	int i, len;
	bool_t force_terminal_bg;
	pthread_mutex_t curses_mutex;

	/* Initialize NCURSES */
	wnd = initscr();
	if (wnd == NULL)
		goto failed;
	start_color();
	cbreak();
	noecho();
	nodelay(wnd, TRUE);
	keypad(wnd, TRUE);
	force_terminal_bg = cfg_get_var_bool(cfg_list, "force-terminal-bg");
	if (force_terminal_bg)
	{
		use_default_colors();
		assume_default_colors(-1, -1);
	}
	wnd_init_pairs(force_terminal_bg);
	pthread_mutex_init(&curses_mutex, NULL);

	/* Initialize global data */
	global = (wnd_global_data_t *)malloc(sizeof(wnd_global_data_t));
	if (global == NULL)
		goto failed;
	memset(global, 0, sizeof(*global));
	global->m_curses_wnd = wnd;
	global->m_last_id = -1;
	global->m_states_stack_pos = 0;
	global->m_lib_active = TRUE;
	global->m_invalid_exist = TRUE;
	global->m_curses_mutex = curses_mutex;

	/* Initialize display buffer */
	len = COLS * LINES;
	db_data = (struct wnd_display_buf_symbol_t *)malloc(len * sizeof(*db_data));
	if (db_data == NULL)
		goto failed;
	memset(db_data, 0, len * sizeof(*db_data));
	for ( i = 0; i < len; i ++ )
		db_data[i].m_char.chars[0] = L' ';
	global->m_display_buf.m_width = COLS;
	global->m_display_buf.m_height = LINES;
	global->m_display_buf.m_data = db_data;
	pthread_mutex_init(&global->m_display_buf.m_mutex, NULL);

	logger_debug(log, "Initializing window system of size %dx%d", COLS, LINES);

	/* Initialize configuration */
	cfg_wnd = cfg_new_list(cfg_list, "windows", NULL,
			CFG_NODE_MEDIUM_LIST | CFG_NODE_RUNTIME, 0);
	if (cfg_wnd == NULL)
		goto failed;
	global->m_root_cfg = cfg_wnd;
	global->m_log = log;
	global->m_classes_cfg = cfg_new_list(cfg_wnd, "classes", 
			NULL, CFG_NODE_MEDIUM_LIST, 0);

	/* Initialize needed window classes */
	klass = wnd_root_class_init(global);
	if (klass == NULL)
		goto failed;

	/* Create root window */
	wnd_root = (wnd_t *)malloc(sizeof(wnd_root_t));
	if (wnd_root == NULL)
		goto failed;
	memset(wnd_root, 0, sizeof(wnd_root_t));
	global->m_root = wnd_root;
	wnd_root->m_global = global;
	wnd_root->m_class = klass;

	/* Initialize window fields */
	if (!wnd_construct(wnd_root, NULL, "root", 0, 0, COLS, LINES, 
				WND_FLAG_ROOT | WND_FLAG_OWN_DECOR))
		goto failed;
	wnd_root->m_cursor_hidden = TRUE;

	/* Set root window specific handlers */
	wnd_msg_add_handler(wnd_root, "keydown", wnd_root_on_keydown);
	wnd_msg_add_handler(wnd_root, "display", wnd_root_on_display);
	wnd_msg_add_handler(wnd_root, "close", wnd_root_on_close);
	wnd_msg_add_handler(wnd_root, "update_screen", wnd_root_on_update_screen);
	wnd_msg_add_handler(wnd_root, "mouse_ldown", wnd_root_on_mouse);

	/* Initialize message queue */
	msg_queue = wnd_msg_queue_init();
	if (msg_queue == NULL)
		goto failed;
	global->m_msg_queue = msg_queue;

	/* Initialize keyboard module */
	kbd_data = wnd_kbd_init(wnd_root);
	if (kbd_data == NULL)
		goto failed;
	global->m_kbd_data = kbd_data;

	/* Initialize kbind module */
	kbind_data = wnd_kbind_init(global);
	if (kbind_data == NULL)
		goto failed;
	global->m_kbind_data = kbind_data;

	/* Initialize mouse */
	mouse_data = wnd_mouse_init(global);
	if (mouse_data == NULL)
		goto failed;
	global->m_mouse_data = mouse_data;
	
	/* Initialize escape sequence for setting window title */
	wnd_set_title_seq_start = cfg_get_var(global->m_root_cfg, "ti.ts");
	if (!wnd_set_title_seq_start)
	{
		if (is_xterm())
			wnd_set_title_seq_start = "\033]2;";
	}
	else if (!(*wnd_set_title_seq_start))
		wnd_set_title_seq_start = NULL;
	wnd_set_title_seq_end = cfg_get_var(global->m_root_cfg, "ti.fs");
	if (!wnd_set_title_seq_end)
		wnd_set_title_seq_end = "\007";

	/* Send display message to this window */
	wnd_postinit(wnd_root);
	return wnd_root;

	/* Code for handling some step failing */
failed:
	if (mouse_data != NULL)
		wnd_mouse_free(mouse_data);
	if (kbind_data != NULL)
		wnd_kbind_free(kbind_data);
	if (kbd_data != NULL)
		wnd_kbd_free(kbd_data);
	if (msg_queue != NULL)
		wnd_msg_queue_free(msg_queue);
	if (cfg_wnd != NULL)
		cfg_free_node(cfg_wnd, TRUE);
	if (wnd_root != NULL)
		free(wnd_root);
	if (klass != NULL)
		wnd_class_free(klass);
	if (db_data != NULL)
		free(db_data);
	if (global != NULL)
		free(global);
	if (wnd != NULL)
		endwin();
	return NULL;
} /* End of 'wnd_init' function */

/* Create a window */
wnd_t *wnd_new( wnd_t *parent, char *title, int x, int y, 
				int width, int height, wnd_flags_t flags )
{
	wnd_t *wnd;
	wnd_class_t *klass;

	/* Allocate memory */
	wnd = (wnd_t *)malloc(sizeof(wnd_t));
	if (wnd == NULL)
		return NULL;
	memset(wnd, 0, sizeof(*wnd));

	/* Set class first */
	klass = wnd_basic_class_init(WND_GLOBAL(parent));
	if (klass == NULL)
		return NULL;
	wnd->m_class = klass;

	/* Initialize window */
	if (!wnd_construct(wnd, parent, title, x, y, width, height, flags))
	{
		free(wnd);
		return NULL;
	}
	wnd_postinit(wnd);
	return wnd;
} /* End of 'wnd_new' function */

/* Initialize window fields
 * Main job for creating window is done here. Constructors for
 * various window classes should call this function to initialize 
 * common window part. */
bool_t wnd_construct( wnd_t *wnd, wnd_t *parent, char *title, int x, int y,
		int width, int height, wnd_flags_t flags )
{
	wnd_t *cur_focus;
	int sx, sy;
	char cfg_name[32];
	wnd_t *child;

	assert(wnd);

	/* Check level */
	if (parent != NULL && parent->m_level >= WND_MAX_LEVEL)
		return FALSE;

	/* Maximize window */
	if (parent != NULL && (flags & WND_FLAG_MAXIMIZED))
	{
		x = 0;
		y = 0;
		width = parent->m_client_w;
		height = parent->m_client_h;
	}

	/* Obtain window screen coordinates */
	if (parent != NULL)
	{
		sx = parent->m_screen_x + parent->m_client_x + x;
		sy = parent->m_screen_y + parent->m_client_y + y;
	}
	else
	{
		sx = x;
		sy = y;
	}

	/* Set window fields */
	wnd->m_title = (title == NULL ? strdup("") : strdup(title));
	if (wnd->m_title == NULL)
		goto failed;
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
	wnd->m_is_invalid = TRUE;
	wnd->m_fg_color = WND_COLOR_WHITE;
	wnd->m_bg_color = WND_COLOR_BLACK;
	wnd->m_attrib = 0;
	wnd->m_cursor_x = wnd->m_cursor_y = 0;
	wnd->m_pos_before_max.x = wnd->m_x;
	wnd->m_pos_before_max.y = wnd->m_y;
	wnd->m_pos_before_max.w = wnd->m_width;
	wnd->m_pos_before_max.h = wnd->m_height;
	if (parent != NULL)
		wnd->m_global = parent->m_global;
	wnd->m_id = ++wnd->m_global->m_last_id;
	wnd->m_mode = WND_MODE_NORMAL;
	wnd->m_cursor_hidden = FALSE;
	wnd->m_level = (parent == NULL ? 0 : parent->m_level + 1);
	wnd_calc_real_pos(wnd);

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
	else if (wnd->m_flags & WND_FLAG_ROOT)
	{
		wnd->m_client_h --;
	}

	/* Write information of us into the windows hierarchy */
	if (parent != NULL)
	{
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
	}

	/* Set z-value and focus information */
	if (parent != NULL)
	{
		wnd->m_zval = parent->m_num_children - 1;
		wnd_regen_zvalue_list(parent);
	}
	else
	{
		wnd->m_zval = 0;
		wnd->m_lower_sibling = NULL;
		wnd->m_higher_sibling = NULL;
	}

	/* Initialize message map */
	wnd_msg_add_handler(wnd, "display", wnd_default_on_display);
	wnd_msg_add_handler(wnd, "keydown", wnd_default_on_keydown);
	wnd_msg_add_handler(wnd, "action", wnd_default_on_action);
	wnd_msg_add_handler(wnd, "close", wnd_default_on_close);
	wnd_msg_add_handler(wnd, "erase_back", wnd_default_on_erase_back);
	wnd_msg_add_handler(wnd, "parent_repos", wnd_default_on_parent_repos);
	wnd_msg_add_handler(wnd, "destructor", wnd_default_destructor);

	/* Initialize configuration list */
	snprintf(cfg_name, sizeof(cfg_name), "%d", wnd->m_id);
	wnd->m_cfg_list = cfg_new_list(WND_ROOT_CFG(wnd),
			cfg_name, NULL, CFG_NODE_MEDIUM_LIST | CFG_NODE_RUNTIME, 0);
	if (wnd->m_cfg_list == NULL)
		goto failed;
	return TRUE;

	/* Failing management code */
failed:
	if (wnd->m_cfg_list != NULL)
		cfg_free_node(wnd->m_cfg_list, TRUE);
	if (wnd->m_title != NULL)
		free(wnd->m_title);
	return FALSE;
} /* End of 'wnd_construct' function */

/* Uninitialize window library */
void wnd_deinit( wnd_t *wnd_root )
{
	wnd_global_data_t *global;
	struct wnd_display_buf_t *db;
	wnd_class_t *klass;

	if (wnd_root == NULL)
		return;

	/* Free windows */
	global = wnd_root->m_global;
	wnd_call_destructor(wnd_root);

	/* Free modules */
	wnd_mouse_free(global->m_mouse_data);
	wnd_kbind_free(global->m_kbind_data);
	wnd_kbd_free(global->m_kbd_data);
	wnd_msg_queue_free(global->m_msg_queue);
	pthread_mutex_destroy(&global->m_curses_mutex);

	/* Free window classes */
	for ( klass = global->m_wnd_classes; klass != NULL; )
	{
		wnd_class_t *next = klass->m_next;
		wnd_class_free(klass);
		klass = next;
	}

	/* Free display buffer */
	db = &global->m_display_buf;
	if (db->m_data != NULL)
	{
		pthread_mutex_destroy(&db->m_mutex);
		free(db->m_data);
	}

	/* Free global data */
	free(global);
	
	/* Uninitialize NCURSES */
	endwin();
} /* End of 'wnd_deinit' function */

/* Run main window loop */
void wnd_main( wnd_t *wnd_root )
{
	wnd_msg_t msg;
	int was_width, was_height;

	assert(wnd_root);

	for ( was_width = wnd_root->m_width, was_height = wnd_root->m_height;; )
	{
		struct winsize winsz;

		/* Do nothing if library is not active now */
		if (!WND_LIB_ACTIVE(wnd_root))
		{
			util_wait();
			continue;
		}

		/* Check if screen size is changed */
		for ( ;; )
		{
			winsz.ws_col = winsz.ws_row = 0;
			ioctl(0, TIOCGWINSZ, &winsz);
			if (winsz.ws_col != was_width || winsz.ws_row != was_height)
			{
				struct wnd_display_buf_t *buf = &WND_DISPLAY_BUF(wnd_root);
				int i, size;

				/* Rearrange all the windows */
				was_width = winsz.ws_col;
				was_height = winsz.ws_row;
				pthread_mutex_lock(&WND_CURSES_MUTEX(wnd_root));
				resizeterm(winsz.ws_row, winsz.ws_col);
				pthread_mutex_unlock(&WND_CURSES_MUTEX(wnd_root));

				/* Reallocate display buffer */
				wnd_display_buf_lock(buf);
				buf->m_dirty = TRUE;
				buf->m_width = COLS;
				buf->m_height = LINES;
				free(buf->m_data);
				size = buf->m_width * buf->m_height;
				size_t sz = size * sizeof(*buf->m_data);
				buf->m_data = (struct wnd_display_buf_symbol_t *)malloc(sz);
				memset(buf->m_data, 0, sz);
				for ( i = 0; i < size; i ++ )
				{
					buf->m_data[i].m_char.chars[0] = L' ';
				}
				wnd_display_buf_unlock(buf);
				wnd_repos(wnd_root, 0, 0, COLS, LINES);
			}
			else
				break;
		}

		/* Get message from queue */
		if (wnd_msg_get(WND_MSG_QUEUE(wnd_root), &msg))
		{
			/* Handle it */
			wnd_t *target;
			wnd_msg_callback_t callback;
			wnd_msg_handler_t *handler, **ph;
			wnd_msg_retcode_t ret;

			/* Choose appropriate callback for calling handler */
			target = msg.m_wnd;
			assert(target);
			ph = wnd_class_get_msg_info(target, msg.m_name, &callback);
			if (ph == NULL)
				continue;
			handler = *ph;

			/* Call handler */
			if (!strcmp(msg.m_name, "display"))
				target->m_is_invalid = FALSE;
			ret = wnd_call_handler(target, msg.m_name, handler, callback, 
					&msg.m_data);
			wnd_msg_free(&msg);
			if (ret == WND_MSG_RETCODE_EXIT)
				break;

			/* Check for invalid windows */
			if (wnd_check_invalid(wnd_root))
				wnd_msg_send(wnd_root, "update_screen",
						wnd_msg_update_screen_new());
		}
		else
		{
			if (wnd_check_invalid(wnd_root))
			{
				wnd_msg_send(wnd_root, "update_screen",
						wnd_msg_update_screen_new());
			}
			util_wait();
		}
	}
} /* End of 'wnd_main' function */

/* Initialize color pairs array */
void wnd_init_pairs( bool_t force_terminal_bg )
{
	int i;

	/* Initialize pairs */
	for ( i = 0; i < COLOR_PAIRS; i ++ )
	{
		short cfg, cbg;
		wnd_color_t fg = i / WND_COLOR_NUMBER;
		wnd_color_t bg = i % WND_COLOR_NUMBER;
		bg ++;
		bg %= WND_COLOR_NUMBER;

		/* Convert our colors to NCURSES */
		cfg = wnd_color_our2curses(fg);
		cbg = wnd_color_our2curses(bg);
		if (force_terminal_bg && cbg == COLOR_BLACK)
			cbg = -1;

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

/* Redisplay all invalid windows */
bool_t wnd_check_invalid( wnd_t *wnd )
{
	bool_t need_update = FALSE;
	wnd_t *child;

	/* Do nothing if we have no invalid windows at all */
	if (!WND_GLOBAL(wnd)->m_invalid_exist)
		return FALSE;

	/* Invalidate this window */
	if (wnd->m_is_invalid)
	{
		wnd_msg_send(wnd, "erase_back", wnd_msg_erase_back_new());
		wnd_send_repaint(wnd, TRUE);
		need_update = TRUE;
	}
	else
	{
		/* Check children */
		for ( child = wnd->m_child; child != NULL; child = child->m_next )
		{
			if (wnd_check_invalid(child))
				need_update = TRUE;
		}
	}
	/* If this is the first-level call, we have no invalid windows */
	if (wnd == WND_ROOT(wnd))
		WND_GLOBAL(wnd)->m_invalid_exist = FALSE;
	return need_update;
} /* End of 'wnd_check_invalid' function */

/* Draw window decorations */
void wnd_draw_decorations( wnd_t *wnd )
{
	int i;

	assert(wnd);
	assert(wnd->m_title);

	/* Save window state */
	wnd_push_state(wnd, WND_STATE_COLOR | WND_STATE_CURSOR);

	/* Display border */
	if (WND_FLAGS(wnd) & WND_FLAG_BORDER)
	{
		int text_pos;
		char *border_style = wnd->m_mode == WND_MODE_NORMAL ? "border-style" :
				"repos-border-style";

		/* Set style */
		wnd_apply_style(wnd, border_style);
		
		/* Print top border */
		wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, 0);
		wnd_put_special(wnd, wnd->m_height > 1 ?
				WND_ACS_CODE(WACS_ULCORNER) : WND_ACS_CODE(WACS_LTEE));
		for ( i = 1; i < wnd->m_width - 1; i ++ )
			wnd_put_special(wnd, WND_ACS_CODE(WACS_HLINE));
		wnd_put_special(wnd, wnd->m_height > 1 ?
				WND_ACS_CODE(WACS_URCORNER) : WND_ACS_CODE(WACS_RTEE));
		
		/* Print caption */
		if (WND_FLAGS(wnd) & WND_FLAG_CAPTION)
		{
			/* Determine title position */
			text_pos = (int)(wnd->m_width - utf8_width(wnd->m_title) - 2) / 2;
			if (text_pos <= 0)
				text_pos = 1;

			/* Display window title */
			wnd_apply_style(wnd, "caption-style");
			wnd_move(wnd, WND_MOVE_ABSOLUTE, text_pos, 0);
			wnd_putc(wnd, ' ');
			wnd_putstring(wnd, WND_PRINT_NONCLIENT | WND_PRINT_ELLIPSES,
					wnd->m_width - text_pos - 2, wnd->m_title);
			wnd_putc(wnd, ' ');
		}

		/* Print left and right borders */
		wnd_apply_style(wnd, border_style);
		for ( i = 1; i < wnd->m_height - 1; i ++ )
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, i);
			wnd_put_special(wnd, WND_ACS_CODE(WACS_VLINE));
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 1, i);
			wnd_put_special(wnd, WND_ACS_CODE(WACS_VLINE));
		}

		/* Print bottom border */
		if (wnd->m_height > 1)
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, 0, wnd->m_height - 1);
			wnd_put_special(wnd, WND_ACS_CODE(WACS_LLCORNER));
			for ( i = 1; i < wnd->m_width - 1; i ++ )
				wnd_put_special(wnd, WND_ACS_CODE(WACS_HLINE));
			wnd_put_special(wnd, WND_ACS_CODE(WACS_LRCORNER));
		}

		/* Print maximize and close boxes */
		if (WND_FLAGS(wnd) & WND_FLAG_MAX_BOX)
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 3, 0);
			wnd_apply_style(wnd, "maximize-box-style");
			wnd_putc(wnd, (WND_FLAGS(wnd) & WND_FLAG_MAXIMIZED) ? 'o' : 'O');
		}
		if (WND_FLAGS(wnd) & WND_FLAG_CLOSE_BOX)
		{
			wnd_move(wnd, WND_MOVE_ABSOLUTE, wnd->m_width - 2, 0);
			wnd_apply_style(wnd, "close-box-style");
			wnd_putc(wnd, 'X');
		}
	}

	/* Display window caption */
	if (WND_FLAGS(wnd) & WND_FLAG_CAPTION)
	{
		/* Set style */
		wnd_apply_style(wnd, "border-style");
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
	wnd_apply_style(wnd_root, "wndbar-style");

	/* Print items for top-level windows */
	if (wnd_root->m_child == NULL)
	{
		wnd_printf(wnd_root, WND_PRINT_NONCLIENT, 0, _("No children\n"));
	}
	else
	{
		wnd_t *child;
		int right_pos = 0, i;

		/* Print items for children */
		wnd_move(wnd_root, WND_MOVE_ABSOLUTE, 0, wnd_root->m_height - 1);
		for ( child = wnd_root->m_child, i = 0; child != NULL; 
				child = child->m_next, i ++ )
		{
			right_pos = (i + 1) * wnd_root->m_width / wnd_root->m_num_children;

			/* Print focus child with another style */
			wnd_push_state(wnd_root, WND_STATE_COLOR | WND_STATE_ATTRIB);
			if (child == wnd_root->m_focus_child)
				wnd_apply_style(wnd_root, "wndbar-focus-style");
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

/* Get window style */
char *wnd_get_style( wnd_t *wnd, char *name )
{
	char *val;
	wnd_class_t *klass;

	assert(wnd);
	assert(name);

	/* First look up window's own configuration list */
	val = cfg_get_var(wnd->m_cfg_list, name);
	if (val != NULL)
		return val;

	/* Now look in the corresponding classes lists */
	for ( klass = wnd->m_class; klass != NULL; klass = klass->m_parent )
	{
		val = cfg_get_var(klass->m_cfg_list, name);
		if (val != NULL)
			return val;
	}

	/* Finally look up common windows setting list */
	return cfg_get_var(WND_ROOT_CFG(wnd), name);
} /* End of 'wnd_get_style' function */

/* Apply color style */
void wnd_apply_style( wnd_t *wnd, char *name )
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
	val = wnd_get_style(wnd, name);
	if (val == NULL)
		return;

	/* Parse style and set it */
	wnd_parse_color_style(val, &fg_color, &bg_color, &attrib);
	wnd_set_color(wnd, fg_color, bg_color);
	wnd_set_attrib(wnd, attrib);
} /* End of 'wnd_apply_style' function */

/* Parse color style value */
void wnd_parse_color_style( char *str, wnd_color_t *fg_color, 
		wnd_color_t *bg_color, int *attrib )
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
} /* End of 'wnd_parse_color_style' function */

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
	wnd_t *parent, *child, *prev, *cur, *last_parent = NULL;

	assert(wnd);

	/* Rearrange windows in every level until we reach current focus branch */
	for ( parent = wnd->m_parent, cur = wnd; parent != NULL;
			cur = parent, parent = parent->m_parent )
	{
		if (parent->m_focus_child == cur)
			continue;

		prev = NULL;
		last_parent = parent;
		for ( child = parent->m_child; child != NULL; child = child->m_next )
		{
			if (child->m_zval > cur->m_zval)
				child->m_zval --;
		}
		cur->m_zval = parent->m_num_children - 1;
		if (cur->m_lower_sibling != NULL)
			cur->m_lower_sibling->m_higher_sibling = cur->m_higher_sibling;
		if (cur->m_higher_sibling != NULL)
			cur->m_higher_sibling->m_lower_sibling = cur->m_lower_sibling;
		if (parent->m_focus_child != NULL)
			parent->m_focus_child->m_higher_sibling = cur;

		/* Set focus to this window */
		if (cur == parent->m_lower_child)
			parent->m_lower_child = cur->m_higher_sibling;
		cur->m_lower_sibling = parent->m_focus_child;
		parent->m_focus_child = cur;
		cur->m_higher_sibling = NULL;
	}

	/* Set the global focus */
	wnd_set_global_focus(WND_GLOBAL(wnd));

	/* Repaint */
	if (last_parent != NULL)
	{
		wnd_global_update_visibility(WND_ROOT(wnd));
		wnd_invalidate(last_parent);
	}
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
	/* Mark window as invalid */
	if (wnd != NULL)
	{
		wnd->m_is_invalid = TRUE;
		WND_GLOBAL(wnd)->m_invalid_exist = TRUE;
	}
} /* End of 'wnd_invalidate' function */

/* Send repainting messages to a window */
void wnd_send_repaint( wnd_t *wnd, bool_t send_to_children )
{
	wnd_msg_send(wnd, "display", wnd_msg_display_new());
	if (send_to_children)
	{
		wnd_t *child;
		for ( child = wnd->m_child; child != NULL; child = child->m_next )
			wnd_send_repaint(child, TRUE);
	}
} /* End of 'wnd_send_repaint' function */

/* Synchronize screen contents with display buffer */
void wnd_sync_screen( wnd_t *wnd )
{
	struct wnd_display_buf_t *buf = &WND_DISPLAY_BUF(wnd);
	struct wnd_display_buf_symbol_t *pos;
	int x = 0, y = 0;
	wnd_t *wnd_focus;
	static bool_t prev_cursor_state = TRUE;
	static int count = 0;

	pthread_mutex_lock(&WND_CURSES_MUTEX(wnd));

	/* Clear screen if buffer is dirty */
	if (buf->m_dirty)
		clear();

	/* Copy buffer to screen */
	move(0, 0);
	wnd_display_buf_lock(buf);
	for ( pos = buf->m_data;; pos ++ )
	{
		if (!pos->m_char.chars[0])
			continue;

		wadd_wch(WND_CURSES(wnd), &pos->m_char);

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
	wnd_display_buf_unlock(buf);

	/* Synchronize cursor */
	wnd_focus = WND_FOCUS(wnd);
	if (wnd_focus->m_cursor_hidden || !wnd_cursor_in_client(wnd_focus))
	{
		move(LINES - 1, COLS - 1);
		if (prev_cursor_state)
		{
			curs_set(0);
			prev_cursor_state = FALSE;
		}
	}
	else
	{
		move(WND_CLIENT2SCREEN_Y(wnd_focus, wnd_focus->m_cursor_y),
				WND_CLIENT2SCREEN_X(wnd_focus, wnd_focus->m_cursor_x));
		if (!prev_cursor_state)
		{
			curs_set(1);
			prev_cursor_state = TRUE;
		}
	}

	/* Refresh screen */
	refresh();
	buf->m_dirty = FALSE;
	pthread_mutex_unlock(&WND_CURSES_MUTEX(wnd));
} /* End of 'wnd_sync_screen' function */

/* Call message handlers chain */
wnd_msg_retcode_t wnd_call_handler( wnd_t *wnd, char *msg_name,
		wnd_msg_handler_t *handler, wnd_msg_callback_t callback, 
		wnd_msg_data_t *data )
{
	wnd_msg_retcode_t ret = WND_MSG_RETCODE_OK;
	for ( ; handler != NULL; )
	{
		wnd_msg_handler_t *next = handler->m_next;
		ret = callback(wnd, handler, data);
		/* Stop handling */
		if (ret == WND_MSG_RETCODE_STOP || ret == WND_MSG_RETCODE_EXIT)
			return ret;
		else
			handler = next;
	}
	return ret;
} /* End of 'wnd_call_handler' function */

/* Call window destructor */
void wnd_call_destructor( wnd_t *wnd )
{
	assert(wnd);
	wnd_call_handler(wnd, "destructor", wnd->m_destructor, 
			wnd_callback_destructor, NULL);
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
	wnd_t *real_wnd;

	assert(wnd);
	real_wnd = wnd_get_top_level_ancestor(wnd);
	assert(real_wnd);

	/* Only windows with border may change position or size */
	if ((mode == WND_MODE_REPOSITION || mode == WND_MODE_RESIZE) &&
			!(real_wnd->m_flags & WND_FLAG_BORDER) ||
			(mode == WND_MODE_RESIZE && 
			 (real_wnd->m_flags & WND_FLAG_NORESIZE)))
		return;
	
	/* Set new mode */
	real_wnd->m_mode = mode;

	/* Install new key handler for window parameters changing */
	wnd_msg_add_handler(wnd, "keydown", wnd_repos_on_key);
	
	/* Repaint window */
	wnd_invalidate(real_wnd);
} /* End of 'wnd_set_mode' function */

/* Handler for key pressing when in reposition or resize window mode */
wnd_msg_retcode_t wnd_repos_on_key( wnd_t *wnd, wnd_key_t key )
{
	int x, y, w, h;
	bool_t not_changed = FALSE;
	wnd_t *real_wnd = wnd_get_top_level_ancestor(wnd);

	assert(wnd);
	assert(real_wnd);

	if (real_wnd->m_mode == WND_MODE_NORMAL)
		return WND_MSG_RETCODE_OK;

	/* Choose new window parameters */
	x = real_wnd->m_x;
	y = real_wnd->m_y;
	w = real_wnd->m_width;
	h = real_wnd->m_height;
	if (real_wnd->m_mode == WND_MODE_REPOSITION)
	{
		if (key == KEY_UP)
			y --;
		else if (key == KEY_DOWN)
			y ++;
		else if (key == KEY_RIGHT)
			x ++;
		else if (key == KEY_LEFT)
			x --;
		else
			not_changed = TRUE;
	}
	else if (real_wnd->m_mode == WND_MODE_RESIZE)
	{
		if (key == KEY_UP)
			h --;
		else if (key == KEY_DOWN)
			h ++;
		else if (key == KEY_RIGHT)
			w ++;
		else if (key == KEY_LEFT)
			w --;
		else
			not_changed = TRUE;
	}
	else
		not_changed = TRUE;
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	/* Unmaximize window */
	if (WND_FLAGS(real_wnd) & WND_FLAG_MAXIMIZED)
	{
		WND_FLAGS(real_wnd) &= ~WND_FLAG_MAXIMIZED;
	}

	/* Return back to normal mode */
	if (not_changed && key == '\n')
	{
		real_wnd->m_mode = WND_MODE_NORMAL;
		wnd_msg_rem_handler(wnd, "keydown");
		wnd_invalidate(real_wnd);
		return WND_MSG_RETCODE_STOP;
	}

	/* Move window */
	if (!not_changed)
	{
		wnd_repos(real_wnd, x, y, w, h);
		return WND_MSG_RETCODE_STOP;
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'wnd_repos_on_key' function */

/* Move and resize window */
void wnd_repos( wnd_t *wnd, int x, int y, int width, int height )
{
	assert(wnd);

	/* Set new window position and size */
	wnd_repos_internal(wnd, x, y, width, height);

	/* Update visibility information */
	wnd_global_update_visibility(WND_ROOT(wnd));

	/* Repaint window */
	wnd_invalidate(wnd->m_parent == NULL ? wnd : wnd->m_parent);
} /* End of 'wnd_repos' function */

/* The internal work of 'wnd_repos' (set position and inform children) */
void wnd_repos_internal( wnd_t *wnd, int x, int y, int w, int h )
{
	int dx, dy, dw, dh;
	int px, py, pw, ph;
	wnd_t *child;

	/* Update position */
	px = wnd->m_x;
	py = wnd->m_y;
	pw = wnd->m_width;
	ph = wnd->m_height;
	dx = x - wnd->m_x;
	dy = y - wnd->m_y;
	dw = w - wnd->m_width;
	dh = h - wnd->m_height;
	wnd->m_x = x;
	wnd->m_y = y;
	wnd->m_width = w;
	wnd->m_height = h;
	if (wnd->m_parent != NULL)
	{
		wnd->m_screen_x = wnd->m_parent->m_screen_x + 
			wnd->m_parent->m_client_x + x;
		wnd->m_screen_y = wnd->m_parent->m_screen_y + 
			wnd->m_parent->m_client_y + y;
	}
	else
	{
		wnd->m_screen_x = x;
		wnd->m_screen_y = y;
	}
	wnd->m_client_w += dw;
	wnd->m_client_h += dh;
	wnd_calc_real_pos(wnd);

	/* Inform children about our repositioning */
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		/* Don't send message, but call handler directly */
		wnd_msg_callback_t callback;
		wnd_msg_t msg;

		msg.m_wnd = child;
		msg.m_name = strdup("parent_repos");
		msg.m_data = wnd_msg_parent_repos_new(px, py, pw, ph, x, y, w, h);

		wnd_msg_handler_t *handler = *wnd_class_get_msg_info(msg.m_wnd, 
				msg.m_name, &callback);
		wnd_call_handler(msg.m_wnd, msg.m_name, handler, callback, 
				&msg.m_data);
		wnd_msg_free(&msg);
	}
} /* End of 'wnd_repos_internal' function */

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

/* Reset the global focus */
void wnd_set_global_focus( wnd_global_data_t *global )
{
	wnd_t *wnd, *was_focus = global->m_focus;

	assert(global);
	for ( wnd = global->m_root; wnd->m_child != NULL; 
			wnd = wnd->m_focus_child );
	global->m_focus = wnd;

	/* Send respective messages */
	if (wnd != was_focus)
	{
		if (was_focus != NULL)
			wnd_msg_send(was_focus, "loose_focus", wnd_msg_loose_focus_new());
		wnd_msg_send(wnd, "get_focus", wnd_msg_get_focus_new());
	}
} /* End of 'wnd_set_global_focus' function */

/* Redisplay the screen (discarding all the optimization stuff) */
void wnd_redisplay( wnd_t *wnd )
{
	assert(wnd);
	assert(WND_GLOBAL(wnd));

	wnd_t *root = WND_ROOT(wnd);

	/* Clear screen */
	struct wnd_display_buf_t *buf = &WND_DISPLAY_BUF(wnd);
	wnd_display_buf_lock(buf);
	buf->m_dirty = TRUE;
	unsigned size = buf->m_width * buf->m_height;
	memset(buf->m_data, 0, size * sizeof(*buf->m_data));
	for ( unsigned i = 0; i < size; i ++ )
	{
		buf->m_data[i].m_char.chars[0] = L' ';
	}
	wnd_display_buf_unlock(buf);
	wnd_global_update_visibility(root);

	wnd_send_repaint(root, TRUE);
	wnd_msg_send(root, "update_screen", wnd_msg_update_screen_new());
} /* End of 'wnd_redisplay' function */

/* Update the whole visibility information */
void wnd_global_update_visibility( wnd_t *wnd_root )
{
	struct wnd_display_buf_t *db = &WND_DISPLAY_BUF(wnd_root);
	struct wnd_display_buf_symbol_t *pos;
	wnd_t *child;
	int i;
	assert(wnd_root);

	/* Clear current information */
	wnd_display_buf_lock(db);
	pos = db->m_data;
	for ( i = db->m_width * db->m_height; i > 0; i --, pos ++ )
		pos->m_wnd = wnd_root;

	/* Fill information */
	for ( child = wnd_root->m_lower_child; child != NULL; 
			child = child->m_higher_sibling )
		wnd_update_visibility(child);
	wnd_display_buf_unlock(db);
} /* End of 'wnd_global_update_visibility' function */

/* Update the visibility information for a window and its descendants */
void wnd_update_visibility( wnd_t *wnd )
{
	struct wnd_display_buf_t *db = &WND_DISPLAY_BUF(wnd);
	struct wnd_display_buf_symbol_t *pos;
	wnd_t *cur, *parent, *child;
	int dist, i, j;

	/* Set visibility tag for each of the window's positions */
	dist = db->m_width - (wnd->m_real_right - wnd->m_real_left);
	pos = &db->m_data[wnd->m_real_top * db->m_width + wnd->m_real_left];
	for ( i = wnd->m_real_top; i < wnd->m_real_bottom; i ++ )
	{
		for ( j = wnd->m_real_left; j < wnd->m_real_right; j ++ )
		{
			pos->m_wnd = wnd;

			/* Move to the next position */
			pos ++;
		}

		/* Move to the next line */
		pos += dist;
	}

	/* Set visibility for the children */
	for ( child = wnd->m_lower_child; child != NULL; 
			child = child->m_higher_sibling )
		wnd_update_visibility(child);
} /* End of 'wnd_update_visibility' function */

/* Calculate window real area */
void wnd_calc_real_pos( wnd_t *wnd )
{
	wnd_t *parent = wnd->m_parent;

	/* First make them equal with the screen coordinates */
	wnd->m_real_left = wnd->m_screen_x;
	wnd->m_real_right = wnd->m_screen_x + wnd->m_width;
	wnd->m_real_top = wnd->m_screen_y;
	wnd->m_real_bottom = wnd->m_screen_y + wnd->m_height;

	/* Clip only with screen boundaries when no-clip-by-parent flag
	 * is set */
	if (WND_FLAGS(wnd) & WND_FLAG_NOPARENTCLIP)
	{
		if (wnd->m_real_left < 0)
			wnd->m_real_left = 0;
		if (wnd->m_real_right >= WND_ROOT(wnd)->m_width)
			wnd->m_real_right = WND_ROOT(wnd)->m_width - 1;
		if (wnd->m_real_top < 0)
			wnd->m_real_top = 0;
		if (wnd->m_real_bottom >= WND_ROOT(wnd)->m_height)
			wnd->m_real_bottom = WND_ROOT(wnd)->m_height - 1;
		return;
	}

	/* Clip with ancestors boundaries */
	if (parent != NULL)
	{
		/* Clip with parent's client area */
		int pl = parent->m_screen_x + parent->m_client_x,
			pr = pl + parent->m_client_w,
			pt = parent->m_screen_y + parent->m_client_y,
			pb = pt + parent->m_client_h;
		if (wnd->m_real_left < pl)
			wnd->m_real_left = pl;
		if (wnd->m_real_right > pr)
			wnd->m_real_right = pr;
		if (wnd->m_real_top < pt)
			wnd->m_real_top = pt;
		if (wnd->m_real_bottom > pb)
			wnd->m_real_bottom = pb;

		/* Clip with parent's real position */
		if (wnd->m_real_left < parent->m_real_left)
			wnd->m_real_left = parent->m_real_left;
		if (wnd->m_real_right > parent->m_real_right)
			wnd->m_real_right = parent->m_real_right;
		if (wnd->m_real_top < parent->m_real_top)
			wnd->m_real_top = parent->m_real_top;
		if (wnd->m_real_bottom > parent->m_real_bottom)
			wnd->m_real_bottom = parent->m_real_bottom;
	}
} /* End of 'wnd_calc_real_pos' function */

/* Close curses */
void wnd_close_curses( wnd_t *wnd_root )
{
	WND_LIB_ACTIVE(wnd_root) = FALSE;
	clear();
	refresh();
	endwin();
} /* End of 'wnd_close_curses' function */

/* Restore curses after closing */
void wnd_restore_curses( wnd_t *wnd_root )
{
	refresh();
	WND_LIB_ACTIVE(wnd_root) = TRUE;
} /* End of 'wnd_restore_curses' function */

/* Set window title */
void wnd_set_title( wnd_t *wnd, const char *title )
{
	free(wnd->m_title);
	wnd->m_title = strdup(title);
} /* End of 'wnd_set_title' function */

/* Lock the display buffer */
void wnd_display_buf_lock( struct wnd_display_buf_t *db )
{
	pthread_mutex_lock(&db->m_mutex);
} /* End of 'wnd_display_buf_lock' function */

/* Unlock the display buffer */
void wnd_display_buf_unlock( struct wnd_display_buf_t *db )
{
	pthread_mutex_unlock(&db->m_mutex);
} /* End of 'wnd_display_buf_unlock' function */

/* Regenerate the window's children list sorted by zvalue */
void wnd_regen_zvalue_list( wnd_t *wnd )
{
	wnd_t *child, *sibling;
	assert(wnd);

	/* Set links in each of the children */
	wnd->m_lower_child = wnd->m_focus_child = NULL;
	for ( child = wnd->m_child; child != NULL; child = child->m_next )
	{
		int zval = child->m_zval;

		/* Set list begin and end pointers */
		if (zval == 0)
			wnd->m_lower_child = child;
		if (zval == wnd->m_num_children - 1)
			wnd->m_focus_child = child;

		/* Search for next and previous siblings */
		child->m_lower_sibling = child->m_higher_sibling = NULL;
		for ( sibling = wnd->m_child; sibling != NULL; 
				sibling = sibling->m_next )
		{
			if (sibling->m_zval == zval - 1)
				child->m_lower_sibling = sibling;
			else if (sibling->m_zval == zval + 1)
				child->m_higher_sibling = sibling;
		}
	}
} /* End of 'wnd_regen_zvalue_list' function */

/* Get window's top-level ancestor */
wnd_t *wnd_get_top_level_ancestor( wnd_t *wnd )
{
	assert(wnd);
	while (wnd->m_parent != NULL && wnd->m_parent->m_parent != NULL)
		wnd = wnd->m_parent;
	return wnd;
} /* End of 'wnd_get_top_level_ancestor' function */

/* Set global title */
void wnd_set_global_title( const char *title )
{
	if (wnd_set_title_seq_start && title && *title)
		printf("%s%s%s", wnd_set_title_seq_start, title, wnd_set_title_seq_end);
} /* End of 'wnd_set_global_title' function */

/* End of 'wnd.c' file */

