/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : ogg.c
 * PURPOSE     : SG MPFC. Ogg Vorbis input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2004
 * NOTE        : Module prefix 'ogg'.
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
#include <string.h>
#include <sys/soundcard.h>
#include <pthread.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "types.h"
#include "cfg.h"
#include "file.h"
#include "genre_list.h"
#include "inp.h"
#include "song_info.h"
#include "util.h"
#include "vcedit.h"

/* Ogg Vorbis file object */
OggVorbis_File ogg_vf;

/* Audio parameters */
int ogg_channels = 0, ogg_freq = 0, ogg_bitrate = 0;
dword ogg_fmt = 0;

/* Genres list */
genre_list_t *ogg_glist = NULL;

/* Variables list */
cfg_list_t *ogg_var_list = NULL;

/* Currently playing song file name */
char ogg_filename[256];

/* Song info to save after play end */
song_info_t ogg_info;
bool_t ogg_need_save_info = FALSE;

/* Audio information of the current song */
vorbis_info *ogg_vi = NULL;

/* Some declarations */
void ogg_save_info( char *filename, song_info_t *info );

/* Message printer */
void (*ogg_print_msg)( char *msg );

/* Callback functions for libvorbis */
size_t ogg_file_read( void *ptr, size_t size, size_t nmemb, void *datasource );
int ogg_file_seek( void *datasource, ogg_int64_t offset, int whence );
int ogg_file_close( void *datasource );
long ogg_file_tell( void *datasource );
ov_callbacks ogg_callbacks = { ogg_file_read, ogg_file_seek, ogg_file_close, 
								ogg_file_tell };

/* Mutex for synchronizing operations */
pthread_mutex_t ogg_mutex;

/* Start playing */
bool_t ogg_start( char *filename )
{
	file_t *fd;
	vorbis_comment *comment;
	int i;

	/* Open file */
	fd = file_open(filename, "rb", ogg_print_msg);
	if (fd == NULL)
		return FALSE;
	if (ov_open_callbacks(fd, &ogg_vf, NULL, 0, ogg_callbacks) < 0)
	{
		file_close(fd);
		return FALSE;
	}

	/* Create mutex */
	pthread_mutex_init(&ogg_mutex, NULL);

	/* Get audio parameters */
	ogg_fmt = AFMT_S16_LE;
	ogg_vi = ov_info(&ogg_vf, -1);
	ogg_channels = ogg_vi->channels;
	ogg_freq = ogg_vi->rate;
	ogg_bitrate = 0;
	ogg_need_save_info = FALSE;
	strcpy(ogg_filename, filename);
	return TRUE;
} /* End of 'ogg_start' function */

/* End playing */
void ogg_end( void )
{
	char fname[256];
	
	ov_clear(&ogg_vf);

	/* Save info if need */
	strcpy(fname, ogg_filename);
	strcpy(ogg_filename, "");
	ogg_vi = NULL;
	if (ogg_need_save_info)
	{
		ogg_save_info(fname, &ogg_info);
		ogg_need_save_info = FALSE;
	}
	ogg_bitrate = 0;
	ogg_freq = 0;
	ogg_channels = 0;
	ogg_fmt = 0;

	/* Destroy mutex */
	pthread_mutex_destroy(&ogg_mutex);
} /* End of 'ogg_end' function */

/* Lock mutex */
void ogg_lock( void )
{
	pthread_mutex_lock(&ogg_mutex);
} /* End of 'ogg_lock' function */

/* Unlock mutex */
void ogg_unlock( void )
{
	pthread_mutex_unlock(&ogg_mutex);
} /* End of 'ogg_unlock' function */

/* Get supported formats */
void ogg_get_formats( char *buf )
{
	strcpy(buf, "ogg");
} /* End of 'ogg_get_formats' function */

/* Get song length */
int ogg_get_len( char *filename )
{
	OggVorbis_File vf;
	file_t *fd;
	int len;

	/* Supported only for regular files */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
		return 0;
	
	/* Create file object */
	fd = file_open(filename, "rb", ogg_print_msg);
	if (fd == NULL)
	{
		return 0;
	}
	if (ov_open_callbacks(fd, &vf, NULL, 0, ogg_callbacks) < 0)
	{
		file_close(fd);
		return 0;
	}

	/* Get length */
	len = (int)ov_time_total(&vf, -1);
	ov_clear(&vf);
	return len;
} /* End of 'ogg_get_len' function */

