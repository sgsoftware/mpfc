/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : audiocd.c
 * PURPOSE     : SG MPFC. Audio CD input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.08.2003
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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include "types.h"
#include "cfg.h"
#include "inp.h"
#include "song.h"
#include "song_info.h"
#include "util.h"

/* The maximal number of tracks */
#define ACD_MAX_TRACKS 100

/* Get track number from filename */
#define acd_fname2trk(name) (atoi(&name[6]) - 1)

/* Calculate track length */
#define acd_get_trk_len(trk) \
	((acd_tracks_info[trk].m_end_min * 60 + acd_tracks_info[trk].m_end_sec) - \
	((acd_tracks_info[trk].m_start_min * 60 + \
	  acd_tracks_info[trk].m_start_sec)))

/* This is pointer to global variables list */
cfg_list_t *acd_var_list = NULL;

/* Tracks information array */
static struct acd_trk_info_t
{
	int m_start_min, m_start_sec, m_start_frm;
	int m_end_min, m_end_sec, m_end_frm;
	int m_len;
	int m_number;
	bool m_data;
} acd_tracks_info[ACD_MAX_TRACKS];
static int acd_num_tracks = 0;
static int acd_cur_track = -1;

/* Current time */
int acd_time = 0;

/* Prepare cdrom for ioctls */
int acd_prepare_cd( void )
{
	int fd;
	
	/* Open device */
	fd = open("/dev/cdrom", O_RDONLY|O_NONBLOCK);
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
bool acd_start( char *filename )
{
	int fd, track;
	struct cdrom_msf msf;

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

	/* Close device */
	close(fd);
	return TRUE;
} /* End of 'mp3_start' function */

/* End playing function */
void acd_end( void )
{
	int fd;

	/* Open device */
	acd_time = 0;
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Stop playing */
	ioctl(fd, CDROMSTOP, 0);

	/* Close device */
	close(fd);
} /* End of 'acd_end' function */

/* Get length */
int acd_get_len( char *filename )
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
	bool playing;

	/* Check if we are playing now */
	if ((fd = acd_prepare_cd()) < 0)
		return 0;
	info.cdsc_format = CDROM_MSF;
	ioctl(fd, CDROMSUBCHNL, &info);
	playing = (info.cdsc_audiostatus == CDROM_AUDIO_PLAY);
	acd_time = info.cdsc_reladdr.msf.minute * 60 + 
		info.cdsc_reladdr.msf.second;
	close(fd);
	return playing ? size : 0;
} /* End of 'acd_get_stream' function */

/* Initialize songs that respect to object */
song_t **acd_init_obj_songs( char *name, int *num_songs )
{
	song_t **s = NULL;
	int fd, i, j;
	struct cdrom_tochdr toc;
	struct cdrom_tocentry entry;

	/* Open cdrom device */
	(*num_songs) = 0;
	if ((fd = acd_prepare_cd()) < 0)
		return NULL;
	
	/* Read tracks information */
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
		if (!acd_tracks_info[i].m_data)
			(*num_songs) ++;
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

	/* Initialize songs */
	s = (song_t **)malloc((*num_songs) * sizeof(song_t *));
	for ( i = 0, j = 0; i < acd_num_tracks; i ++ )
	{
		song_t *song;
			
		if (acd_tracks_info[i].m_data)
			continue;
		s[j ++] = song = (song_t *)malloc(sizeof(song_t));
		sprintf(song->m_file_name, "Track %02i", acd_tracks_info[i].m_number);
		strcpy(song->m_title, song->m_file_name);
		song->m_len = acd_tracks_info[i].m_len;
		song->m_inp = NULL;
		song->m_info = NULL;
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
void acd_get_audio_params( int *ch, int *freq, dword *fmt )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
} /* End of 'acd_get_audio_params' function */

/* Get current time */
int acd_get_cur_time( void ) 
{
	return acd_time;
} /* End of 'acd_get_cur_time' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = acd_start;
	fl->m_end = acd_end;
	fl->m_get_stream = acd_get_stream;
	fl->m_get_len = acd_get_len;
	fl->m_seek = acd_seek;
	fl->m_get_audio_params = acd_get_audio_params;
	fl->m_init_obj_songs = acd_init_obj_songs;
	fl->m_flags = INP_NO_OUTP;
	fl->m_pause = acd_pause;
	fl->m_resume = acd_resume;
	fl->m_get_cur_time = acd_get_cur_time;
} /* End of 'inp_get_func_list' function */

/* Save variables list */
void inp_set_vars( cfg_list_t *list )
{
	acd_var_list = list;
} /* End of 'inp_set_vars' function */

/* End of 'audiocd.c' file */

