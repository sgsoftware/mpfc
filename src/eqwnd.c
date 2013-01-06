/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Equalizer window functions implementation.
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "eqwnd.h"
#include "help_screen.h"
#include "player.h"
#include "wnd.h"
#include "util.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_hbox.h"
#include "wnd_label.h"
#include "wnd_filebox.h"

/* Equalizer window parameters */
#define EQWND_SLIDER_START(eq)	0
#define EQWND_SLIDER_SIZE(eq) 	(WND_HEIGHT(eq) - 1)
#define EQWND_SLIDER_END(eq)	(EQWND_SLIDER_START(eq) + \
		EQWND_SLIDER_SIZE(eq) - 1)
#define EQWND_SLIDER_CENTER(eq)	((EQWND_SLIDER_START(eq) + \
			EQWND_SLIDER_END(eq)) / 2)
#define EQWND_SLIDER_WIDTH(eq)	6
#define EQWND_NUM_BANDS			11

/* Create a new equalizer window */
eq_wnd_t *eqwnd_new( wnd_t *parent )
{
	eq_wnd_t *eq;

	/* Allocate memory */
	eq = (eq_wnd_t *)malloc(sizeof(eq_wnd_t));
	if (eq == NULL)
		return NULL;
	memset(eq, 0, sizeof(*eq));
	WND_OBJ(eq)->m_class = eqwnd_class_init(WND_GLOBAL(parent));

	/* Initialize equalizer window */
	if (!eqwnd_construct(eq, parent))
	{
		free(eq);
		return NULL;
	}
	wnd_postinit(eq);
	return eq;
} /* End of 'eqwnd_new' function */

/* Initialize equalizer window */
bool_t eqwnd_construct( eq_wnd_t *eq, wnd_t *parent )
{
	wnd_t *wnd = (wnd_t *)eq;

	/* Initialize window part */
	if (!wnd_construct(wnd, parent, _("MPFC Equalizer"), 0, 0, 0, 0,
				WND_FLAG_FULL_BORDER | WND_FLAG_MAXIMIZED))
		return FALSE;

	/* Register handlers */
	wnd_msg_add_handler(wnd, "display", eqwnd_on_display);
	wnd_msg_add_handler(wnd, "action", eqwnd_on_action);
	wnd_msg_add_handler(wnd, "mouse_ldown", eqwnd_on_mouse_ldown);

	/* Set fields */
	wnd->m_cursor_hidden = TRUE;
	eq->m_pos = 0;
	eq->m_cur_value = eqwnd_get_band(eq->m_pos);
	return TRUE;
} /* End of 'eqwnd_init' function */

