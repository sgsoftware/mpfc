/******************************************************************
 * Copyright (C) 2003 - 2011 by SG Software.
 *
 * SG MPFC. M3U playlist plugin.
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
#include <string.h>
#include "types.h"
#include "file.h"
#include "plp.h"
#include "logger.h"
#include "pmng.h"
#include "util.h"

/* Logger */
static logger_t *m3u_log = NULL;

static char *m3u_desc = "m3u playlist plugin";

/* Plugin author */
static char *m3u_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Get supported formats function */
void m3u_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "m3u");
	if (content_type != NULL)
		strcpy(content_type, "");
} /* End of 'm3u_get_formats' function */

/* Parse playlist and handle its contents */
plp_status_t m3u_for_each_item( char *pl_name, void *ctx, plp_func_t f )
{
	file_t *fd;
	char str[1024];
	bool_t ext_info;

	/* Try to open file */
	fd = file_open(pl_name, "rt", NULL);
	if (fd == NULL)
	{
		logger_error(m3u_log, 0, _("Unable to read %s file"), pl_name);
		return PLP_STATUS_FAILED;
	}

	/* Read file head */
	file_gets(str, sizeof(str), fd);
	ext_info = !strncmp(str, "#EXTM3U", 7);
	if (!ext_info)
		file_seek(fd, 0, SEEK_SET);
		
	/* Read file contents */
	while (!file_eof(fd))
	{
		char len[10], *title;
		int i, j, song_len, str_len;

		/* Read file name if no extended info is supplied */
		if (!ext_info)
		{
			file_gets(str, sizeof(str), fd);
			if (file_eof(fd))
				break;
			util_del_nl(str, str);

			song_metadata_t metadata = SONG_METADATA_EMPTY;
			f(ctx, str, &metadata);
			continue;
		}
		
		/* Read song length and title string */
		file_gets(str, sizeof(str), fd);
		if (file_eof(fd) || strlen(str) < 10)
			break;

		/* Extract song length from string read */
		for ( i = 8, j = 0; str[i] && str[i] != ',' && j < (sizeof(len) - 1); 
				i ++, j ++ )
			len[j] = str[i];
		len[j] = 0;
		if (str[i])
			song_len = atoi(len);
		title = strdup(&str[i + 1]);
		util_del_nl(title, title);

		/* Read song file name */
		file_gets(str, sizeof(str), fd);
		util_del_nl(str, str);

		song_metadata_t metadata = SONG_METADATA_EMPTY;
		metadata.m_title = title;
		metadata.m_len = song_len;
		f(ctx, str, &metadata);
		free(title);
	}

	/* Close file */
	file_close(fd);
	return PLP_STATUS_OK;
} /* End of 'm3u_for_each_item' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = m3u_desc;
	pd->m_author = m3u_author;
	PLIST_DATA(pd)->m_get_formats = m3u_get_formats;
	PLIST_DATA(pd)->m_for_each_item = m3u_for_each_item;
	m3u_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'm3u.c' file */

