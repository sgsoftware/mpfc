/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : recorder.c
 * PURPOSE     : SG MPFC. Radio input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 9.02.2004
 * NOTE        : Module prefix 'rec'.
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
#include "types.h"
#include "inp.h"
#include "mystring.h"
#include "pmng.h"

/* Recording sources */
#define REC_MIC 0
#define REC_LINE 1
#define REC_CD 2

/* Plugins manager */
static pmng_t *rec_pmng = NULL;

/* Audio device */
static int rec_fd = -1;

/* Current recording source */
static int rec_source = -1;

/* Convert file name to recording source */
static int rec_name2src( char *filename )
{
	if (strncmp(filename, "recorder:", 9))
		return -1;
	filename += 9;
	if (!strcmp(filename, "mic"))
		return REC_MIC;
	else if (!strcmp(filename, "line"))
		return REC_LINE;
	else if (!strcmp(filename, "cd"))
		return REC_CD;
	else
		return -1;
} /* End of 'rec_name2src' function */

/* Start play function */
bool_t rec_start( char *filename )
{
	int mixer_fd;
	int format = AFMT_S16_LE, rate = 44100, ch = 2;

	/* Get recording source */
	rec_source = rec_name2src(filename);
	if (rec_source < 0)
		return FALSE;
	
	/* Set recording source as line-in */
	mixer_fd = open("/dev/mixer", O_WRONLY);
	if (mixer_fd >= 0)
	{
		int mask = 0;
		switch (rec_source)
		{
		case REC_MIC:
			mask = SOUND_MASK_MIC;
			break;
		case REC_LINE:
			mask = SOUND_MASK_LINE;
			break;
		case REC_CD:
			mask = SOUND_MASK_CD;
			break;
		}
		ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &mask);
		close(mixer_fd);
	}

	/* Open audio device (for reading audio data) */
	rec_fd = open("/dev/dsp", O_RDONLY);
	if (rec_fd < 0)
		return FALSE;
	ioctl(rec_fd, SNDCTL_DSP_SETFMT, &format);
	ioctl(rec_fd, SNDCTL_DSP_CHANNELS, &ch);
	ioctl(rec_fd, SNDCTL_DSP_SPEED, &rate);
	
	return TRUE;
} /* End of 'rec_start' function */

/* End playing function */
void rec_end( void )
{
	if (rec_fd >= 0)
	{
		close(rec_fd);
		rec_fd = -1;
		rec_source = -1;
	}
} /* End of 'rec_end' function */

/* Get stream function */
int rec_get_stream( void *buf, int size )
{
	/* Read audio data */
	if (rec_fd >= 0)
		size = read(rec_fd, buf, size);
	else
		size = 0;
	return size;
} /* End of 'rec_get_stream' function */

/* Set song title */
str_t *rec_set_song_title( char *filename )
{
	int src = rec_name2src(filename);

	switch (src)
	{
	case REC_MIC:
		return str_new(_("Microphone"));
	case REC_LINE:
		return str_new(_("Line-in"));
	case REC_CD:
		return str_new(_("Audio CD"));
	default:
		return NULL;
	}
} /* End of 'rec_set_song_title' function */

/* Initialize songs that respect to object */
song_t **rec_init_obj_songs( char *name, int *num_songs )
{
	song_t **s = NULL;
	int source = -1;

	(*num_songs) = 0;
	if (!strlen(name))
		return NULL;

	/* Get source */
	if (!strcmp(name, "mic"))
		source = REC_MIC;
	else if (!strcmp(name, "line"))
		source = REC_LINE;
	else if (!strcmp(name, "cd"))
		source = REC_CD;
	if (source < 0)
		return NULL;

	/* Initialize songs */
	(*num_songs) = 1;
	s = (song_t **)malloc(sizeof(song_t *));
	s[0] = (song_t *)malloc(sizeof(song_t));
	sprintf(s[0]->m_file_name, "recorder:%s", name);
	s[0]->m_title = rec_set_song_title(s[0]->m_file_name);
	s[0]->m_len = 0;
	s[0]->m_info = NULL;
	s[0]->m_inp = NULL;
	return s;
} /* End of 'rec_init_obj_songs' function */

/* Get audio parameters */
void rec_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
	*bitrate = 0;
} /* End of 'rec_get_audio_params' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = rec_start;
	fl->m_end = rec_end;
	fl->m_get_stream = rec_get_stream;
	fl->m_get_audio_params = rec_get_audio_params;
	fl->m_init_obj_songs = rec_init_obj_songs;
	fl->m_flags = INP_OWN_SOUND;
	fl->m_set_song_title = rec_set_song_title;
	rec_pmng = fl->m_pmng;
} /* End of 'inp_get_func_list' function */

/* End of 'recorder.c' file */

