/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : echo.h
 * PURPOSE     : SG MPFC. Echo effect plugin.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
 * NOTE        : Module prefix 'echo'.
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

#include <sys/soundcard.h>
#include "types.h"
#include "cfg.h"
#include "ep.h"
#include "pmng.h"
#include "util.h"
#include "wnd.h"
#include "wnd_checkbox.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define MAX_DELAY 1000
#define MAX_SRATE 50000
#define MAX_CHANNELS 2
#define BYTES_PS 2
#define BUFFER_SAMPLES (MAX_SRATE * MAX_DELAY / 1000)
#define BUFFER_SHORTS (BUFFER_SAMPLES * MAX_CHANNELS)
#define BUFFER_BYTES (BUFFER_SHORTS * BYTES_PS)

static short *buffer = NULL;
static int w_ofs = 0;

/* Plugins manager */
static pmng_t *echo_pmng = NULL;

/* Plugin description */
static char *echo_desc = "Echo effect plugin";

/* Plugin author */
static char *echo_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Configuration lists */
static cfg_node_t *echo_root_cfg = NULL, *echo_cfg = NULL;

/* Apply echo plugin function */
int echo_apply( byte *d, int len, int fmt, int srate, int nch )
{
	int i, in, out, buf, r_ofs, fb_div;
	short *data = (short *)d;
	static int old_srate, old_nch;
	bool_t surround_enable;
	int del, volume, feedback;

	if (!(fmt == AFMT_S16_NE || fmt == AFMT_S16_LE || fmt == AFMT_S16_BE))
		return len;

	surround_enable = cfg_get_var_bool(echo_cfg, "surround-enable");
	del = cfg_get_var_int(echo_cfg, "delay");
	volume = cfg_get_var_int(echo_cfg, "volume");
	feedback = cfg_get_var_int(echo_cfg, "feedback");

	if (!buffer)
		buffer = (short *)malloc(BUFFER_BYTES + 2);

	if (nch != old_nch || srate != old_srate)
	{
		memset(buffer, 0, BUFFER_BYTES);
		w_ofs = 0;
		old_nch = nch;
		old_srate = srate;
	}

	if (surround_enable && nch == 2)
		fb_div = 200;
	else
		fb_div = 100;

	r_ofs = w_ofs - (srate * del / 1000) * nch;
	if (r_ofs < 0)
		r_ofs += BUFFER_SHORTS;

	for (i = 0; i < len / BYTES_PS; i++)
	{
		in = data[i];
		buf = buffer[r_ofs];
		if (surround_enable && nch == 2)
		{
			if (i & 1)
				buf -= buffer[r_ofs - 1];
			else
				buf -= buffer[r_ofs + 1];
		}
		out = in + buf * volume / 100;
		buf = in + buf * feedback / fb_div;
		out = CLAMP(out, -32768, 32767);
		buf = CLAMP(buf, -32768, 32767);
		buffer[w_ofs] = buf;
		data[i] = out;
		if (++r_ofs >= BUFFER_SHORTS)
			r_ofs -= BUFFER_SHORTS;
		if (++w_ofs >= BUFFER_SHORTS)
			w_ofs -= BUFFER_SHORTS;
	}

	return len;
} /* End of 'echo_apply' function */

/* Set default configuration values */
void plugin_set_cfg_default( cfg_node_t *list )
{
	cfg_set_var_int(list, "delay", 500);
	cfg_set_var_int(list, "volume", 50);
	cfg_set_var_int(list, "feedback", 50);
} /* End of 'echo_set_cfg_default' function */

/* Handle 'ok_clicked' message for configuration dialog */
wnd_msg_retcode_t echo_on_configure( wnd_t *wnd )
{
	editbox_t *eb_delay, *eb_volume, *eb_feedback;
	checkbox_t *cb_surround;
	
	/* Get controls */
	eb_delay = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "delay"));
	eb_volume = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "volume"));
	eb_feedback = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "feedback"));
	cb_surround = CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "surround"));
	assert(eb_delay && eb_volume && eb_feedback && cb_surround);
	
	/* Set values */
	cfg_set_var(echo_cfg, "delay", EDITBOX_TEXT(eb_delay));
	cfg_set_var(echo_cfg, "volume", EDITBOX_TEXT(eb_volume));
	cfg_set_var(echo_cfg, "feedback", EDITBOX_TEXT(eb_feedback));
	cfg_set_var_bool(echo_cfg, "surround-enable", cb_surround->m_checked);
	return WND_MSG_RETCODE_OK;
} /* End of 'echo_on_configure' function */

/* Launch configuration dialog */
void echo_configure( wnd_t *parent )
{
	dialog_t *dlg;

	dlg = dialog_new(parent, _("Configure echo plugin"));
	editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Delay: "), 
			"delay", cfg_get_var(echo_cfg, "delay"), 'd', 50);
	editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Volume: "), 
			"volume", cfg_get_var(echo_cfg, "volume"), 'v', 50);
	editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Feedback: "), 
			"feedback", cfg_get_var(echo_cfg, "feedback"), 'f', 50);
	checkbox_new(WND_OBJ(dlg->m_vbox), _("Enable s&urround"), "surround", 
			'u', cfg_get_var_bool(echo_cfg, "surround-enable"));
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", echo_on_configure);
	dialog_arrange_children(dlg);
} /* End of 'echo_configure' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = echo_desc;
	pd->m_author = echo_author;
	pd->m_configure = echo_configure;
	EP_DATA(pd)->m_apply = echo_apply;
	echo_pmng = pd->m_pmng;
	echo_cfg = pd->m_cfg;
	echo_root_cfg = pd->m_root_cfg;
} /* End of 'plugin_exchange_data' function */

/* End of 'echo.c' file */

