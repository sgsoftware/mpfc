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

#include <alsa/asoundlib.h>
#include <sys/soundcard.h>
#include <stdarg.h>
#include "types.h"
#include "cfg.h"
#include "outp.h"
#include "pmng.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

static snd_pcm_t *handle = NULL;
static snd_pcm_hw_params_t *hwparams = NULL;
static pmng_t *alsa_pmng = NULL;
static int alsa_fmt = SND_PCM_FORMAT_S16_LE;
static int alsa_size = 2;
static int alsa_rate = 44100;
static int alsa_channels = 2;
static bool_t alsa_paused = FALSE;

static char *alsa_default_dev = "plughw:0,0";

static char *alsa_desc = "ALSA output plugin";
static char *alsa_author = 
		"Thadeu Lima de Souza Cascardo <cascardo@minaslivre.org>; "
		"Sergey E. Galanov <sgsoftware@mail.ru>";
static cfg_node_t *alsa_cfg = NULL;
static logger_t *alsa_log = NULL;

void alsa_end ();
bool_t alsa_open_dev( void );

void alsa_error_handler( const char *file, int line, const char *function,
		int err, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	logger_add_message_vararg(alsa_log, LOGGER_MSG_ERROR, 1, (char *)fmt, ap);
	va_end(ap);
}

bool_t alsa_start ()
{
  int dir = 1;

  snd_lib_error_set_handler(alsa_error_handler);
  if (!alsa_open_dev())
	  return FALSE;
  hwparams = malloc (snd_pcm_hw_params_sizeof ());
  memset (hwparams, 0, snd_pcm_hw_params_sizeof());
  snd_pcm_hw_params_any (handle, hwparams);
  snd_pcm_hw_params_set_access (handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format (handle, hwparams, alsa_fmt);
  snd_pcm_hw_params_set_rate_near (handle, hwparams, &alsa_rate, &dir);
  snd_pcm_hw_params_set_channels (handle, hwparams, alsa_channels);
  snd_pcm_hw_params_set_period_time(handle, hwparams, 100000, 0);
  snd_pcm_hw_params_set_buffer_time(handle, hwparams, 500000, 0);
  if (snd_pcm_hw_params (handle, hwparams) < 0)
    {
      alsa_end ();
      return FALSE;
    }
  snd_pcm_prepare (handle);
  alsa_paused = FALSE;
  return TRUE;
}

void alsa_end ()
{
	alsa_paused = FALSE;
  if (handle != NULL)
    {
      snd_pcm_close(handle);
      handle = NULL;
    }
  if (hwparams != NULL)
    {
      free (hwparams);
      hwparams = NULL;
    }
}

void alsa_play (void *buf, int size)
{
  if (handle == NULL)
    return;
  while (snd_pcm_writei (handle, buf, size / (alsa_channels * alsa_size)) < 0)
    snd_pcm_prepare (handle);
}

void alsa_set_channels (int channels)
{
  if (handle == NULL)
    return;
  alsa_channels = channels;
  alsa_end ();
  alsa_start ();
}

void alsa_set_rate (int rate)
{
  if (handle == NULL)
    return;
  alsa_rate = rate;
  alsa_end ();
  alsa_start ();
}

void alsa_set_fmt (dword fmt)
{
  if (handle == NULL)
    return;
  switch (fmt)
    {
      case AFMT_U8:
        alsa_fmt = SND_PCM_FORMAT_U8;
	alsa_size = 1;
        break;
      case AFMT_U16_LE:
        alsa_fmt = SND_PCM_FORMAT_U16_LE;
	alsa_size = 2;
        break;
      case AFMT_U16_BE:
        alsa_fmt = SND_PCM_FORMAT_U16_BE;
	alsa_size = 2;
        break;
      case AFMT_S8:
        alsa_fmt = SND_PCM_FORMAT_S8;
	alsa_size = 1;
        break;
      case AFMT_S16_LE:
        alsa_fmt = SND_PCM_FORMAT_S16_LE;
	alsa_size = 2;
        break;
      case AFMT_S16_BE:
        alsa_fmt = SND_PCM_FORMAT_S16_BE;
	alsa_size = 2;
        break;
      default:
        return;
    }
  alsa_end ();
  alsa_start ();
}

void alsa_flush ()
{
  if (handle == NULL)
    return;
  snd_pcm_drain (handle);
}

void alsa_set_volume (int left, int right)
{
  snd_mixer_t* mix;
  snd_mixer_elem_t* elem;
  long scaled_left, scaled_right;
  long min, max;
  snd_mixer_open (&mix, 0);
  snd_mixer_attach (mix, "default");
  snd_mixer_selem_register (mix, NULL, NULL);
  snd_mixer_load (mix);
  elem = snd_mixer_first_elem (mix);
  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  if (max <= min)
  {
	  snd_mixer_close(mix);
	  return;
  }
  scaled_left = min + ((max - min) * left / 100);
  scaled_right = min + ((max - min) * right / 100);
  snd_mixer_selem_set_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT,
		  scaled_left);
  snd_mixer_selem_set_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT,
		  scaled_right);
  snd_mixer_close (mix);
}

