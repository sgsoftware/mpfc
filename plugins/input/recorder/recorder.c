/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : recorder.c
 * PURPOSE     : SG MPFC. Radio input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 19.09.2004
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include "types.h"
#include "inp.h"
#include "mystring.h"
#include "pmng.h"

/* Recording sources */
enum
{
	REC_MIC,
	REC_LINE,
	REC_CD
};
static struct 
{
	char *m_name;
	int m_id;
} rec_sources[] = { {"mic", REC_MIC}, {"line", REC_LINE}, {"cd", REC_CD} };
static int rec_num_sources = sizeof(rec_sources) / sizeof(rec_sources[0]);

/* Structure for readdir */
typedef struct
{
	int m_next_file;
} rec_dir_data_t;

/* Plugins manager */
static pmng_t *rec_pmng = NULL;

/* Audio device */
static int rec_fd = -1;

/* Current recording source */
static int rec_source = -1;

/* Plugin description */
static char *rec_desc = "Sound recording plugin";

/* Plugin author */
static char *rec_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Convert file name to recording source */
static int rec_name2src( char *filename )
{
	int i;
	for ( i = 0; i < rec_num_sources; i ++ )
	{
		if (filename[0] == '/' && !strcmp(&filename[1], rec_sources[i].m_name))
			return i;
	}
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
		switch (rec_sources[rec_source].m_id)
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

	switch (rec_sources[src].m_id)
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

/* Get audio parameters */
void rec_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
	*bitrate = 0;
} /* End of 'rec_get_audio_params' function */

/* Open directory */
void *rec_opendir( char *name )
{
	rec_dir_data_t *data;
	int fd;

	/* Create directory data */
	data = (rec_dir_data_t *)malloc(sizeof(*data));
	data->m_next_file = 0;
	return data;
} /* End of 'rec_opendir' function */

/* Close directory */
void rec_closedir( void *dir )
{
	assert(dir);
	free(dir);
} /* End of 'rec_closedir' function */

/* Read directory entry */
char *rec_readdir( void *dir )
{
	rec_dir_data_t *data = (rec_dir_data_t *)dir;

	assert(dir);

	/* Finish */
	if (data->m_next_file >= rec_num_sources)
		return NULL;

	/* Return next file name */
	return rec_sources[data->m_next_file ++].m_name;
} /* End of 'rec_readdir' function */

/* Get file parameters */
int rec_stat( char *name, struct stat *sb )
{
	memset(sb, 0, sizeof(*sb));
	if (!strcmp(name, "/"))
	{
		sb->st_mode = S_IFDIR;
		return 0;
	}
	else if (rec_name2src(name) >= 0)
	{
		sb->st_mode = S_IFREG;
		return 0;
	}
	return ENOENT;
} /* End of 'rec_stat' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = rec_desc;
	pd->m_author = rec_author;
	INP_DATA(pd)->m_start = rec_start;
	INP_DATA(pd)->m_end = rec_end;
	INP_DATA(pd)->m_get_stream = rec_get_stream;
	INP_DATA(pd)->m_get_audio_params = rec_get_audio_params;
	INP_DATA(pd)->m_flags = INP_OWN_SOUND | INP_VFS;
	INP_DATA(pd)->m_set_song_title = rec_set_song_title;
	INP_DATA(pd)->m_vfs_opendir = rec_opendir;
	INP_DATA(pd)->m_vfs_readdir = rec_readdir;
	INP_DATA(pd)->m_vfs_closedir = rec_closedir;
	INP_DATA(pd)->m_vfs_stat = rec_stat;
	rec_pmng = pd->m_pmng;
} /* End of 'plugin_exchange_data' function */

/* End of 'recorder.c' file */

