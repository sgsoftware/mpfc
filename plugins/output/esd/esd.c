/*
 * Copyright (C) 2004 Thadeu Lima de Souza Cascardo (cascardo@minaslivre.org)
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

#include <esd.h>
#include <sys/soundcard.h>
#include <unistd.h>
#include "types.h"
#include "cfg.h"
#include "outp.h"
#include "pmng.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

static int sock = -1;
static pmng_t *esd_pmng = NULL;
static cfg_node_t *esd_cfg = NULL;
static esd_format_t esd_format = ESD_STREAM | ESD_PLAY;
static esd_format_t esd_fmt = ESD_BITS16;
static int esd_rate = 44100;
static int esd_channels = ESD_STEREO;

static char *esd_desc = "ESD output plugin";
static char *esd_author = 
		"Thadeu Lima de Souza Cascardo <cascardo@minaslivre.org>";

void esd_end ();

bool_t esd_start ()
{
  char *host = cfg_get_var (esd_cfg, "host");
  if (host == NULL)
    host = "localhost";
  sock = esd_play_stream (esd_format | esd_fmt | esd_channels, esd_rate, host, NULL);
  if (sock < 0)
    {
      esd_end ();
      return FALSE;
    }
  return TRUE;
}

void esd_end ()
{
  if (sock != -1)
    {
      esd_close (sock);
      sock = -1;
    }
}

void esd_play (void *buf, int size)
{
  if (sock == -1)
    return;
  write (sock, buf, size);
}

void esd_set_channels (int channels)
{
  if (sock == -1)
    return;
  switch (channels)
    {
      case 1:
        esd_channels = ESD_MONO;
        break;
      case 2:
      default:
        esd_channels = ESD_STEREO;
    }
  esd_end ();
  esd_start ();
}

void esd_set_rate (int rate)
{
  if (sock == -1)
    return;
  esd_rate = rate;
  esd_end ();
  esd_start ();
}

void esd_set_fmt (dword fmt)
{
  if (sock == -1)
    return;
  switch (fmt)
    {
      case AFMT_U8:
      case AFMT_S8:
        esd_fmt = ESD_BITS8;
        break;
      case AFMT_U16_LE:
      case AFMT_U16_BE:
      case AFMT_S16_LE:
      case AFMT_S16_BE:
        esd_fmt = ESD_BITS16;
	break;
      default:
        return;
    }
  esd_end ();
  esd_start ();
}

wnd_msg_retcode_t esd_on_configure( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "host"));
	assert(eb);
	cfg_set_var(esd_cfg, "host", EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
}

void esd_configure( wnd_t *parent )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(parent, _("Configure ESD plugin"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Host: "), 
			"host", cfg_get_var(esd_cfg, "host"), 'h', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", esd_on_configure);
	dialog_arrange_children(dlg);
}

void plugin_exchange_data (plugin_data_t *pd)
{
  pd->m_desc = esd_desc;
  pd->m_author = esd_author;
  pd->m_configure = esd_configure;
  OUTP_DATA(pd)->m_start = esd_start;
  OUTP_DATA(pd)->m_end = esd_end;
  OUTP_DATA(pd)->m_play = esd_play;
  OUTP_DATA(pd)->m_set_channels = esd_set_channels;
  OUTP_DATA(pd)->m_set_freq = esd_set_rate;
  OUTP_DATA(pd)->m_set_fmt = esd_set_fmt;
  esd_pmng = pd->m_pmng;
  esd_cfg = pd->m_cfg;
}
