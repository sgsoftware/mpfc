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
#include "outp.h"
#include "pmng.h"

static int sock = -1;
static pmng_t *esd_pmng = NULL;
static esd_format_t esd_format = ESD_STREAM | ESD_PLAY;
static esd_format_t esd_fmt = ESD_BITS16;
static int esd_rate = 44100;
static int esd_channels = ESD_STEREO;

void esd_end ();

bool_t esd_start ()
{
  cfg_list_t *cfg = pmng_get_cfg (esd_pmng);
  char *host = cfg_get_var (cfg, "esd-host");
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

void outp_get_func_list (outp_func_list_t *fl)
{
  fl->m_start = esd_start;
  fl->m_end = esd_end;
  fl->m_play = esd_play;
  fl->m_set_channels = esd_set_channels;
  fl->m_set_freq = esd_set_rate;
  fl->m_set_fmt = esd_set_fmt;
  esd_pmng = fl->m_pmng;
}
