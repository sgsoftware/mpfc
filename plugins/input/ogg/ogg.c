/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : ogg.c
 * PURPOSE     : SG MPFC. Ogg Vorbis input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.10.2004
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
#include "file.h"
#include "genre_list.h"
#include "inp.h"
#include "pmng.h"
#include "song_info.h"
#include "util.h"
#include "vcedit.h"
#include "wnd.h"
#include "wnd_checkbox.h"
#include "wnd_dialog.h"

/* Ogg Vorbis file object */
static OggVorbis_File ogg_vf;

/* Audio parameters */
static int ogg_channels = 0, ogg_freq = 0, ogg_bitrate = 0;
static dword ogg_fmt = 0;

/* Genres list */
static genre_list_t *ogg_glist = NULL;

/* Plugins manager */
static pmng_t *ogg_pmng = NULL;

/* Currently playing song file name */
static char ogg_filename[MAX_FILE_NAME] = "";

/* Song info to save after play end */
static song_info_t *ogg_info = NULL;

/* Audio information of the current song */
static vorbis_info *ogg_vi = NULL;

/* Some declarations */
bool_t ogg_save_info( char *filename, song_info_t *info );

/* Callback functions for libvorbis */
static size_t ogg_file_read( void *ptr, size_t size, size_t nmemb, 
		void *datasource );
static int ogg_file_seek( void *datasource, ogg_int64_t offset, int whence );
static int ogg_file_close( void *datasource );
static long ogg_file_tell( void *datasource );
static ov_callbacks ogg_callbacks = { ogg_file_read, ogg_file_seek, 
	ogg_file_close, ogg_file_tell };

/* Mutex for synchronizing operations */
static pthread_mutex_t ogg_mutex;

/* Logger */
static logger_t *ogg_log;

/* Configuration list */
static cfg_node_t *ogg_cfg;

/* Plugin info */
static char *ogg_desc = "OggVorbis plugin";
static char *ogg_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Start playing with an opened file descriptor */
bool_t ogg_start_with_fd( char *filename, file_t *fd )
{
	vorbis_comment *comment;
	int i;

	/* Open file */
	if (fd == NULL)
	{
		fd = file_open(filename, "rb", ogg_log);
		if (fd == NULL)
			return FALSE;
	}
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
	ogg_info = NULL;
	file_set_min_buf_size(fd, ogg_vi->bitrate_nominal >> 3);
	util_strncpy(ogg_filename, filename, sizeof(ogg_filename));
	return TRUE;
} /* End of 'ogg_start_with_fd' function */

/* Start playing */
bool_t ogg_start( char *filename )
{
	return ogg_start_with_fd(filename, NULL);
} /* End of 'ogg_start' function */

/* End playing */
void ogg_end( void )
{
	char fname[MAX_FILE_NAME];
	
	ov_clear(&ogg_vf);

	/* Save info if need */
	util_strncpy(fname, ogg_filename, sizeof(fname));
	strcpy(ogg_filename, "");
	ogg_vi = NULL;
	if (ogg_info != NULL)
	{
		ogg_save_info(fname, ogg_info);
		si_free(ogg_info);
		ogg_info = NULL;
	}
	ogg_bitrate = 0;
	ogg_freq = 0;
	ogg_channels = 0;
	ogg_fmt = 0;

	/* Destroy mutex */
	pthread_mutex_destroy(&ogg_mutex);
} /* End of 'ogg_end' function */

/* Lock mutex */
static void ogg_lock( void )
{
	pthread_mutex_lock(&ogg_mutex);
} /* End of 'ogg_lock' function */

/* Unlock mutex */
static void ogg_unlock( void )
{
	pthread_mutex_unlock(&ogg_mutex);
} /* End of 'ogg_unlock' function */

/* Get supported formats */
void ogg_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "ogg");
	if (content_type != NULL)
		strcpy(content_type, "application/ogg");
} /* End of 'ogg_get_formats' function */

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
void ogg_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = ogg_channels;
	*freq = ogg_freq;
	*fmt = ogg_fmt;
	*bitrate = ogg_bitrate;
} /* End of 'ogg_get_audio_params' function */

