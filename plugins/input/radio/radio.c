/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : radio.c
 * PURPOSE     : SG MPFC. Radio input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 20.09.2004
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <linux/videodev.h>
#include "types.h"
#include "inp.h"
#include "mystring.h"
#include "pmng.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* Convert file name to frequency */
#define rad_name2freq(name) atof(&name[1])

/* Plugins manager */
static pmng_t *rad_pmng = NULL;

/* Audio device */
static int audio_fd = -1;

/* Start play function */
bool_t rad_start( char *filename )
{
	int fd, f;
	struct video_audio va;
	struct video_tuner vt;
	float freq;
	int format = AFMT_S16_LE, ch = 2, rate = 44100;
	int mixer_fd;

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
	if (vt.rangelow > 0 && (f < vt.rangelow || f > vt.rangehigh))
	{
		close(fd);
		fd = -1;
		return FALSE;
	}
	ioctl(fd, VIDIOCSFREQ, &f);
	va.mode = VIDEO_SOUND_STEREO;
	ioctl(fd, VIDIOCSAUDIO, &va);
	close(fd);

	/* Set recording source as line-in */
	mixer_fd = open("/dev/mixer", O_WRONLY);
	if (mixer_fd >= 0)
	{
		int mask = SOUND_MASK_LINE;
		ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &mask);
		close(mixer_fd);
	}

	/* Open audio device (for reading audio data) */
	audio_fd = open("/dev/dsp", O_RDONLY);
	if (audio_fd >= 0)
	{
		ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format);
		ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &ch);
		ioctl(audio_fd, SNDCTL_DSP_SPEED, &rate);
	}
	
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

	if (audio_fd >= 0)
	{
		close(audio_fd);
		audio_fd = -1;
	}
} /* End of 'rad_end' function */

/* Get stream function */
int rad_get_stream( void *buf, int size )
{
	/* Read audio data */
	if (audio_fd >= 0)
	{
		int ret_size = read(audio_fd, buf, size);
		if (ret_size > 0)
			size = ret_size;
	}
	return size;
} /* End of 'rad_get_stream' function */

/* Set song title */
str_t *rad_set_song_title( char *filename )
{
	str_t *str;
	str = str_new("");
	str_printf(str, "Radio at %.2f Hz", rad_name2freq(filename));
	return str;
} /* End of 'rad_set_song_title' function */

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
void rad_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
	*bitrate = 0;
} /* End of 'rad_get_audio_params' function */

/* Get file parameters */
int rad_stat( char *name, struct stat *sb )
{
	memset(sb, 0, sizeof(*sb));
	if (!strcmp(name, "/"))
	{
		sb->st_mode = S_IFDIR;
		return 0;
	}
	else if (name[0] == '/')
	{
		sb->st_mode = S_IFREG;
		return 0;
	}
	return ENOENT;
} /* End of 'acd_stat' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = rad_start;
	fl->m_end = rad_end;
	fl->m_get_stream = rad_get_stream;
	fl->m_get_audio_params = rad_get_audio_params;
	fl->m_flags = INP_OWN_SOUND | INP_VFS | INP_VFS_NOT_FIXED;
	fl->m_pause = rad_pause;
	fl->m_resume = rad_resume;
	fl->m_set_song_title = rad_set_song_title;
	fl->m_vfs_stat = rad_stat;
	rad_pmng = fl->m_pmng;
} /* End of 'inp_get_func_list' function */

/* End of 'radio.c' file */

