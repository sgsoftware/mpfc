/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : audiocd.h
 * PURPOSE     : SG MPFC AudioCD input plugin. Interface for main
 *               stuff.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.11.2003
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

#ifndef __SG_MPFC_AUDIOCD_H__
#define __SG_MPFC_AUDIOCD_H__

#include "types.h"
#include "song_info.h"

/* The maximal number of tracks */
#define ACD_MAX_TRACKS 100

/* Tracks information array */
extern struct acd_trk_info_t
{
	int m_start_min, m_start_sec, m_start_frm;
	int m_end_min, m_end_sec, m_end_frm;
	int m_len;
	int m_number;
	bool_t m_data;
} acd_tracks_info[ACD_MAX_TRACKS];
extern int acd_num_tracks;
extern int acd_cur_track;
extern bool_t acd_info_read;

/* Message printer */
extern void (*acd_print_msg)( char *msg );

/* Get track start frame offset */
#define acd_get_trk_offset(t) \
	((acd_tracks_info[t].m_start_min * 60 + \
				acd_tracks_info[t].m_start_sec) * 75 + \
				acd_tracks_info[t].m_start_frm)

/* Get disc length */
#define acd_get_disc_len() \
	(((acd_tracks_info[acd_num_tracks - 1].m_end_min * 60 + \
				acd_tracks_info[acd_num_tracks - 1].m_end_sec) * 75 + \
				acd_tracks_info[acd_num_tracks - 1].m_end_frm) / 75)

/* Get song info */
bool_t acd_get_info( char *filename, song_info_t *info );

/* Set song title */
void acd_set_song_title( char *buf, char *filename );

/* Print message */
void acd_print( char *format, ... );

#endif

/* End of 'audiocd.h' file */