/* Get current time */
int ogg_get_cur_time( void )
{
	return ov_time_tell(&ogg_vf);
} /* End of 'ogg_get_cur_time' function */

/* Get comments list from vorbis comment */
static char **ogg_get_comment_list( vorbis_comment *vc )
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
static char **ogg_add_tag( char **list, char *label, char *tag )
{
	char str[256];
	int len;
	int i;

	/* Search list for our tag */
	snprintf(str, sizeof(str), "%s=%s", label, tag);
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
static void ogg_add_list( vorbis_comment *vc, char **comments )
{
	while (*comments)
		vorbis_comment_add(vc, *comments++);
} /* End of 'ogg_add_list' function */

/* Save song information */
bool_t ogg_save_info( char *filename, song_info_t *info )
{
	char **comment_list;
	vcedit_state *state;
	vorbis_comment *comment;
	FILE *in, *out;
	int i, outfd;
	char tmpfn[MAX_FILE_NAME];
	
	/* Supported only for regular files */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
	{
		logger_error(ogg_log, 1, _("Only regular files are supported for"
					"writing info by ogg plugin"));
		return FALSE;
	}

	/* Convert to UTF-8 if need */
	if (cfg_get_var_int(ogg_cfg, "always-use-utf8") &&
			(info->m_charset == NULL || strcasecmp(info->m_charset, "utf-8")))
	{
		si_convert_cs(info, "utf-8", ogg_pmng);
	}
	
	/* Schedule info for saving if we are playing this file now */
	if (!strcmp(filename, ogg_filename))
	{
		if (ogg_info != NULL)
			si_free(ogg_info);
		ogg_info = si_dup(info);
		return TRUE;
	}

	/* Read current info at first */
	state = vcedit_new_state();
	in = fopen(filename, "rb");
	if (in == NULL)
	{
		vcedit_clear(state);
		logger_error(ogg_log, 1, _("Unable to open file %s"), filename);
		return FALSE;
	}
	if (vcedit_open(state, in) < 0)
	{
		fclose(in);
		vcedit_clear(state);
		return FALSE;
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
	comment_list = ogg_add_tag(comment_list, "genre", info->m_genre);
	//comment_list = ogg_add_tag(comment_list, "", info->m_comments);
	ogg_add_list(comment, comment_list);
	for ( i = 0; comment_list[i] != NULL; i ++ )
		free(comment_list[i]);
	free(comment_list);

	/* Save */
	snprintf(tmpfn, sizeof(tmpfn), "%s.XXXXXX", filename);
	if ((outfd = mkstemp(tmpfn)) < 0)
	{
		fclose(in);
		vcedit_clear(state);
		return FALSE;
	}
	if ((out = fdopen(outfd, "wb")) == NULL)
	{
		close(outfd);
		fclose(in);
		vcedit_clear(state);
		logger_error(ogg_log, 1, _("Unable to open file %s for writing"), 
				tmpfn);
		return FALSE;
	}
	vcedit_write(state, out);
	vcedit_clear(state);
	fclose(in);
	fclose(out);
	rename(tmpfn, filename);
	return TRUE;
} /* End of 'ogg_save_info' function */

/* Get song information */
song_info_t *ogg_get_info( char *filename, int *len )
{
	OggVorbis_File vf;
	file_t *fd;
	vorbis_comment *comment;
	vorbis_info *vi;
	song_info_t *si = NULL;
	char own_data[1024];

	/* For non-regular files return only audio parameters */
	*len = 0;
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
	{
		if (strcmp(filename, ogg_filename))
			return NULL;

		si = si_new();
		si->m_flags |= SI_ONLY_OWN;
		snprintf(own_data, sizeof(own_data),
				_("Nominal bitrate: %i kb/s\n"
				"Samplerate: %i Hz\n"
				"Channels: %i"),
				ogg_vi->bitrate_nominal / 1000, ogg_vi->rate, 
				ogg_vi->channels);
		si_set_own_data(si, own_data);
		return si;
	}

	/* Return current info if we have it */
	if (ogg_info != NULL && !strcmp(filename, ogg_filename))
	{
		return si_dup(ogg_info);
	}

	/* Open file */
	fd = file_open(filename, "rb", ogg_log);
	if (fd == NULL)
		return NULL;
	if (ov_open_callbacks(fd, &vf, NULL, 0, ogg_callbacks) < 0)
	{
		file_close(fd);
		return NULL;
	}

	*len = ov_time_total(&vf, -1);
	si = si_new();
	si->m_glist = ogg_glist;
	comment = ov_comment(&vf, -1);
	si_set_name(si, vorbis_comment_query(comment, "title", 0));
	si_set_artist(si, vorbis_comment_query(comment, "artist", 0));
	si_set_album(si, vorbis_comment_query(comment, "album", 0));
	si_set_track(si, vorbis_comment_query(comment, "tracknumber", 0));
	si_set_year(si, vorbis_comment_query(comment, "date", 0));
	si_set_genre(si, vorbis_comment_query(comment, "genre", 0));

	/* Set additional information */
	vi = ov_info(&vf, -1);
	if (vi != NULL)
	{
		snprintf(own_data, sizeof(own_data),
				_("Nominal bitrate: %i kb/s\n"
				"Samplerate: %i Hz\n"
				"Channels: %i\n"
				"Length: %i seconds\n"
				"File size: %i bytes"),
				vi->bitrate_nominal / 1000, vi->rate, vi->channels, 
				*len, util_get_file_size(filename));
		si_set_own_data(si, own_data);
	}

	/* Close file */
	ov_clear(&vf);
	return si;
} /* End of 'ogg_get_info' function */

/* Handle 'ok_clicked' message for configuration dialog */
wnd_msg_retcode_t ogg_on_configure( wnd_t *wnd )
{
	checkbox_t *cb = CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd),
				"always_use_utf8"));
	assert(cb);
	cfg_set_var_bool(ogg_cfg, "always-use-utf8", cb->m_checked);
	return WND_MSG_RETCODE_OK;
} /* End of 'ogg_on_configure' function */

