/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : audiocd.c
 * PURPOSE     : SG MPFC. Audio CD input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 19.12.2003
 * NOTE        : Module prefix 'acd'.
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

#include <linux/cdrom.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include "types.h"
#include "audiocd.h"
#include "cddb.h"
#include "inp.h"
#include "pmng.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* Get track number from filename */
#define acd_fname2trk(name) (strncmp(name, "audiocd:track", 13) ? -1 : \
		atoi(&(name)[13]) - 1)

/* Calculate track length */
#define acd_get_trk_len(trk) \
	((acd_tracks_info[trk].m_end_min * 60 + acd_tracks_info[trk].m_end_sec) - \
	((acd_tracks_info[trk].m_start_min * 60 + \
	  acd_tracks_info[trk].m_start_sec)))

/* Plugins manager */
pmng_t *acd_pmng = NULL;

/* Tracks information array */
struct acd_trk_info_t acd_tracks_info[ACD_MAX_TRACKS];
int acd_num_tracks = 0;
static int acd_cur_track = -1;
static bool_t acd_info_read = FALSE;

/* Current time */
static int acd_time = 0;

/* Whether running first time? */
static bool_t acd_first_time = TRUE;

/* The next track */
static char acd_next_song[MAX_FILE_NAME] = "";

/* Message printer */
pmng_print_msg_t acd_print_msg = NULL;

/* Audio device */
static int audio_fd = -1;

/* Prepare cdrom for ioctls */
static int acd_prepare_cd( void )
{
	int fd;
	char *dev;

	/* Get device name */
	dev = cfg_get_var(pmng_get_cfg(acd_pmng), "audiocd-device");
	if (dev == NULL)
		dev = "/dev/cdrom";
	
	/* Open device */
	fd = open(dev, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return fd;

	/* Check drive */
	if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) != CDS_DISC_OK)
	{
		close(fd);
		return -1;
	}
	return fd;
} /* End of 'acd_prepare_cd' function */