/* Get stream */
int ogg_get_stream( void *buf, int size ) 
{
	int current_section, ret;

	ogg_lock();
	ret = ov_read(&ogg_vf, buf, size, 0, 2, 1, &current_section);
	ogg_bitrate = ov_bitrate(&ogg_vf, current_section);
	ogg_freq = ogg_vi->rate;
	ogg_channels = ogg_vi->channels;
	if (!(ogg_vi->bitrate_upper == ogg_vi->bitrate_lower && 
				ogg_vi->bitrate_upper == ogg_vi->bitrate_nominal))
		ogg_bitrate = ov_bitrate_instant(&ogg_vf);
	ogg_unlock();
	return ret;
} /* End of 'ogg_get_stream' function */

/* Seek song */
void ogg_seek( int val )
{
	/* Supported only for regular files */
	if (file_get_type(ogg_filename) != FILE_TYPE_REGULAR)
		return;
	
	ogg_lock();
	ov_time_seek(&ogg_vf, val);
	ogg_unlock();
} /* End of 'ogg_seek' function */

/* Get audio parameters */
void ogg_get_audio_params( int *ch, int *freq, dword *fmt )
{
	*ch = ogg_channels;
	*freq = ogg_freq;
	*fmt = ogg_fmt;
} /* End of 'ogg_get_audio_params' function */

/* Get current bitrate */
int ogg_get_bitrate( void )
{
	return ogg_bitrate;
} /* End of 'ogg_get_bitrate' function */  
	
/* Get current time */
int ogg_get_cur_time( void )
{
	return ov_time_tell(&ogg_vf);
} /* End of 'ogg_get_cur_time' function */

/* Get comments list from vorbis comment */
char **ogg_get_comment_list( vorbis_comment *vc )
{
	int i;
	char **strv;

	strv = (char **)malloc(sizeof(char *) * (vc->comments + 1));
	for ( i = 0; i < vc->comments; i ++ )
		strv[i] = strdup(vc->user_comments[i]);
	strv[i] = NULL;
	return strv;
} /* End of 'ogg_get_comment_list' function */

/* Add tag to comments list */
char **ogg_add_tag( char **list, char *label, char *tag )
{
	char str[256];
	int len;
	int i;

	/* Search list for our tag */
	sprintf(str, "%s=%s", label, tag);
	len = strlen(label) + 1;
	for ( i = 0; list[i] != NULL; i ++ )
	{
		/* Found - modify it */
		if (!strncasecmp(str, list[i], len))
		{
			free(list[i]);
			list[i] = strdup(str);
			return list; 
		}
	}
	/* Not found - add tag */
	list = (char **)realloc(list, sizeof(char *) * (i + 2));
	list[i] = strdup(str);
	list[i + 1] = NULL;
	return list;
} /* End of 'ogg_add_tag' function */

/* Add comments list to vorbis comment */
void ogg_add_list( vorbis_comment *vc, char **comments )
{
	while (*comments)
		vorbis_comment_add(vc, *comments++);
} /* End of 'ogg_add_list' function */

/* Save song information */
void ogg_save_info( char *filename, song_info_t *info )
{
	char **comment_list;
	vcedit_state *state;
	vorbis_comment *comment;
	FILE *in, *out;
	int i, outfd;
	char tmpfn[256];
	
	/* Supported only for regular files */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
		return;
	
	/* Schedule info for saving if we are playing this file now */
	if (!strcmp(filename, ogg_filename))
	{
		memcpy(&ogg_info, info, sizeof(*info));
		ogg_need_save_info = TRUE;
		return;
	}

	/* Read current info at first */
	state = vcedit_new_state();
	in = fopen(filename, "rb");
	if (in == NULL)
	{
		vcedit_clear(state);
		return;
	}
	if (vcedit_open(state, in) < 0)
	{
		fclose(in);
		vcedit_clear(state);
		return;
	}
	comment = vcedit_comments(state);
	comment_list = ogg_get_comment_list(comment);
	vorbis_comment_clear(comment);

	/* Set our fields */
	comment_list = ogg_add_tag(comment_list, "title", info->m_name);
	comment_list = ogg_add_tag(comment_list, "artist", info->m_artist);
	comment_list = ogg_add_tag(comment_list, "album", info->m_album);
	comment_list = ogg_add_tag(comment_list, "tracknumber", info->m_track);
	comment_list = ogg_add_tag(comment_list, "date", info->m_year);
	comment_list = ogg_add_tag(comment_list, "genre", 
			(info->m_genre == GENRE_ID_OWN_STRING) ? 
			info->m_genre_data.m_text :
			((info->m_genre == GENRE_ID_UNKNOWN) ? "" :
			 ogg_glist->m_list[info->m_genre].m_name));
	//comment_list = ogg_add_tag(comment_list, "", info->m_comments);
	ogg_add_list(comment, comment_list);
	for ( i = 0; comment_list[i] != NULL; i ++ )
		free(comment_list[i]);
	free(comment_list);

	/* Save */
	sprintf(tmpfn, "%s.XXXXXX", filename);
	if ((outfd = mkstemp(tmpfn)) < 0)
	{
		fclose(in);
		vcedit_clear(state);
		return;
	}
	if ((out = fdopen(outfd, "wb")) == NULL)
	{
		close(outfd);
		fclose(in);
		vcedit_clear(state);
		return;
	}
	vcedit_write(state, out);
	vcedit_clear(state);
	fclose(in);
	fclose(out);
	rename(tmpfn, filename);
} /* End of 'ogg_save_info' function */

