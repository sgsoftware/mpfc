/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.c
 * PURPOSE     : SG Konsamp. MP3 input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
 * NOTE        : Module prefix 'mp3'.
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

#include <smpeg/smpeg.h>
#include <SDL/SDL_audio.h>
#include <string.h>
#include "types.h"
#include "error.h"
#include "inp.h"
#include "song_info.h"

/* Currently playing song SMPEG object */
SMPEG *mp3_smpeg = NULL;

/* Current seek value */
int mp3_seek_val = 0;

/* Current song audio parameters */
int mp3_channels = 0, mp3_freq = 0, mp3_bits = 0;

/* Start play function */
bool mp3_start( char *filename )
{
	SMPEG_Info info;
	SDL_AudioSpec as;

	/* Initialize SMPEG object */
	mp3_smpeg = SMPEG_new(filename, &info, SDL_FALSE);
	if (mp3_smpeg == NULL || SMPEG_error(mp3_smpeg) != NULL)
	{
		if (mp3_smpeg != NULL)
		{
//			SMPEG_delete(mp3_smpeg);
			mp3_smpeg = NULL;
		}
		return FALSE;
	}
	SMPEG_wantedSpec(mp3_smpeg, &as);
	SMPEG_enablevideo(mp3_smpeg, SDL_FALSE);
	SMPEG_play(mp3_smpeg);

	/* Save song parameters */
	mp3_channels = as.channels;
	mp3_freq = as.freq;
	mp3_bits = (as.format == AUDIO_U8 || as.format == AUDIO_S8) ? 8 : 16;
	mp3_seek_val = 0;
	return TRUE;
} /* End of 'mp3_start' function */

/* End playing function */
void mp3_end( void )
{
	/* Destroy SMPEG object */
	if (mp3_smpeg != NULL)
	{
		SMPEG_delete(mp3_smpeg);
		mp3_smpeg = NULL;
	}
} /* End of 'mp3_end' function */

/* Get supported formats function */
void mp3_get_formats( char *buf )
{
	strcpy(buf, "mp3");
} /* End of 'mp3_get_formats' function */

/* Get song length */
int mp3_get_len( char *filename )
{
	SMPEG *smpeg;
	SMPEG_Info info;

	smpeg = SMPEG_new(filename, &info, SDL_FALSE);
	if (smpeg == NULL || SMPEG_error(smpeg) != NULL)
	{
/*		if (smpeg != NULL)
			SMPEG_delete(smpeg);*/
		return 0;
	}
	SMPEG_getinfo(smpeg, &info);
	SMPEG_delete(smpeg);
	return info.total_time;
} /* End of 'mp3_get_len' function */

/* Get song information */
bool mp3_get_info( char *filename, song_info_t *info )
{
	char id3_tag[128];
	FILE *fd;
	int i;

	/* Read ID3 tag */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return FALSE;
	fseek(fd, -128, SEEK_END);
	fread(id3_tag, sizeof(id3_tag), 1, fd);
	fclose(fd);

	/* Parse tag */
	if (id3_tag[0] != 'T' || id3_tag[1] != 'A' || id3_tag[2] != 'G')
		return FALSE;
	strncpy(info->m_name, &id3_tag[3], 30);
	for ( i = 29; i >= 0 && info->m_name[i] == ' '; i -- );
	info->m_name[i + 1] = 0;
	strncpy(info->m_artist, &id3_tag[33], 30);
	for ( i = 29; i >= 0 && info->m_artist[i] == ' '; i -- );
	info->m_artist[i + 1] = 0;
	strncpy(info->m_album, &id3_tag[63], 30);
	for ( i = 29; i >= 0 && info->m_album[i] == ' '; i -- );
	info->m_album[i + 1] = 0;
	strncpy(info->m_year, &id3_tag[93], 4);
	for ( i = 3; i >= 0 && info->m_year[i] == ' '; i -- );
	info->m_year[i + 1] = 0;
	
	return TRUE;
} /* End of 'mp3_get_info' function */

/* Get stream function */
int mp3_get_stream( void *buf, int size )
{
	if (mp3_smpeg != NULL)
	{
		if (mp3_seek_val)
		{
			SMPEG_skip(mp3_smpeg, mp3_seek_val);
			mp3_seek_val = 0;
		}
		
		memset(buf, 0, size);
		size = SMPEG_playAudio(mp3_smpeg, (Uint8 *)buf, size);
	}
	else
		size = 0;
	return size;
} /* End of 'mp3_get_stream' function */

/* Seek song */
void mp3_seek( int shift )
{
	if (mp3_smpeg != NULL)
	{
		mp3_seek_val += shift;
	}
} /* End of 'mp3_seek' function */

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, int *bits )
{
	*ch = mp3_channels;
	*freq = mp3_freq;
	*bits = mp3_bits;
} /* End of 'mp3_get_audio_params' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = mp3_start;
	fl->m_end = mp3_end;
	fl->m_get_stream = mp3_get_stream;
	fl->m_get_len = mp3_get_len;
	fl->m_get_info = mp3_get_info;
	fl->m_seek = mp3_seek;
	fl->m_get_audio_params = mp3_get_audio_params;
	fl->m_get_formats = mp3_get_formats;
} /* End of 'inp_get_func_list' function */

/* End of 'mp3.c' file */