/* Start play function */
bool_t acd_start( char *filename )
{
	int fd, track;
	struct cdrom_msf msf;
	struct cdrom_subchnl info;
	bool_t playing = FALSE;
	int mixer_fd;
	int format = AFMT_S16_LE, ch = 2, rate = 44100;

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return FALSE;
	acd_cur_track = track;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return FALSE;

	/* Start playing */
	msf.cdmsf_min0 = acd_tracks_info[track].m_start_min;
	msf.cdmsf_sec0 = acd_tracks_info[track].m_start_sec;
	msf.cdmsf_frame0 = acd_tracks_info[track].m_start_frm;
	msf.cdmsf_min1 = acd_tracks_info[track].m_end_min;
	msf.cdmsf_sec1 = acd_tracks_info[track].m_end_sec;
	msf.cdmsf_frame1 = acd_tracks_info[track].m_end_frm;
	if (ioctl(fd, CDROMPLAYMSF, &msf) < 0)
	{
		close(fd);
		return FALSE;
	}
	acd_time = 0;
	acd_info_read = FALSE;
	strcpy(acd_next_song, "");

	/* Close device */
	close(fd);

	/* Set recording source as cd */
	mixer_fd = open("/dev/mixer", O_WRONLY);
	if (mixer_fd >= 0)
	{
		int mask = SOUND_MASK_CD;
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
	
	/* Open audio device */
	return TRUE;
} /* End of 'mp3_start' function */

/* End playing function */
void acd_end( void )
{
	int fd;

	/* Open device */
	acd_time = 0;
	acd_info_read = FALSE;
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Stop playing */
	if (strncmp(acd_next_song, "audiocd", 7))
		ioctl(fd, CDROMSTOP, 0);

	/* Close device */
	close(fd);
	cddb_free();

	if (audio_fd >= 0)
	{
		close(audio_fd);
		audio_fd = -1;
	}
} /* End of 'acd_end' function */

/* Get length */
static int acd_get_len( char *filename )
{
	int track;
	
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return 0;
	return acd_tracks_info[track].m_len;
} /* End of 'acd_get_len' function */

/* Get stream function */
int acd_get_stream( void *buf, int size )
{
	int fd;
	struct cdrom_subchnl info;
	bool_t playing;

	/* Check if we are playing now */
	if ((fd = acd_prepare_cd()) < 0)
		return 0;
	info.cdsc_format = CDROM_MSF;
	ioctl(fd, CDROMSUBCHNL, &info);
	playing = (info.cdsc_audiostatus == CDROM_AUDIO_PLAY);
	acd_time = info.cdsc_reladdr.msf.minute * 60 + 
		info.cdsc_reladdr.msf.second;
	close(fd);
	if (!playing)
		return 0;

	/* Read audio data */
	if (audio_fd >= 0)
	{
		int ret_size = read(audio_fd, buf, size);
		if (ret_size > 0)
			size = ret_size;
	}
	return size;
} /* End of 'acd_get_stream' function */

/* Initialize songs that respect to object */
song_t **acd_init_obj_songs( char *name, int *num_songs )
{
	song_t **s = NULL;
	int fd, i, j;
	struct cdrom_tochdr toc;
	struct cdrom_tocentry entry;
	int track;

	/* Get track number that we want to add */
	if (!*name)
		track = 0;
	else if (strncmp(name, "track", 5))
	{
		*num_songs = 0;
		return NULL;
	}
	else
		track = atoi(&name[5]);

	/* Open cdrom device */
	(*num_songs) = 0;
	if ((fd = acd_prepare_cd()) < 0)
		return NULL;

	/* Free CDDB data */
	cddb_free();

	/* Read tracks information */
	if (acd_first_time || ioctl(fd, CDROM_MEDIA_CHANGED, 0))
	{
		ioctl(fd, CDROMREADTOCHDR, &toc);
		acd_num_tracks = toc.cdth_trk1 - toc.cdth_trk0 + 1;
		entry.cdte_format = CDROM_MSF;
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			entry.cdte_track = i + toc.cdth_trk0;
			ioctl(fd, CDROMREADTOCENTRY, &entry);
			acd_tracks_info[i].m_start_min = entry.cdte_addr.msf.minute;
			acd_tracks_info[i].m_start_sec = entry.cdte_addr.msf.second;
			acd_tracks_info[i].m_start_frm = entry.cdte_addr.msf.frame;
			acd_tracks_info[i].m_data = entry.cdte_ctrl & CDROM_DATA_TRACK;
			acd_tracks_info[i].m_number = i + toc.cdth_trk0;
		}
		for ( i = 0; i < acd_num_tracks - 1; i ++ )
		{
			acd_tracks_info[i].m_end_min = acd_tracks_info[i + 1].m_start_min;
			acd_tracks_info[i].m_end_sec = acd_tracks_info[i + 1].m_start_sec;
			acd_tracks_info[i].m_end_frm = acd_tracks_info[i + 1].m_start_frm;
			acd_tracks_info[i].m_len = acd_get_trk_len(i);
		}
		entry.cdte_track = CDROM_LEADOUT;
		ioctl(fd, CDROMREADTOCENTRY, &entry);
		acd_tracks_info[i].m_end_min = entry.cdte_addr.msf.minute;
		acd_tracks_info[i].m_end_sec = entry.cdte_addr.msf.second;
		acd_tracks_info[i].m_end_frm = entry.cdte_addr.msf.frame;
		acd_tracks_info[i].m_end_min = entry.cdte_addr.msf.minute;
		acd_tracks_info[i].m_len = acd_get_trk_len(i);
		acd_first_time = FALSE;
	}

	/* Determine number of songs */
	for ( i = 0; i < acd_num_tracks; i ++ )
		if (!acd_tracks_info[i].m_data && (!track || i == track - 1))
			(*num_songs) ++;
			
	/* Return nothing if no tracks are about to be added */
	if (!(*num_songs) || (track && (track < 0 || track > acd_num_tracks)))
		return NULL;

	/* Initialize songs */
	s = (song_t **)malloc((*num_songs) * sizeof(song_t *));
	for ( i = !track ? 0 : track - 1, j = 0; i < acd_num_tracks; i ++ )
	{
		song_t *song;
			
		if (acd_tracks_info[i].m_data)
			continue;
		s[j ++] = song = (song_t *)malloc(sizeof(song_t));
		sprintf(song->m_file_name, "audiocd:track%02d", 
				acd_tracks_info[i].m_number);
		song->m_title = acd_set_song_title(song->m_file_name);
		song->m_len = acd_tracks_info[i].m_len;
		song->m_inp = NULL;
		song->m_info = NULL;

		if (track)
			break;
	}

	/* Close device */
	close(fd);
	return s;
} /* End of 'acd_init_obj_songs' function */

