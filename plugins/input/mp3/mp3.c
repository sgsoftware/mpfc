/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.c
 * PURPOSE     : SG Konsamp. MP3 input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 23.04.2003
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
#include "genre_list.h"
#include "inp.h"
#include "mp3.h"
#include "song_info.h"

/* Currently playing song SMPEG object */
SMPEG *mp3_smpeg = NULL;

/* Current seek value */
int mp3_seek_val = 0;

/* Current song audio parameters */
int mp3_channels = 0, mp3_freq = 0, mp3_bits = 0;

/* Current file name */
char mp3_file_name[128] = "";

/* ID3 tag scheduled for saving */
char mp3_tag[128];

/* Flag of whether we must save tag */
bool mp3_need_save = FALSE;

/* Genres list */
genre_list_t *mp3_glist = NULL;

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
	strcpy(mp3_file_name, filename);
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

		/* Save tag */
		if (mp3_need_save)
		{
			mp3_save_tag(mp3_file_name, mp3_tag);
			mp3_need_save = FALSE;
		}
		
		strcpy(mp3_file_name, "");
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

/* Save ID3 tag */
void mp3_save_tag( char *filename, char *tag )
{
	FILE *fd;
	char buf[10];
	bool has_tag = TRUE;
	
	/* Open file */
	fd = fopen(filename, "a+b");
	if (fd == NULL)
		return;

	/* Check if we have ID3 tag in this file */
	fseek(fd, -128, SEEK_END);
	fread(buf, 1, 3, fd);
	if (buf[0] != 'T' || buf[1] != 'A' || buf[2] != 'G')
		has_tag = FALSE;

	/* Save tag */
	if (has_tag)
		fseek(fd, -128, SEEK_END);
	else
		fseek(fd, 0, SEEK_END);
	fwrite(tag, 1, 128, fd);

	/* Close file */
	fclose(fd);
} /* End of 'mp3_save_tag' function */

/* Save song information */
void mp3_save_info( char *filename, song_info_t *info )
{
	char id3_tag[128];

	/* Create ID3 tag */
	strncpy(&id3_tag[0], "TAG", 3);
	strncpy(&id3_tag[3], info->m_name, 30);
	strncpy(&id3_tag[33], info->m_artist, 30);
	strncpy(&id3_tag[63], info->m_album, 30);
	strncpy(&id3_tag[93], info->m_year, 4);
	strncpy(&id3_tag[97], "", 30);
	id3_tag[127] = (info->m_genre == GENRE_ID_UNKNOWN) ?
		info->m_genre_data : mp3_glist->m_list[info->m_genre].m_data;

	/* Save tag or save it later */
	if (!strcmp(filename, mp3_file_name))
	{
		mp3_need_save = TRUE;
		memcpy(mp3_tag, id3_tag, 128);
	}
	else
		mp3_save_tag(filename, id3_tag);
} /* End of 'mp3_save_info' function */

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
	strncpy(info->m_comments, &id3_tag[97], 30);
	for ( i = 3; i >= 0 && info->m_comments[i] == ' '; i -- );
	info->m_comments[i + 1] = 0;
	info->m_genre = glist_get_id(mp3_glist, id3_tag[127]);
	info->m_genre_data = id3_tag[127];
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