/* Handle display message */
wnd_msg_retcode_t eqwnd_on_display( wnd_t *wnd )
{
	eq_wnd_t *eq = (eq_wnd_t *)wnd;
	int i, start_y = 0;
	int x;
	char *str[EQWND_NUM_BANDS] = {"PREAMP", "60HZ", "170HZ", "310HZ", "600HZ",
						"1KHZ", "3KHZ", "6KHZ", "12KHZ", "14KHZ", "16KHZ"};
	
	/* Display bands sliders */
	eqwnd_scroll(eq);
	for ( i = eq->m_scroll, x = 3; i < EQWND_NUM_BANDS; i ++ )
	{
		float val;

		val = eqwnd_get_band(i);
		x += eqwnd_display_slider(eq, x, (i == eq->m_pos), val, str[i]);
		if (i == 0)
		{
			wnd_apply_style(wnd, "label-style");
			wnd_move(wnd, 0, x, EQWND_SLIDER_START(eq));
			wnd_printf(wnd, 0, 0, "+20 dB");
			wnd_move(wnd, 0, x, EQWND_SLIDER_CENTER(eq));
			wnd_printf(wnd, 0, 0, "  0 dB");
			wnd_move(wnd, 0, x, EQWND_SLIDER_END(eq));
			wnd_printf(wnd, 0, 0, "-20 dB");
			x += 10;
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'eqwnd_display' function */

/* Handle action message */
wnd_msg_retcode_t eqwnd_on_action( wnd_t *wnd, char *action )
{
	eq_wnd_t *eq = (eq_wnd_t *)wnd;

	if (!strcasecmp(action, "quit"))
	{
		wnd_close(wnd);
	}
	else if (!strcasecmp(action, "move_left"))
	{
		eq->m_pos --;
		if (eq->m_pos < 0)
			eq->m_pos = EQWND_NUM_BANDS - 1;
		eq->m_cur_value = eqwnd_get_band(eq->m_pos);
		wnd_invalidate(wnd);
	}
	else if (!strcasecmp(action, "move_right"))
	{
		eq->m_pos ++;
		if (eq->m_pos >= EQWND_NUM_BANDS)
			eq->m_pos = 0;
		eq->m_cur_value = eqwnd_get_band(eq->m_pos);
		wnd_invalidate(wnd);
	}
	else if (!strcasecmp(action, "decrease"))
	{
		eqwnd_set_band(eq->m_pos, eq->m_cur_value - 
				(40. / (EQWND_SLIDER_SIZE(eq) - 1)));
		eq->m_cur_value = eqwnd_get_band(eq->m_pos);
		player_eq_changed = TRUE;
		wnd_invalidate(wnd);
	}
	else if (!strcasecmp(action, "increase"))
	{
		eqwnd_set_band(eq->m_pos, eq->m_cur_value + 
				(40. / (EQWND_SLIDER_SIZE(eq) - 1)));
		eq->m_cur_value = eqwnd_get_band(eq->m_pos);
		player_eq_changed = TRUE;
		wnd_invalidate(wnd);
	}
	else if (!strcasecmp(action, "load_eqf"))
	{
		eqwnd_load_eqf_dlg();
	}
	else if (!strcasecmp(action, "help"))
	{
		eqwnd_help(eq);
	}
	if (player_eq_changed && 
			cfg_get_var_bool(cfg_list, "equalizer.enable-on-change"))
		cfg_set_var_bool(cfg_list, "equalizer.enabled", TRUE);
	return WND_MSG_RETCODE_OK;
} /* End of 'eqwnd_handle_key' function */

/* Display slider */
int eqwnd_display_slider( eq_wnd_t *eq, int x, bool_t hl, float val, char *str )
{
	wnd_t *wnd = WND_OBJ(eq);
	int pos, i, y;
	
	wnd_apply_style(wnd, hl ? "focus-band-style" : "band-style");
	pos = eqwnd_val2pos(val, EQWND_SLIDER_SIZE(eq));
	for ( y = EQWND_SLIDER_START(eq), i = 0; y <= EQWND_SLIDER_END(eq); 
			y ++, i ++ )
	{
		wnd_move(wnd, 0, x, y);
		if (i == pos)
		{
			wnd_put_special(wnd, ACS_BLOCK);
			wnd_put_special(wnd, ACS_BLOCK);
		}
		else
		{
			wnd_put_special(wnd, ACS_VLINE);
			wnd_put_special(wnd, ACS_VLINE);
		}
	}
	wnd_move(wnd, 0, x - 1, EQWND_SLIDER_END(eq) + 1);
	wnd_apply_style(wnd, hl ? "focus-label-style" : "label-style");
	wnd_printf(wnd, 0, 0, "%s", str);
	return EQWND_SLIDER_WIDTH(eq);
} /* End of 'eqwnd_display_slider' function */

/* Launch load preset from EQF file dialog */
void eqwnd_load_eqf_dlg( void )
{
	dialog_t *dlg;

	dlg = dialog_new(wnd_root, _("Load preset from a Winamp EQF file"));
	filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Name: "), "name", "", 
			'n', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", eqwnd_on_load);
	dialog_arrange_children(dlg);
} /* End of 'eqwnd_load_eqf_dlg' function */

/* Handle 'ok_clicked' for EQF loading dialog */
wnd_msg_retcode_t eqwnd_on_load( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "name"));
	assert(eb);
	eqwnd_load_eqf(EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'eqwnd_on_load' function */

/* Load a Winamp EQF file */
void eqwnd_load_eqf( char *filename )
{
	FILE *fd;
	char header[31];
	byte bands[EQWND_NUM_BANDS];
	int i;

	/* Open file */
	fd = util_fopen(filename, "rb");
	if (fd == NULL)
		return;

	/* Read data */
	if (fread(header, 1, 31, fd) != 31)
	{
		fclose(fd);
		return;
	}
	if (!strncmp(header, "Winamp EQ library file v1.1", 27))
	{
		if (fseek(fd, 257, SEEK_CUR) == -1)	/* Skip name */
		{
			fclose(fd);
			return;
		}
		if (fread(bands, 1, EQWND_NUM_BANDS, fd) != EQWND_NUM_BANDS)
		{
			fclose(fd);
			return;
		}

		eqwnd_set_band(0, 20.0 - ((bands[10] * 40.0) / 63.0));
		for ( i = 0; i < 10; i ++ )
		{
			eqwnd_set_band(i + 1, 20.0 - ((bands[i] * 40.0) / 64.0));
		}
	}

	/* Close file */
	fclose(fd);

	/* Report about changing */
	player_eq_changed = TRUE;
} /* End of 'eqwnd_load_eqf' function */

/* Handle mouse left button click */
wnd_msg_retcode_t eqwnd_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type )
{
	int band;
	eq_wnd_t *eq = (eq_wnd_t *)wnd;

	/* Check if this point is inside any of the band sliders */
	for ( band = 0; band < EQWND_NUM_BANDS; band ++ )
	{
		int sx, sy, sw, sh;

		eqwnd_get_slider_pos(eq, band, &sx, &sy, &sw, &sh);
		if (x >= sx && y >= sy && x <= sx + sw && y <= sy + sh)
		{
			eq->m_pos = band;
			eq->m_cur_value = eqwnd_pos2val(y - sy, sh);
			eqwnd_set_band(band, eq->m_cur_value);
			wnd_invalidate(wnd);
			break;
		}
	}
	return WND_MSG_RETCODE_OK;
} /* End of 'eqwnd_on_mouse_ldown' function */

/* Get equalizer band slider position */
void eqwnd_get_slider_pos( eq_wnd_t *eq, int band, int *x, int *y, int *w, 
		int *h )
{
	*x = 2 + (band - eq->m_scroll) * EQWND_SLIDER_WIDTH(eq);
	if (band > 0 && eq->m_scroll == 0)
		(*x) += 10;
	*y = EQWND_SLIDER_START(eq);
	*w = EQWND_SLIDER_WIDTH(eq);
	*h = EQWND_SLIDER_SIZE(eq);
} /* End of 'eqwnd_get_slider_pos' function */

/* Convert band value to position */
int eqwnd_val2pos( float val, int height )
{
	return rintf(((-val) + 20.) * (height - 1) / 40.);
} /* End of 'eqwnd_val2pos' function */

/* Convert position to band value */
float eqwnd_pos2val( int pos, int height )
{
	return 20. - (pos * 40. / (height - 1));
} /* End of 'eqwnd_pos2val' function */

/* Show help screen */
void eqwnd_help( eq_wnd_t *eq )
{
	help_new(WND_ROOT(eq), HELP_EQWND);
} /* End of 'eqwnd_help' function */

/* Initialize equalizer window class */
wnd_class_t *eqwnd_class_init( wnd_global_data_t *global )
{
	wnd_class_t *klass = wnd_class_new(global, "equalizer", 
			wnd_basic_class_init(global), NULL, NULL,
			eqwnd_class_set_default_styles);
	return klass;
} /* End of 'eqwnd_class_init' function */

/* Set equalizer window class default styles */
void eqwnd_class_set_default_styles( cfg_node_t *list )
{
	cfg_set_var(list, "band-style", "cyan:black");
	cfg_set_var(list, "focus-band-style", "cyan:black:bold");
	cfg_set_var(list, "label-style", "white:black");
	cfg_set_var(list, "focus-label-style", "white:black:bold");

	/* Set kbinds */
	cfg_set_var(list, "kbind.quit", "q;<Escape>");
	cfg_set_var(list, "kbind.move_left", "h;<Left>;<Ctrl-b>");
	cfg_set_var(list, "kbind.move_right", "l;<Right>;<Ctrl-f>");
	cfg_set_var(list, "kbind.increase", "k;<Up>;<Ctrl-p>");
	cfg_set_var(list, "kbind.decrease", "j;<Down>;<Ctrl-n>");
	cfg_set_var(list, "kbind.load_eqf", "p");
	cfg_set_var(list, "kbind.help", "?");
} /* End of 'eqwnd_class_set_default_styles' function */

/* Get equlizer band value */
float eqwnd_get_band( int band )
{
	char var_name[256];

	if (band == 0)
		snprintf(var_name, sizeof(var_name), "equalizer.preamp");
	else
		snprintf(var_name, sizeof(var_name), "equalizer.band%d", band);
	return cfg_get_var_float(cfg_list, var_name);
} /* End of 'eqwnd_get_band' function */

/* Set equalizer band value */
void eqwnd_set_band( int band, float val )
{
	char var_name[256];

	/* Clip value */
	if (val < -20.)
		val = -20.;
	else if (val > 20.)
		val = 20.;

	if (band == 0)
		snprintf(var_name, sizeof(var_name), "equalizer.preamp");
	else
		snprintf(var_name, sizeof(var_name), "equalizer.band%d", band);
	cfg_set_var_float(cfg_list, var_name, val);
} /* End of 'eqwnd_set_band' function */

/* Scroll window if current band is not visible */
void eqwnd_scroll( eq_wnd_t *eq )
{
	for ( ; eq->m_scroll >= 0 && eq->m_scroll < EQWND_NUM_BANDS - 1; )
	{
		int x, y, w, h;
		eqwnd_get_slider_pos(eq, eq->m_pos, &x, &y, &w, &h);
		if (x < 0)
			eq->m_scroll --;
		else if (x >= WND_WIDTH(eq))
			eq->m_scroll ++;
		else
			break;
	}
} /* End of 'eqwnd_scroll' function */

/* End of 'eqwnd.c' file */

