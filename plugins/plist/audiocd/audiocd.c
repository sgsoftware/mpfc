/******************************************************************
 * Copyright (C) 2003 - 2011 by SG Software.
 *
 * SG MPFC. Audio CD playlist plugin.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include "types.h"
#include "plp.h"
#include "logger.h"
#include "pmng.h"
#include "util.h"

/* Logger */
static logger_t *audiocd_log = NULL;

static char *audiocd_desc = "AudioCD playlist plugin";

/* Plugin author */
static char *audiocd_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Get supported formats function */
char *audiocd_get_prefix( void )
{
	return "audiocd://";
} /* End of 'audiocd_get_formats' function */

/* Determine the number of tracks in AudioCD */
guint64 audiocd_get_num_tracks( void )
{
	guint64 num_tracks = 0;

	/* Create cdda element */
	GstElement *cdda = gst_element_make_from_uri(GST_URI_SRC, "cdda://", NULL, NULL);
	if (!cdda)
	{
		logger_error(audiocd_log, 0, _("Unable to load Audio CD"));
		return 0;
	}

	//rb_debug ("cdda longname: %s", gst_element_factory_get_longname (gst_element_get_factory (priv->cdda)));
	//g_object_set (G_OBJECT (priv->cdda), "device", priv->device_path, NULL);
	GstElement *pipeline = gst_pipeline_new("pipeline");
	GstElement *sink = gst_element_factory_make("fakesink", "fakesink");
	gst_bin_add_many(GST_BIN(pipeline), cdda, sink, NULL);
	gst_element_link(cdda, sink);

	/* disable paranoia (if using cdparanoia) since we're only reading track information here.
	 * this reduces cdparanoia's cache size, so the process is much faster.
	 */
	//if (g_object_class_find_property (G_OBJECT_GET_CLASS (source), "paranoia-mode"))
	//	g_object_set (source, "paranoia-mode", 0, NULL);

	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
	if (ret == GST_STATE_CHANGE_ASYNC)
		ret = gst_element_get_state(pipeline, NULL, NULL, 3 * GST_SECOND);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		logger_error(audiocd_log, 0, _("Unable to load Audio CD"));
		goto finally;
	}

	GstFormat fmt = gst_format_get_by_nick("track");
	if (!gst_element_query_duration(cdda, fmt, &num_tracks))
	{
		logger_error(audiocd_log, 0, _("Unable to load Audio CD"));
		goto finally;
	}

finally:
	g_object_unref(pipeline);
	return num_tracks;
}

/* Parse playlist and handle its contents */
plp_status_t audiocd_for_each_item( char *pl_name, void *ctx, plp_func_t f )
{
	guint64 num_tracks = audiocd_get_num_tracks();
	if (!num_tracks)
		return PLP_STATUS_FAILED;

	/* Now add 'cdda://idx' elements */
	for ( unsigned long long i = 0; i < num_tracks; i++ )
	{
		char name[64];
		snprintf(name, sizeof(name), "cdda://%llu", i + 1);
		logger_error(audiocd_log, 0, _("adding %s"), name);
		song_metadata_t metadata = SONG_METADATA_EMPTY;

		plp_status_t res = f(ctx, name, &metadata);
		if (res != PLP_STATUS_OK)
			return res;
	}
	return PLP_STATUS_OK;
} /* End of 'audiocd_for_each_item' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = audiocd_desc;
	pd->m_author = audiocd_author;
	PLIST_DATA(pd)->m_get_prefix = audiocd_get_prefix;
	PLIST_DATA(pd)->m_for_each_item = audiocd_for_each_item;
	audiocd_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'audiocd.c' file */