/* Get song information */
bool_t ogg_get_info( char *filename, song_info_t *info )
{
	OggVorbis_File vf;
	file_t *fd;
	char *str;
	vorbis_comment *comment;
	vorbis_info *vi;
	bool_t ret = FALSE;

	/* For non-regular files return only audio parameters */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
	{
		if (strcmp(filename, ogg_filename))
			return FALSE;

		info->m_genre = GENRE_ID_UNKNOWN;
		info->m_only_own = TRUE;
		sprintf(info->m_own_data, 
				_("Nominal bitrate: %i kb/s\n"
				"Samplerate: %i Hz\n"
				"Channels: %i"),
				ogg_vi->bitrate_nominal / 1000, ogg_vi->rate, 
				ogg_vi->channels);
		return TRUE;
	}

	/* Return current info if we have it */
	if (ogg_need_save_info && !strcmp(filename, ogg_filename))
	{
		memcpy(info, &ogg_info, sizeof(ogg_info));
		return TRUE;
	}

	/* Open file */
	fd = file_open(filename, "rb", ogg_print_msg);
	if (fd == NULL)
		return FALSE;
	if (ov_open_callbacks(fd, &vf, NULL, 0, ogg_callbacks) < 0)
	{
		file_close(fd);
		return FALSE;
	}

	memset(info, 0, sizeof(*info));
	comment = ov_comment(&vf, -1);
	str = vorbis_comment_query(comment, "title", 0);
	strcpy(info->m_name, str == NULL ? "" : (ret = TRUE, str));
	str = vorbis_comment_query(comment, "artist", 0);
	strcpy(info->m_artist, str == NULL ? "" : (ret = TRUE, str));
	str = vorbis_comment_query(comment, "album", 0);
	strcpy(info->m_album, str == NULL ? "" : (ret = TRUE, str));
	str = vorbis_comment_query(comment, "tracknumber", 0);
	strcpy(info->m_track, str == NULL ? "" : (ret = TRUE, str));
	str = vorbis_comment_query(comment, "date", 0);
	strcpy(info->m_year, str == NULL ? "" : (ret = TRUE, str));
	str = vorbis_comment_query(comment, "genre", 0);
	info->m_genre = glist_get_id_by_text(ogg_glist, 
			str == NULL ? "" : (ret = TRUE, str));
	if (info->m_genre == GENRE_ID_UNKNOWN)
	{
		info->m_genre = GENRE_ID_OWN_STRING;
		strcpy(info->m_genre_data.m_text, str == NULL ? "" : str);
	}

	/* Set additional information */
	vi = ov_info(&vf, -1);
	if (vi != NULL)
	{
		sprintf(info->m_own_data, 
				_("Nominal bitrate: %i kb/s\n"
				"Samplerate: %i Hz\n"
				"Channels: %i\n"
				"Length: %i seconds\n"
				"File size: %i bytes"),
				vi->bitrate_nominal / 1000, vi->rate, vi->channels, 
				(int)ov_time_total(&vf, -1), util_get_file_size(filename));
	}
	info->m_not_own_present = ret;

	/* Close file */
	ov_clear(&vf);
	return TRUE;
	return FALSE;
} /* End of 'ogg_get_info' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = ogg_start;
	fl->m_end = ogg_end;
	fl->m_get_len = ogg_get_len;
	fl->m_get_stream = ogg_get_stream;
	fl->m_seek = ogg_seek;
	fl->m_get_audio_params = ogg_get_audio_params;
	fl->m_get_bitrate = ogg_get_bitrate;
	fl->m_get_cur_time = ogg_get_cur_time;
	fl->m_get_formats = ogg_get_formats;
	fl->m_save_info = ogg_save_info;
	fl->m_get_info = ogg_get_info;
	fl->m_glist = ogg_glist;
	ogg_print_msg = fl->m_print_msg;
} /* End of 'ogg_get_func_list' function */