/* Pause */
void acd_pause( void )
{
	int fd;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Pause */
	if (ioctl(fd, CDROMPAUSE, 0) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_pause' function */

/* Resume */
void acd_resume( void )
{
	int fd;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Pause */
	if (ioctl(fd, CDROMRESUME, 0) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_resume' function */

/* Seek */
void acd_seek( int shift )
{
	int fd;
	struct cdrom_subchnl info;
	struct cdrom_msf msf;

	if (acd_cur_track < 0 || acd_cur_track >= acd_num_tracks)
		return;

	/* Start playing from new position */
	if ((fd = acd_prepare_cd()) < 0)
		return;
	shift = acd_tracks_info[acd_cur_track].m_start_min * 60 + 
		acd_tracks_info[acd_cur_track].m_start_sec + shift;
	msf.cdmsf_min0 = shift / 60;
	msf.cdmsf_sec0 = shift % 60;
	msf.cdmsf_frame0 = 0;
	msf.cdmsf_min1 = acd_tracks_info[acd_cur_track].m_end_min;
	msf.cdmsf_sec1 = acd_tracks_info[acd_cur_track].m_end_sec;
	msf.cdmsf_frame1 = acd_tracks_info[acd_cur_track].m_end_frm;
	if (ioctl(fd, CDROMPLAYMSF, &msf) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_seek' function */

/* Get audio parameters */
void acd_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
	*bitrate = 0;
} /* End of 'acd_get_audio_params' function */

/* Get current time */
int acd_get_cur_time( void ) 
{
	return acd_time;
} /* End of 'acd_get_cur_time' function */

/* Get song info */
song_info_t *acd_get_info( char *filename, int *len )
{
	int track;

	/* Get length */
	*len = acd_get_len(filename);

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return NULL;

	/* Read whole disc info */
	if (!cddb_read())
		return NULL;

	/* Save info for specified track */
	return cddb_get_trk_info(track);
} /* End of 'acd_get_info' function */

/* Save song info */
void acd_save_info( char *filename, song_info_t *info )
{
	int track;

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return;

	/* Save info */
	cddb_save_trk_info(track, info);
} /* End of 'acd_save_info' function */

/* Set next song name */
void acd_set_next_song( char *name )
{
	if (name == NULL)
		strcpy(acd_next_song, "");
	else
		strcpy(acd_next_song, name);
} /* End of 'acd_set_next_song' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = acd_start;
	fl->m_end = acd_end;
	fl->m_get_stream = acd_get_stream;
	fl->m_seek = acd_seek;
	fl->m_get_audio_params = acd_get_audio_params;
	fl->m_init_obj_songs = acd_init_obj_songs;
	fl->m_flags = INP_OWN_SOUND;
	fl->m_pause = acd_pause;
	fl->m_resume = acd_resume;
	fl->m_get_cur_time = acd_get_cur_time;
	fl->m_get_info = acd_get_info;
	fl->m_save_info = acd_save_info;
	fl->m_set_song_title = acd_set_song_title;
	fl->m_set_next_song = acd_set_next_song;
	acd_pmng = fl->m_pmng;
	acd_print_msg = pmng_get_printer(acd_pmng);

	fl->m_num_spec_funcs = 2;
	fl->m_spec_funcs = (inp_spec_func_t *)malloc(sizeof(inp_spec_func_t) * 
			fl->m_num_spec_funcs);
	fl->m_spec_funcs[0].m_title = strdup(_("Reload info from CDDB"));
	fl->m_spec_funcs[0].m_flags = 0;
	fl->m_spec_funcs[0].m_func = cddb_reload;
	fl->m_spec_funcs[1].m_title = strdup(_("Submit info to CDDB"));
	fl->m_spec_funcs[1].m_flags = INP_SPEC_SAVE_INFO;
	fl->m_spec_funcs[1].m_func = cddb_submit;
} /* End of 'inp_get_func_list' function */

/* Print message */
void acd_print( char *format, ... )
{
	char msg[1024];
	va_list ap;

	va_start(ap, format);
	vsprintf(msg, format, ap);
	va_end(ap);
	
	if (acd_print_msg != NULL)
		acd_print_msg(msg);
} /* End of 'acd_print' function */

/* Set song title */
str_t *acd_set_song_title( char *filename )
{
	int track = acd_fname2trk(filename);
	str_t *title = str_new("");
	str_printf(title, "Track %02d", track + 1);
	return title;
} /* End of 'acd_set_song_title' function */

/* End of 'audiocd.c' file */