/* Initialize genres list */
void mp3_init_glist( void )
{
	genre_list_t *l;

	/* Create list */
	if ((mp3_glist = glist_new()) == NULL)
		return;

	/* Fill it */
	l = mp3_glist;
	glist_add(l, "A Capella", 0x7B);
	glist_add(l, "Acid", 0x22);
	glist_add(l, "Acid Jazz", 0x4A);
	glist_add(l, "Acid Punk", 0x49);
	glist_add(l, "Acoustic", 0x63);
	glist_add(l, "Alt. Rock", 0x28);
	glist_add(l, "Alternative", 0x14);
	glist_add(l, "Ambient", 0x1A);
	glist_add(l, "Anime", 0x91);
	glist_add(l, "Avantgarde", 0x5A);
	glist_add(l, "Ballad", 0x74);
	glist_add(l, "Bass", 0x29);
	glist_add(l, "Beat", 0x87);
	glist_add(l, "Bebop", 0x55);
	glist_add(l, "Big Band", 0x60);
	glist_add(l, "Black Metal", 0x8A);
	glist_add(l, "Bluegrass", 0x59);
	glist_add(l, "Blues", 0x00);
	glist_add(l, "Booty Bass", 0x6B);
	glist_add(l, "BritPop", 0x84);
	glist_add(l, "Cabaret", 0x41);
	glist_add(l, "Celtic", 0x58);
	glist_add(l, "Chamber Music", 0x68);
	glist_add(l, "Christian Gangsta Rap", 0x88);
	glist_add(l, "Christian Rap", 0x3D);
	glist_add(l, "Christian Rock", 0x8D);
	glist_add(l, "Classic Rock", 0x01);
	glist_add(l, "Classical", 0x20);
	glist_add(l, "Club", 0x70);
	glist_add(l, "Club-House", 0x80);
	glist_add(l, "Comedy", 0x39);
	glist_add(l, "Contemporary Christian", 0x8C);
	glist_add(l, "Country", 0x02);
	glist_add(l, "Crossover", 0x8B);
	glist_add(l, "Cult", 0x3A);
	glist_add(l, "Dance", 0x03);
	glist_add(l, "Dance Hall", 0x7D);
	glist_add(l, "Darkwave", 0x32);
	glist_add(l, "Death Metal", 0x16);
	glist_add(l, "Disco", 0x04);
	glist_add(l, "Dream", 0x37);
	glist_add(l, "Drum & Bass", 0x7F);
	glist_add(l, "Drum Solo", 0x7A);
	glist_add(l, "Duet", 0x78);
	glist_add(l, "Easy Listening", 0x62);
	glist_add(l, "Electronic", 0x34);
	glist_add(l, "Ethnic", 0x30);
	glist_add(l, "Eurodance", 0x36);
	glist_add(l, "Euro-House", 0x7C);
	glist_add(l, "Euro-Techno", 0x19);
	glist_add(l, "Fast-Fusion", 0x54);
	glist_add(l, "Folk", 0x50);
	glist_add(l, "Folk/Rock", 0x51);
	glist_add(l, "Folklore", 0x73);
	glist_add(l, "Freestyle", 0x77);
	glist_add(l, "Funk", 0x05);
	glist_add(l, "Fusion", 0x1E);
	glist_add(l, "Game", 0x24);
	glist_add(l, "Gangsta Rap", 0x3B);
	glist_add(l, "Goa", 0x7E);
	glist_add(l, "Gospel", 0x26);
	glist_add(l, "Gothic", 0x31);
	glist_add(l, "Gothic Rock", 0x5B);
	glist_add(l, "Grunge", 0x06);
	glist_add(l, "Hard Rock", 0x4F);
	glist_add(l, "Hardcore", 0x81);
	glist_add(l, "Heavy Metal", 0x89);
	glist_add(l, "Hip-Hop", 0x07);
	glist_add(l, "House", 0x23);
	glist_add(l, "Humour", 0x64);
	glist_add(l, "Indie", 0x83);
	glist_add(l, "Industrial", 0x13);
	glist_add(l, "Instrumental", 0x21);
	glist_add(l, "Instrumental Pop", 0x2E);
	glist_add(l, "Instrumental Rock", 0x2F);
	glist_add(l, "Jazz", 0x08);
	glist_add(l, "Jazz+Funk", 0x1D);
	glist_add(l, "JPop", 0x92);
	glist_add(l, "Jungle", 0x3F);
	glist_add(l, "Latin", 0x56);
	glist_add(l, "Lo-Fi", 0x47);
	glist_add(l, "Meditative", 0x2D);
	glist_add(l, "Merengue", 0x8E);
	glist_add(l, "Metal", 0x09);
	glist_add(l, "Musical", 0x4D);
	glist_add(l, "National Folk", 0x52);
	glist_add(l, "Native American", 0x40);
	glist_add(l, "Negerpunk", 0x85);
	glist_add(l, "New Age", 0x0A);
	glist_add(l, "New Wave", 0x42);
	glist_add(l, "Noise", 0x27);
	glist_add(l, "Oldies", 0x0B);
	glist_add(l, "Opera", 0x67);
	glist_add(l, "Other", 0x0C);
	glist_add(l, "Polka", 0x4B);
	glist_add(l, "Polsk Punk", 0x86);
	glist_add(l, "Pop", 0x0D);
	glist_add(l, "Pop/Funk", 0x3E);
	glist_add(l, "Pop-Folk", 0x35);
	glist_add(l, "Porn Groove", 0x6D);
	glist_add(l, "Power Ballad", 0x75);
	glist_add(l, "Prank", 0x17);
	glist_add(l, "Primus", 0x6C);
	glist_add(l, "Progressive Rock", 0x5C);
	glist_add(l, "Psychedelic", 0x43);
	glist_add(l, "Psychedelic Rock", 0x5D);
	glist_add(l, "Punk", 0x2B);
	glist_add(l, "Punk Rock", 0x79);
	glist_add(l, "R&B", 0x0E);
	glist_add(l, "Rap", 0x0F);
	glist_add(l, "Rave", 0x44);
	glist_add(l, "Reggae", 0x10);
	glist_add(l, "Retro", 0x4C);
	glist_add(l, "Revival", 0x57);
	glist_add(l, "Rythmic Soul", 0x76);
	glist_add(l, "Rock", 0x11);
	glist_add(l, "Rock & Roll", 0x4E);
	glist_add(l, "Salsa", 0x8F);
	glist_add(l, "Samba", 0x72);
	glist_add(l, "Satire", 0x6E);
	glist_add(l, "Showtunes", 0x45);
	glist_add(l, "Ska", 0x15);
	glist_add(l, "Slow Jam", 0x6F);
	glist_add(l, "Slow Rock", 0x5F);
	glist_add(l, "Sonata", 0x69);
	glist_add(l, "Soul", 0x2A);
	glist_add(l, "Sound Clip", 0x25);
	glist_add(l, "Soundtrack", 0x18);
	glist_add(l, "Southern Rock", 0x38);
	glist_add(l, "Space", 0x2C);
	glist_add(l, "Speech", 0x65);
	glist_add(l, "Swing", 0x53);
	glist_add(l, "Symphonic Rock", 0x5E);
	glist_add(l, "Symphony", 0x6A);
	glist_add(l, "Synthpop", 0x93);
	glist_add(l, "Tango", 0x71);
	glist_add(l, "Techno", 0x12);
	glist_add(l, "Techno-Industrial", 0x33);
	glist_add(l, "Terror", 0x82);
	glist_add(l, "Thrash Metal", 0x90);
	glist_add(l, "Top 40", 0x3C);
	glist_add(l, "Trailer", 0x46);
	glist_add(l, "Trance", 0x1F);
	glist_add(l, "Tribal", 0x48);
	glist_add(l, "Trip-Hop", 0x1B);
	glist_add(l, "Vocal", 0x1C);
} /* End of 'mp3_init_glist' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = mp3_start;
	fl->m_end = mp3_end;
	fl->m_get_stream = mp3_get_stream;
	fl->m_get_len = mp3_get_len;
	fl->m_get_info = mp3_get_info;
	fl->m_save_info = mp3_save_info;
	fl->m_seek = mp3_seek;
	fl->m_get_audio_params = mp3_get_audio_params;
	fl->m_get_formats = mp3_get_formats;
	fl->m_glist = mp3_glist;
} /* End of 'inp_get_func_list' function */

/* This function is called when initializing module */
void _init( void )
{
	/* Initialize genres list */
	mp3_init_glist();
} /* End of '_init' function */

/* This function is called when uninitializing module */
void _fini( void )
{
	glist_free(mp3_glist);
} /* End of '_fini' function */

/* End of 'mp3.c' file */

