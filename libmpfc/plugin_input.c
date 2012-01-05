/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Input plugin management functions implementation.
 * $Id$
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

#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gst/gst.h>
#include "types.h"
#include "inp.h"
#include "mystring.h"
#include "plugin.h"
#include "pmng.h"
#include "song.h"
#include "util.h"

/* Initialize input plugin */
plugin_t *inp_init( char *name, pmng_t *pmng )
{
	inp_data_t pd;
	plugin_t *p;

	/* Create plugin */
	memset(&pd, 0, sizeof(pd));
	p = plugin_init(pmng, name, PLUGIN_TYPE_INPUT, sizeof(in_plugin_t), 
			PLUGIN_DATA(&pd));
	if (p == NULL)
		return NULL;

	/* Set other fields */
	p->m_destructor = inp_free;
	INPUT_PLUGIN(p)->m_pd = pd;
	p->m_pd = PLUGIN_DATA(&INPUT_PLUGIN(p)->m_pd);
	return p;
} /* End of 'inp_init' function */

/* Input plugin destructor */
void inp_free( plugin_t *p )
{
	inp_data_t *pd = &INPUT_PLUGIN(p)->m_pd;

	if (pd->m_spec_funcs != NULL)
	{
		int i;

		for ( i = 0; i < pd->m_num_spec_funcs; i ++ )
			if (pd->m_spec_funcs[i].m_title != NULL)
				free(pd->m_spec_funcs[i].m_title);
		free(pd->m_spec_funcs);
	}
} /* End of 'inp_free' function */

/* Start playing function */
bool_t inp_start( in_plugin_t *p, char *filename, file_t *fd )
{
	if (p == NULL)
		return FALSE;
	if (p->m_pd.m_start_with_fd != NULL)
		return p->m_pd.m_start_with_fd(filename, fd);
	else if (p->m_pd.m_start != NULL)
		return p->m_pd.m_start(filename);
	return FALSE;
} /* End of 'inp_start' function */

/* End playing function */
void inp_end( in_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_end != NULL))
		p->m_pd.m_end();
} /* End of 'inp_end' function */

/* Get song information function */
song_info_t *inp_get_info( in_plugin_t *p, char *full_name, int *len )
{
	GstElement *pipe = NULL;
	song_info_t *si = NULL;

	(*len) = 0;

	/* Create decoding pipeline */
	pipe = gst_pipeline_new("pipeline");
	if (!pipe)
		goto finally;
	GstElement *dec = gst_element_factory_make("uridecodebin", NULL);
	if (!dec)
		goto finally;
	g_object_set(dec, "uri", full_name, NULL);
	if (!gst_bin_add(GST_BIN(pipe), dec))
	{
		gst_object_unref(dec);
		goto finally;
	}
	GstElement *sink = gst_element_factory_make("fakesink", NULL);
	if (!sink)
		goto finally;
	if (!gst_bin_add(GST_BIN(pipe), sink))
	{
		gst_object_unref(sink);
		goto finally;
	}
	gst_element_set_state(pipe, GST_STATE_PAUSED);

	si = si_new();

	/* Listen on the bus for tag messages */
	for ( ;; )
	{
		GstMessage *msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(pipe),
				GST_CLOCK_TIME_NONE,
				GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR);
		if (!msg)
			break;
		if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_TAG)
		{
			gst_message_unref(msg);
			break;
		}

		GstTagList *tags = NULL;
		gst_message_parse_tag(msg, &tags);
		if (tags)
		{
			gchar *val;
			if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &val))
			{
				si_set_name(si, val);
				g_free(val);
			}
			if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &val))
			{
				si_set_artist(si, val);
				g_free(val);
			}
			if (gst_tag_list_get_string(tags, GST_TAG_ALBUM, &val))
			{
				si_set_album(si, val);
				g_free(val);
			}
			if (gst_tag_list_get_string(tags, GST_TAG_COMMENT, &val))
			{
				si_set_comments(si, val);
				g_free(val);
			}
			if (gst_tag_list_get_string(tags, GST_TAG_GENRE, &val))
			{
				si_set_genre(si, val);
				g_free(val);
			}

			GDate *date;
			if (gst_tag_list_get_date(tags, GST_TAG_DATE, &date))
			{
				char year[100] = "";
				char *p = year;
				size_t sz = sizeof(year);

				GDateYear y = g_date_get_year(date);
				if (g_date_valid_year(y))
				{
					size_t len = snprintf(p, sz, "%d", y);

					GDateMonth m = g_date_get_month(date);
					if (g_date_valid_month(m))
					{
						p += len;
						sz -= len;
						len = snprintf(p, sz, "/%02d", m);

						GDateDay d = g_date_get_day(date);
						if (g_date_valid_day(d))
						{
							p += len;
							sz -= len;
							snprintf(p, sz, "/%02d", d);
						}
					}
				}
				si_set_year(si, year);
				g_date_free(date);
			}

			unsigned track;
			if (gst_tag_list_get_uint(tags, GST_TAG_TRACK_NUMBER, &track))
			{
				char trackstr[20];
				snprintf(trackstr, sizeof(trackstr), "%02d", track);
				si_set_track(si, trackstr);
			}

			gst_tag_list_free(tags);
		}

		gst_message_unref(msg);
	}

	/* Get song length */
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 gst_len;
	if (gst_element_query_duration(dec, &fmt, &gst_len))
		(*len) = gst_len / 1000000000;

	/* Convert charset*/
	/* TODO: gstreamer
	si_convert_cs(si, cfg_get_var(plugin_get_root_cfg(PLUGIN(p)),
				"charset-output"), plugin_get_pmng(PLUGIN(p)));
				*/