/* Launch configuration dialog */
void ogg_configure( wnd_t *parent )
{
	dialog_t *dlg;

	dlg = dialog_new(parent, _("Configure OggVorbis plugin"));
	checkbox_new(WND_OBJ(dlg->m_vbox),
			_("Always use &UTF-8"), "always_use_utf8", 'u', 
			cfg_get_var_bool(ogg_cfg, "always-use-utf8"));
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", ogg_on_configure);
	dialog_arrange_children(dlg);
} /* End of 'ogg_configure' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = ogg_desc;
	pd->m_author = ogg_author;
	pd->m_configure = ogg_configure;
	INP_DATA(pd)->m_start = ogg_start;
	INP_DATA(pd)->m_end = ogg_end;
	INP_DATA(pd)->m_get_stream = ogg_get_stream;
	INP_DATA(pd)->m_seek = ogg_seek;
	INP_DATA(pd)->m_get_audio_params = ogg_get_audio_params;
	INP_DATA(pd)->m_get_cur_time = ogg_get_cur_time;
	INP_DATA(pd)->m_get_formats = ogg_get_formats;
	INP_DATA(pd)->m_save_info = ogg_save_info;
	INP_DATA(pd)->m_get_info = ogg_get_info;
	ogg_pmng = pd->m_pmng;
	ogg_log = pd->m_logger;
	ogg_cfg = pd->m_cfg;
} /* End of 'ogg_get_func_list' function */

/* Initialize genres list */
static void ogg_init_glist( void )
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
static int ogg_file_seek( void *datasource, ogg_int64_t offset, int whence )
{
	file_t *fd = (file_t *)datasource;

	if (fd->m_type == FILE_TYPE_HTTP)
		return -1;
	else
		return file_seek(fd, offset, whence);
} /* End of 'ogg_file_seek' function */
	
/* Helper reading function */
static size_t ogg_file_read( void *ptr, size_t size, size_t nmemb, 
		void *datasource )
{
	return file_read(ptr, size, nmemb, datasource);
} /* End of 'ogg_file_read' function */

/* Helper file closing function */
static int ogg_file_close( void *datasource )
{
	return file_close(datasource);
} /* End of 'ogg_file_close' function */

/* Helper file position telling function */
static long ogg_file_tell( void *datasource )
{
	return file_tell(datasource);
} /* End of 'ogg_file_tell' function */

/* End of 'ogg.c' file */

