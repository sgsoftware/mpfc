/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : radio.c
 * PURPOSE     : SG MPFC. Radio input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 1.08.2003
 * NOTE        : Module prefix 'rad'.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include <linux/videodev.h>
#include "types.h"
#include "cfg.h"
#include "inp.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* Convert file name to frequency */
#define rad_name2freq(name) atof(&name[6])

/* This is pointer to global variables list */
cfg_list_t *rad_var_list = NULL;

/* Start play function */
bool rad_start( char *filename )
{
	int fd, f;
	struct video_audio va;
	struct video_tuner vt;
	float freq;

	if ((fd = open("/dev/radio0", O_RDONLY)) < 0)
		return FALSE;
	ioctl(fd, VIDIOCGAUDIO, &va);
	va.flags = 0;
	va.volume = 8192;
	va.audio = 0;
	va.balance = 32768;
	vt.tuner = 0;
	ioctl(fd, VIDIOCSTUNER, &vt);
	freq = rad_name2freq(filename);
	f = (int)(freq * 16000.);
	ioctl(fd, VIDIOCSFREQ, &f);
	va.mode = VIDEO_SOUND_STEREO;
	ioctl(fd, VIDIOCSAUDIO, &va);

	close(fd);
	return TRUE;
} /* End of 'rad_start' function */

/* End playing function */
void rad_end( void )
{
	int fd;
	struct video_audio va;

	fd = open("/dev/radio0", O_RDONLY);
	if (fd < 0)
		return;

	memset(&va, 0, sizeof(va));
	va.flags = VIDEO_AUDIO_MUTE;
	ioctl(fd, VIDIOCSAUDIO, &va);
	close(fd);
} /* End of 'rad_end' function */

/* Get stream function */
int rad_get_stream( void *buf, int size )
{
	return size;
} /* End of 'rad_get_stream' function */

/* Initialize songs that respect to object */
song_t **rad_init_obj_songs( char *name, int *num_songs )
{
	song_t **s = NULL;
	float freq, f, fact;
	struct video_audio va;
	struct video_tuner vt;
	int fd;

	if (!strlen(name))
		return;

	/* Get tuner parameters */
	fd = open("/dev/radio0", O_RDONLY);
	if (fd < 0)
	{
		return NULL;
	}
	vt.tuner = 0;
	if (ioctl(fd, VIDIOCGTUNER, &vt) < 0)
	{
		close(fd);
		return NULL;
	}

	/* Check frequency */
	freq = (float)atof(name);
	f = freq * 16000.;
	if (vt.rangelow > 0 && (f < vt.rangelow || f > vt.rangehigh))
	{
		close(fd);
		return NULL;
	}
	close(fd);

	/* Initialize songs */
	(*num_songs) = 1;
	s = (song_t **)malloc(sizeof(song_t *));
	s[0] = (song_t *)malloc(sizeof(song_t));
	sprintf(s[0]->m_file_name, "Radio:%f", freq);
	strcpy(s[0]->m_title, s[0]->m_file_name);
	s[0]->m_len = 0;
	s[0]->m_info = NULL;
	s[0]->m_inp = NULL;
	return s;
} /* End of 'rad_init_obj_songs' function */

/* Pause */
void rad_pause( void )
{
	int fd;
	struct video_audio va;

	fd = open("/dev/radio0", O_RDONLY);
	if (fd < 0)
		return;

	memset(&va, 0, sizeof(va));
	va.flags = VIDEO_AUDIO_MUTE;
	ioctl(fd, VIDIOCSAUDIO, &va);
	close(fd);
} /* End of 'rad_pause' function */

/* Resume */
void rad_resume( void )
{
	int fd;
	struct video_audio va;

	fd = open("/dev/radio0", O_RDONLY);
	if (fd < 0)
		return;

	memset(&va, 0, sizeof(va));
	va.balance = 32768;
	va.volume = 8192;
	ioctl(fd, VIDIOCSAUDIO, &va);
	close(fd);
} /* End of 'rad_resume' function */

/* Get audio parameters */
void rad_get_audio_params( int *ch, int *freq, dword *fmt )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
} /* End of 'rad_get_audio_params' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = rad_start;
	fl->m_end = rad_end;
	fl->m_get_stream = rad_get_stream;
	fl->m_get_audio_params = rad_get_audio_params;
	fl->m_init_obj_songs = rad_init_obj_songs;
	fl->m_flags = INP_NO_OUTP;
	fl->m_pause = rad_pause;
	fl->m_resume = rad_resume;
} /* End of 'inp_get_func_list' function */

/* Save variables list */
void inp_set_vars( cfg_list_t *list )
{
	rad_var_list = list;
} /* End of 'inp_set_vars' function */

/* End of 'radio.c' file */

