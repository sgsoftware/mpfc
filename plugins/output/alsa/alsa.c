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
#include "types.h"
#include "outp.h"
#include "pmng.h"

static snd_pcm_t *handle = NULL;
static snd_pcm_hw_params_t *hwparams = NULL;
static pmng_t *alsa_pmng = NULL;
static int alsa_fmt = SND_PCM_FORMAT_S16_LE;
static int alsa_size = 2;
static int alsa_rate = 44100;
static int alsa_channels = 2;

void alsa_end ();

bool_t alsa_start ()
{
  int dir = 1;

  if (snd_pcm_open (&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0) < 0)
    return FALSE;
  hwparams = malloc (snd_pcm_hw_params_sizeof ());
  memset (hwparams, 0, snd_pcm_hw_params_sizeof());
  snd_pcm_hw_params_any (handle, hwparams);
  snd_pcm_hw_params_set_access (handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format (handle, hwparams, alsa_fmt);
  snd_pcm_hw_params_set_rate_near (handle, hwparams, &alsa_rate, &dir);
  snd_pcm_hw_params_set_channels (handle, hwparams, alsa_channels);
  snd_pcm_hw_params_set_periods (handle, hwparams, 2, 0);
  snd_pcm_hw_params_set_period_size (handle, hwparams, 2048, 0);
  snd_pcm_hw_params_set_buffer_size (handle, hwparams, 4096);
  if (snd_pcm_hw_params (handle, hwparams) < 0)
    {
      alsa_end ();
      return FALSE;
    }
  return TRUE;
}

void alsa_end ()
{
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
  snd_mixer_open (&mix, 0);
  snd_mixer_attach (mix, "default");
  snd_mixer_selem_register (mix, NULL, NULL);
  snd_mixer_load (mix);
  elem = snd_mixer_first_elem (mix);
  snd_mixer_selem_set_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, (long)left);
  snd_mixer_selem_set_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT, (long)right);
  snd_mixer_close (mix);
}

void alsa_get_volume (int *left, int *right)
{
  snd_mixer_t* mix;
  snd_mixer_elem_t* elem;
  snd_mixer_open (&mix, 0);
  snd_mixer_attach (mix, "default");
  snd_mixer_selem_register (mix, NULL, NULL);
  snd_mixer_load (mix);
  elem = snd_mixer_first_elem (mix);
  snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, (long*)left);
  snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT, (long*)right);
  snd_mixer_close (mix);
}

void outp_get_func_list (outp_func_list_t *fl)
{
  fl->m_start = alsa_start;
  fl->m_end = alsa_end;
  fl->m_play = alsa_play;
  fl->m_set_channels = alsa_set_channels;
  fl->m_set_freq = alsa_set_rate;
  fl->m_set_fmt = alsa_set_fmt;
  fl->m_flush = alsa_flush;
  fl->m_set_volume = alsa_set_volume;
  fl->m_get_volume = alsa_get_volume;
  alsa_pmng = fl->m_pmng;
}