void alsa_get_volume (int *left, int *right)
{
  snd_mixer_t* mix;
  snd_mixer_elem_t* elem;
  long min, max;
  snd_mixer_open (&mix, 0);
  snd_mixer_attach (mix, "default");
  snd_mixer_selem_register (mix, NULL, NULL);
  snd_mixer_load (mix);
  elem = snd_mixer_first_elem (mix);
  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  if (max <= min)
  {
	  snd_mixer_close(mix);
	  return;
  }
  snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT,
		  (long *)left);
  snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT,
		  (long *)right);
  (*left) = ((*left) - min) * 100 / (max - min);
  (*right) = ((*right) - min) * 100 / (max - min);
  snd_mixer_close (mix);
}

bool_t alsa_open_dev( void )
{
	char *dev;

	/* Get device name */
	dev = cfg_get_var(alsa_cfg, "device");
	if (dev == NULL)
		dev = alsa_default_dev;

	/* Try devices specified in this variable */
	while (dev != NULL)
	{
		/* Get next device name */
		char *s = strchr(dev, ';');
		if (s != NULL)
			(*s) = 0;

		/* Try to open this device */
		if (snd_pcm_open(&handle, dev, SND_PCM_STREAM_PLAYBACK, 
					SND_PCM_NONBLOCK) >= 0)
		{
			int ret;

			snd_pcm_close(handle);
			handle = NULL;
			ret = snd_pcm_open(&handle, dev, SND_PCM_STREAM_PLAYBACK, 0);
			if (s != NULL)
				(*s) = ';';
			return (ret >= 0);
		}
		if (s != NULL)
			(*s) = ';';
		dev = (s == NULL ? NULL : s + 1);
	}

	return FALSE;
}

void alsa_pause( void )
{
	if (handle != NULL && !alsa_paused)
		snd_pcm_pause(handle, TRUE);
	alsa_paused = TRUE;
}

void alsa_resume( void )
{
	if (handle != NULL && alsa_paused)
		snd_pcm_pause(handle, FALSE);
	alsa_paused = FALSE;
}

wnd_msg_retcode_t alsa_on_configure( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "device"));
	assert(eb);
	cfg_set_var(alsa_cfg, "device", EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
}

void alsa_configure( wnd_t *parent )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(parent, _("Configure ALSA plugin"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Device: "), 
			"device", cfg_get_var(alsa_cfg, "device"), 'd', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", alsa_on_configure);
	dialog_arrange_children(dlg);
}

void plugin_exchange_data (plugin_data_t *pd)
{
  pd->m_desc = alsa_desc;
  pd->m_author = alsa_author;
  pd->m_configure = alsa_configure;
  OUTP_DATA(pd)->m_start = alsa_start;
  OUTP_DATA(pd)->m_end = alsa_end;
  OUTP_DATA(pd)->m_play = alsa_play;
  OUTP_DATA(pd)->m_set_channels = alsa_set_channels;
  OUTP_DATA(pd)->m_set_freq = alsa_set_rate;
  OUTP_DATA(pd)->m_set_fmt = alsa_set_fmt;
  OUTP_DATA(pd)->m_flush = alsa_flush;
  OUTP_DATA(pd)->m_pause = alsa_pause;
  OUTP_DATA(pd)->m_resume = alsa_resume;
  OUTP_DATA(pd)->m_set_volume = alsa_set_volume;
  OUTP_DATA(pd)->m_get_volume = alsa_get_volume;
  alsa_pmng = pd->m_pmng;
  alsa_cfg = pd->m_cfg;
  alsa_log = pd->m_logger;
}
