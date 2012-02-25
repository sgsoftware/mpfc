/******************************************************************
 * Copyright (C) 2003 - 2011 by SG Software.
 *
 * SG MPFC. PLS playlist plugin.
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
#include <ctype.h>
#include "types.h"
#include "file.h"
#include "plp.h"
#include "logger.h"
#include "pmng.h"
#include "util.h"

/* Logger */
static logger_t *pls_log = NULL;

static char *pls_desc = "pls playlist plugin";

/* Plugin author */
static char *pls_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Get supported formats function */
void pls_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "pls");
	if (content_type != NULL)
		strcpy(content_type, "");
} /* End of 'pls_get_formats' function */

/* Parse playlist and handle its contents */
plp_status_t pls_for_each_item( char *pl_name, void *ctx, plp_func_t f )
{
	file_t *fd;
	int num = 0;
	char str[1024];
	int num_entries;
	struct pls_entry_t
	{
		char *name;
		char *title;
		int len;
	} *entries;
	int i;

	/* Try to open file */
	fd = file_open(pl_name, "rt", NULL);
	if (fd == NULL)
	{
		logger_error(pls_log, 0, _("Unable to open file %s"), pl_name);
		return PLP_STATUS_FAILED;
	}

	/* Read header */
	file_gets(str, sizeof(str), fd);
	util_del_nl(str, str);
	if (strcasecmp(str, "[playlist]"))
	{
		file_close(fd);
		logger_error(pls_log, 1, _("%s: missing play list header"), 
				pl_name);
		return PLP_STATUS_FAILED;
	}

	/* Read number of entries */
	file_gets(str, sizeof(str), fd);
	util_del_nl(str, str);
	if (strncasecmp(str, "numberofentries=", 16))
	{
		file_close(fd);
		logger_error(pls_log, 1, _("%s: missing `numberofentries' tag"), 
				pl_name);
		return 0;
	}
	num_entries = atoi(strchr(str, '=') + 1);

	/* Allocate memory for play list entries */
	entries = (struct pls_entry_t *)malloc(sizeof(*entries) * num_entries);
	if (entries == NULL)
	{
		file_close(fd);
		logger_error(pls_log, 0, _("No enough memory"));
		return 0;
	}
	memset(entries, 0, sizeof(*entries) * num_entries);

	/* Read data */
	while (!file_eof(fd))
	{
		char *value;
		enum
		{
			FILE_NAME,
			TITLE,
			LENGTH
		} type;
		int index;
		char *s = str;
		
		/* Read line */
		file_gets(str, sizeof(str), fd);
		util_del_nl(str, str);

		/* Determine line type */
		if (!strncasecmp(s, "File", 4))
		{
			s += 4;
			type = FILE_NAME;
		}
		else if (!strncasecmp(s, "Title", 5))
		{
			s += 5;
			type = TITLE;
		}
		else if (!strncasecmp(s, "Length", 6))
		{
			s += 6;
			type = LENGTH;
		}
		else
		{
			continue;
		}

		/* Extract index */
		index = 0;
		while (isdigit(*s))
		{
			index *= 10;
			index += ((*s) - '0');
			s ++;
		}
		index --;
		if (index >= num_entries)
			continue;

		/* Extract value */
		if ((*s) != '=')
			continue;
		else
			s ++;
		value = strdup(s);

		/* Save entry */
		if (type == FILE_NAME)
			entries[index].name = value;
		else if (type == TITLE)
			entries[index].title = value;
		else 
		{
			entries[index].len = atoi(value);
			free(value);
		}
	}

	/* Close file */
	file_close(fd);

	/* Add the value to the play list */
	for ( i = 0; i < num_entries; i ++ )
	{
		char *name = entries[i].name;
		char *title = entries[i].title;
		int len = entries[i].len;

		if (name != NULL)
		{
			song_metadata_t metadata = SONG_METADATA_EMPTY;
			metadata.m_title = title;
			metadata.m_len = len < 0 ? 0 : len;
			f(ctx, name, &metadata);

			/* Free this entry */
			free(name);
			if (title != NULL)
				free(title);
		}
		else if (title != NULL)
			free(title);
	}
	free(entries);
	return PLP_STATUS_OK;
} /* End of 'pls_for_each_item' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = pls_desc;
	pd->m_author = pls_author;
	PLIST_DATA(pd)->m_get_formats = pls_get_formats;
	PLIST_DATA(pd)->m_for_each_item = pls_for_each_item;
	pls_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'pls.c' file */