/* Save variables list */
void inp_save_vars( cfg_list_t *l )
{
	ogg_var_list = l;
} /* End of 'inp_save_vars' function */

/* Initialize genres list */
void ogg_init_glist( void )
{
	genre_list_t *l;

	/* Create list */
	if ((ogg_glist = glist_new()) == NULL)
		return;

	/* Fill it */
	l = ogg_glist;
	glist_add(l, "A Capella", 0);
	glist_add(l, "Acid", 0);
	glist_add(l, "Acid Jazz", 0);
	glist_add(l, "Acid Punk", 0);
	glist_add(l, "Acoustic", 0);
	glist_add(l, "Alt", 0);
	glist_add(l, "Alternative", 0);
	glist_add(l, "Ambient", 0);
	glist_add(l, "Anime", 0);
	glist_add(l, "Avantgarde", 0);
	glist_add(l, "Ballad", 0);
	glist_add(l, "Bass", 0);
	glist_add(l, "Beat", 0);
	glist_add(l, "Bebob", 0);
	glist_add(l, "Big Band", 0);
	glist_add(l, "Black Metal", 0);
	glist_add(l, "Bluegrass", 0);
	glist_add(l, "Blues", 0);
	glist_add(l, "Booty Bass", 0);
	glist_add(l, "BritPop", 0);
	glist_add(l, "Cabaret", 0);
	glist_add(l, "Celtic", 0);
	glist_add(l, "Chamber Music", 0);
	glist_add(l, "Chanson", 0);
	glist_add(l, "Chorus", 0);
	glist_add(l, "Christian Gangsta Rap", 0);
	glist_add(l, "Christian Rap", 0);
	glist_add(l, "Christian Rock", 0);
	glist_add(l, "Classical", 0);
	glist_add(l, "Classic Rock", 0);
	glist_add(l, "Club", 0);
	glist_add(l, "Club-House", 0);
	glist_add(l, "Comedy", 0);
	glist_add(l, "Contemporary Christian", 0);
	glist_add(l, "Country", 0);
	glist_add(l, "Crossover", 0);
	glist_add(l, "Cult", 0);
	glist_add(l, "Dance", 0);
	glist_add(l, "Dance Hall", 0);
	glist_add(l, "Darkwave", 0);
	glist_add(l, "Death Metal", 0);
	glist_add(l, "Disco", 0);
	glist_add(l, "Dream", 0);
	glist_add(l, "Drum & Bass", 0);
	glist_add(l, "Drum Solo", 0);
	glist_add(l, "Duet", 0);
	glist_add(l, "Easy Listening", 0);
	glist_add(l, "Electronic", 0);
	glist_add(l, "Ethnic", 0);
	glist_add(l, "Eurodance", 0);
	glist_add(l, "Euro-House", 0);
	glist_add(l, "Euro-Techno", 0);
	glist_add(l, "Fast-Fusion", 0);
	glist_add(l, "Folk", 0);
	glist_add(l, "Folklore", 0);
	glist_add(l, "Folk/Rock", 0);
	glist_add(l, "Freestyle", 0);
	glist_add(l, "Funk", 0);
	glist_add(l, "Fusion", 0);
	glist_add(l, "Game", 0);
	glist_add(l, "Gangsta Rap", 0);
	glist_add(l, "Goa", 0);
	glist_add(l, "Gospel", 0);
	glist_add(l, "Gothic", 0);
	glist_add(l, "Gothic Rock", 0);
	glist_add(l, "Grunge", 0);
	glist_add(l, "Hardcore", 0);
	glist_add(l, "Hard Rock", 0);
	glist_add(l, "Heavy Metal", 0);
	glist_add(l, "House", 0);
	glist_add(l, "Humour", 0);
	glist_add(l, "Indie", 0);
	glist_add(l, "Industrial", 0);
	glist_add(l, "Instrumental", 0);
	glist_add(l, "Instrumental Pop", 0);
	glist_add(l, "Jazz+Funk", 0);
	glist_add(l, "JPop", 0);
	glist_add(l, "Jungle", 0);
	glist_add(l, "Latin", 0);
	glist_add(l, "Lo-Fi", 0);
	glist_add(l, "Meditative", 0);
	glist_add(l, "Merengue", 0);
	glist_add(l, "Metal", 0);
	glist_add(l, "Musical", 0);
	glist_add(l, "National Folk", 0);
	glist_add(l, "Native American", 0);
	glist_add(l, "Negerpunk", 0);
	glist_add(l, "New Age", 0);
	glist_add(l, "New Wave", 0);
	glist_add(l, "Noise", 0);
	glist_add(l, "Oldies", 0);
	glist_add(l, "Opera", 0);
	glist_add(l, "Other", 0);
	glist_add(l, "Polka", 0);
	glist_add(l, "Polsk Punk", 0);
	glist_add(l, "Pop", 0);
	glist_add(l, "Pop-Folk", 0);
	glist_add(l, "Pop/Funk", 0);
	glist_add(l, "Porn Groove", 0);
	glist_add(l, "Power Ballad", 0);
	glist_add(l, "Pranks", 0);
	glist_add(l, "Primus", 0);
	glist_add(l, "Progressive Rock", 0);
	glist_add(l, "Psychedelic", 0);
	glist_add(l, "Psychedelic Rock", 0);
	glist_add(l, "Punk", 0);
	glist_add(l, "Punk Rock", 0);
	glist_add(l, "Rap", 0);
	glist_add(l, "Rave", 0);
	glist_add(l, "R&B", 0);
	glist_add(l, "Reggae", 0);
	glist_add(l, "Retro", 0);
	glist_add(l, "Revival", 0);
	glist_add(l, "Rhythmic Soul", 0);
	glist_add(l, "Rock", 0);
	glist_add(l, "Rock & Roll", 0);
	glist_add(l, "Salsa", 0);
	glist_add(l, "Samba", 0);
	glist_add(l, "Satire", 0);
	glist_add(l, "Showtunes", 0);
	glist_add(l, "Ska", 0);
	glist_add(l, "Slow Jam", 0);
	glist_add(l, "Slow Rock", 0);
	glist_add(l, "Sonata", 0);
	glist_add(l, "Soul", 0);
	glist_add(l, "Sound Clip", 0);
	glist_add(l, "Soundtrack", 0);
	glist_add(l, "Southern Rock", 0);
	glist_add(l, "Space", 0);
	glist_add(l, "Speech", 0);
	glist_add(l, "Swing", 0);
	glist_add(l, "Symphonic Rock", 0);
	glist_add(l, "Symphony", 0);
	glist_add(l, "Synthpop", 0);
	glist_add(l, "Tango", 0);
	glist_add(l, "Techno", 0);
	glist_add(l, "Techno-Industrial", 0);
	glist_add(l, "Terror", 0);
	glist_add(l, "Thrash Metal", 0);
	glist_add(l, "Top 40", 0);
	glist_add(l, "Trailer", 0);
	glist_add(l, "Trance", 0);
	glist_add(l, "Tribal", 0);
	glist_add(l, "Trip-Hop", 0);
	glist_add(l, "Vocal", 0);
} /* End of 'ogg_init_glist' function */
	
/* This function is called when initializing module */
void _init( void )
{
	/* Initialize genres list */
	ogg_init_glist();
} /* End of '_init' function */

/* This function is called when uninitializing module */
void _fini( void )
{
	glist_free(ogg_glist);
} /* End of '_fini' function */

/* Helper seeking function */
int ogg_file_seek( void *datasource, ogg_int64_t offset, int whence )
{
	file_t *fd = (file_t *)datasource;

	if (fd->m_type == FILE_TYPE_HTTP)
		return -1;
	else
		return file_seek(fd, offset, whence);
} /* End of 'ogg_file_seek' function */
	
/* Helper reading function */
size_t ogg_file_read( void *ptr, size_t size, size_t nmemb, void *datasource )
{
	return file_read(ptr, size, nmemb, datasource);
} /* End of 'ogg_file_read' function */

/* Helper file closing function */
int ogg_file_close( void *datasource )
{
	return file_close(datasource);
} /* End of 'ogg_file_close' function */

/* Helper file position telling function */
long ogg_file_tell( void *datasource )
{
	return file_tell(datasource);
} /* End of 'ogg_file_tell' function */

/* End of 'ogg.c' file */