finally:
	if (pipe)
	{
		gst_element_set_state(pipe, GST_STATE_NULL);
		gst_object_unref(pipe);
	}
	return si;
} /* End of 'inp_get_info' function */
	
/* Save song information function */
bool_t inp_save_info( in_plugin_t *p, char *file_name, song_info_t *info )
{
	bool_t ret = FALSE;
	if (p != NULL && (p->m_pd.m_save_info != NULL) && info != NULL)
	{
		/* Convert charset */
		char *was_cs = (info->m_charset == NULL ? NULL : 
				strdup(info->m_charset));
		pmng_t *pmng = plugin_get_pmng(PLUGIN(p));
		
		si_convert_cs(info, cfg_get_var(pmng_get_cfg(pmng), 
					"charset-save-info"), pmng);
		ret = p->m_pd.m_save_info(file_name, info);
		si_convert_cs(info, was_cs, pmng);
		if (was_cs != NULL)
			free(was_cs);
	}
	return ret;
} /* End of 'inp_save_info' function */
	
/* Get supported file formats */
void inp_get_formats( in_plugin_t *p, char *extensions, char *content_type )
{
	if (p != NULL && (p->m_pd.m_get_formats != NULL))
		p->m_pd.m_get_formats(extensions, content_type);
	else
	{
		if (extensions != NULL)
			strcpy(extensions, "");
		if (content_type != NULL)
			strcpy(content_type, "");
	}
} /* End of 'inp_get_formats' function */
	
/* Get stream function */
int inp_get_stream( in_plugin_t *p, void *buf, int size )
{
	if (p != NULL && (p->m_pd.m_get_stream != NULL))
		return p->m_pd.m_get_stream(buf, size);
	return 0;
} /* End of 'inp_get_stream' function */

/* Seek song */
void inp_seek( in_plugin_t *p, int seconds )
{
	if (p != NULL && (p->m_pd.m_seek != NULL))
		p->m_pd.m_seek(seconds);
} /* End of 'inp_seek' function */

/* Get song audio parameters */
void inp_get_audio_params( in_plugin_t *p, int *channels, 
							int *frequency, dword *fmt, int *bitrate )
{
	if (p != NULL && (p->m_pd.m_get_audio_params != NULL))
		p->m_pd.m_get_audio_params(channels, frequency, fmt, bitrate);
	else 
	{
		*channels = 0;
		*frequency = 0;
		*fmt = 0;
		*bitrate = 0;
	}
} /* End of 'inp_get_audio_params' function */

/* Set equalizer parameters */
void inp_set_eq( in_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_set_eq != NULL))
		p->m_pd.m_set_eq();
} /* End of 'inp_set_eq' function */

/* Get plugin flags */
dword inp_get_flags( in_plugin_t *p )
{
	dword flags;

	flags = inp_get_plugin_flags(p);
	if (flags != 0)
		return flags;
	if (p != NULL)
		return p->m_pd.m_flags;
	return 0;
} /* End of 'inp_get_flags' function */

/* Pause playing */
void inp_pause( in_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_pause != NULL))
		p->m_pd.m_pause();
} /* End of 'inp_pause' function */

