/******************************************************************
 * Copyright (C) 2003 - 2013 by SG Software.
 *
 * SG MPFC. Load and save song info.
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

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <tag_c.h>
#include "metadata_io.h"
#include "util.h"
	
/* Get song information using gstreamer */
static song_info_t *md_get_info_gst( const char *full_name, int *len )
{
	GstElement *pipe = NULL;
	song_info_t *si = NULL;

	/* Create decoding pipeline */
	pipe = gst_element_factory_make("playbin", "play");
	if (!pipe)
		goto finally;
	GstElement *sink = gst_element_factory_make("fakesink", NULL);
	if (!sink)
		goto finally;
	g_object_set(G_OBJECT(pipe), "audio-sink", sink, NULL);
	g_object_set(G_OBJECT(pipe), "uri", full_name, NULL);
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

			GstDateTime *dt;
			if (gst_tag_list_get_date_time(tags, GST_TAG_DATE_TIME, &dt))
			{
				char year[100] = "";
				char *p = year;
				size_t sz = sizeof(year);

				if (gst_date_time_has_year(dt))
				{
					gint y = gst_date_time_get_year(dt);
					size_t len = snprintf(p, sz, "%d", y);

					if (gst_date_time_has_month(dt))
					{
						gint m = gst_date_time_get_month(dt);
						p += len;
						sz -= len;
						len = snprintf(p, sz, "/%02d", m);

						if (gst_date_time_has_day(dt))
						{
							GDateDay d = gst_date_time_get_day(dt);
							p += len;
							sz -= len;
							snprintf(p, sz, "/%02d", d);
						}
					}
				}
				si_set_year(si, year);

				gst_date_time_unref(dt);
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

	for ( int i = 0; i < 5; i++ )
	{
		/* Get song length */
		gint64 gst_len;
		if (gst_element_query_duration(pipe, GST_FORMAT_TIME, &gst_len))
		{
			(*len) = gst_len / 1000000000;
			break;
		}

		/* Try again */
		util_wait();
	}

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
} /* End of 'md_get_info_gst' function */
	
/* Get song information using taglib */
static song_info_t *md_get_info_taglib( const char *file_name, int *len )
{
	TagLib_File *file = taglib_file_new(file_name);
	if (!file)
		return NULL;
	if (!taglib_file_is_valid(file))
	{
		taglib_file_free(file);
		return NULL;
	}

	TagLib_Tag *tag = taglib_file_tag(file);

	song_info_t *si = si_new();
	si_set_name(si, taglib_tag_title(tag));
	si_set_artist(si, taglib_tag_artist(tag));
	si_set_album(si, taglib_tag_album(tag));
	si_set_comments(si, taglib_tag_comment(tag));
	si_set_genre(si, taglib_tag_genre(tag));

	unsigned year = taglib_tag_year(tag);
	if (year > 0)
	{
		char y[32];
		snprintf(y, sizeof(y), "%d", year);
		si_set_year(si, y);
	}

	unsigned track = taglib_tag_track(tag);
	if (track > 0)
	{
		char t[32];
		snprintf(t, sizeof(t), "%d", track);
		si_set_track(si, t);
	}

	(*len) = taglib_audioproperties_length(taglib_file_audioproperties(file));

	taglib_tag_free_strings();
	taglib_file_free(file);
	return si;
} /* End of 'md_get_info_taglib' function */
	
/* Get song information function */
song_info_t *md_get_info( const char *file_name, const char *full_uri, int *len )
{
	(*len) = 0;
	
	if (file_name)
	{
		song_info_t *si = md_get_info_taglib(file_name, len);
		if (si)
			return si;
	}

	if (full_uri)
		return md_get_info_gst(full_uri, len);
	return NULL;
} /* End of 'md_get_info' function */

/* Save song information function */
bool_t md_save_info( const char *file_name, song_info_t *info )
{
	TagLib_File *file = taglib_file_new(file_name);
	if (!file)
		return FALSE;

	TagLib_Tag *tag = taglib_file_tag(file);

	taglib_tag_set_title(tag, info->m_name);
	taglib_tag_set_artist(tag, info->m_artist);
	taglib_tag_set_album(tag, info->m_album);
	taglib_tag_set_comment(tag, info->m_comments);
	taglib_tag_set_genre(tag, info->m_genre);

	long year = strtol(info->m_year, NULL, 10);
	if (year >= 0 && year < LONG_MAX)
		taglib_tag_set_year(tag, year);

	long track = strtol(info->m_track, NULL, 10);
	if (track >= 0 && track < LONG_MAX)
		taglib_tag_set_track(tag, track);

	bool_t saved = taglib_file_save(file);
	taglib_file_free(file);
	return saved;

	/* Convert charset */
	/* TODO: gstreamer/taglib
	char *was_cs = (info->m_charset == NULL ? NULL : 
			strdup(info->m_charset));
	pmng_t *pmng = plugin_get_pmng(PLUGIN(p));
	
	si_convert_cs(info, cfg_get_var(pmng_get_cfg(pmng), 
				"charset-save-info"), pmng);
	ret = p->m_pd.m_save_info(file_name, info);
	si_convert_cs(info, was_cs, pmng);
	if (was_cs != NULL)
		free(was_cs);
		*/
} /* End of 'md_save_info' function */

/* End of 'metadata_io.c' file */