/* Resume playing */
void inp_resume( in_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_resume != NULL))
		p->m_pd.m_resume();
} /* End of 'inp_resume' function */

/* Get current time */
int inp_get_cur_time( in_plugin_t *p )
{
	if (p != NULL && (p->m_pd.m_get_cur_time != NULL))
		return p->m_pd.m_get_cur_time();
	return -1;
} /* End of 'inp_get_cur_time' function */

/* Get number of special functions */
int inp_get_num_specs( in_plugin_t *p )
{
	if (p != NULL)
		return p->m_pd.m_num_spec_funcs;
	return 0;
} /* End of 'inp_get_num_specs' function */

/* Get special function title */
char *inp_get_spec_title( in_plugin_t *p, int index )
{
	if (p != NULL && index >= 0 && index < p->m_pd.m_num_spec_funcs &&
			p->m_pd.m_spec_funcs != NULL)
		return p->m_pd.m_spec_funcs[index].m_title;
	return NULL;
} /* End of 'inp_get_spec_title' function */

/* Get special function flags */
dword inp_get_spec_flags( in_plugin_t *p, int index )
{
	if (p != NULL && index >= 0 && index < p->m_pd.m_num_spec_funcs &&
			p->m_pd.m_spec_funcs != NULL)
		return p->m_pd.m_spec_funcs[index].m_flags;
	return 0;
} /* End of 'inp_get_spec_flags' function */

/* Call special function */
void inp_spec_func( in_plugin_t *p, int index, char *filename )
{
	if (p != NULL && p->m_pd.m_spec_funcs != NULL && index >= 0 && 
			index < p->m_pd.m_num_spec_funcs && 
			p->m_pd.m_spec_funcs[index].m_func != NULL)
		p->m_pd.m_spec_funcs[index].m_func(filename);
} /* End of 'inp_spec_func' function */

/* Set song title */
str_t *inp_set_song_title( in_plugin_t *p, char *filename )
{
	if (filename == NULL)
		return NULL;
	
	if (p != NULL && p->m_pd.m_set_song_title != NULL)
		return p->m_pd.m_set_song_title(filename);
	else
		return str_new(util_short_name(filename));
} /* End of 'inp_set_song_title' function */

/* Set next song name */
void inp_set_next_song( in_plugin_t *p, char *name )
{
	if (p != NULL && p->m_pd.m_set_next_song != NULL)
		p->m_pd.m_set_next_song(name);
} /* End of 'inp_set_next_song' function */

/* Open directory */
void *inp_vfs_opendir( in_plugin_t *p, char *name )
{
	if (p != NULL && p->m_pd.m_vfs_opendir != NULL)
		return p->m_pd.m_vfs_opendir(name);
	else
		return NULL;
} /* End of 'inp_vfs_opendir' function */

/* Close directory */
void inp_vfs_closedir( in_plugin_t *p, void *dir )
{
	if (p != NULL && p->m_pd.m_vfs_closedir != NULL)
		p->m_pd.m_vfs_closedir(dir);
} /* End of 'inp_vfs_closedir' function */

/* Read directory entry */
char *inp_vfs_readdir( in_plugin_t *p, void *dir )
{
	if (p != NULL && p->m_pd.m_vfs_readdir != NULL)
		return p->m_pd.m_vfs_readdir(dir);
	else
		return NULL;
} /* End of 'inp_vfs_readdir' function */

/* Get file parameters */
int inp_vfs_stat( in_plugin_t *p, char *name, struct stat *sb )
{
	if (p != NULL && p->m_pd.m_vfs_stat != NULL)
		return p->m_pd.m_vfs_stat(name, sb);
	else
		return EACCES;
} /* End of 'inp_vfs_stat' function */

/* Get mixer type */
plugin_mixer_type_t inp_get_mixer_type( in_plugin_t *p )
{
	if (p != NULL && p->m_pd.m_get_mixer_type != NULL)
		return p->m_pd.m_get_mixer_type();
	else
		return PLUGIN_MIXER_DEFAULT;
} /* End of 'inp_get_mixer_type' function */

/* Get plugin flags */
dword inp_get_plugin_flags( in_plugin_t *p )
{
	if (p != NULL && p->m_pd.m_get_plugin_flags != NULL)
		return p->m_pd.m_get_plugin_flags();
	else
		return 0;
} /* End of 'inp_redirect' function */

/* End of 'inp.c' file */

