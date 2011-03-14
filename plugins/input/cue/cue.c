/******************************************************************
 * Copyright (C) 2005 by SG Software.
 *
 * SG MPFC. Cue format support plugin.
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

//#include "config.h"
#include <ctype.h>
#define __MPFC_OUTER__
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include "types.h"
#include "cue_sheet.h"
#include "inp.h"
#include "pmng.h"
#include "util.h"

/* Directory data type */
typedef struct
{
	bool_t m_is_cue;
	union
	{
		DIR *m_dir;
		struct
		{
			cue_sheet_t *m_sheet;
			char *m_cuename;
			int m_cur_track;
		} m_cue;
	} m_data;
} cue_dir_data_t;

/* Plugins manager */
static pmng_t *cue_pmng = NULL;

/* Plugin description */
static char *cue_desc = "Cue format support plugin";

/* Plugin author */
static char *cue_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Logger */
static logger_t *cue_log = NULL;

/* Parse cue track name */
bool_t cue_parse_track_name( char *name, char **cuename, int *track_num )
{
	char *dir_sep, *s, *cue_ext;
	int tn;

	logger_debug(cue_log, "cue: in cue_parse_track_name %s", name);

	/* Parse */
	dir_sep = strrchr(name, '/');
	if (dir_sep == NULL)
	{
		logger_debug(cue_log, "cue: no directory separator");
		return FALSE;
	}

	/* Check that 'directory' name has '.cue' extension */
	cue_ext = strrchr(name, '.');
	if (cue_ext == NULL || cue_ext > dir_sep)
	{
		logger_debug(cue_log, "cue: extension not found");
		return FALSE;
	}
	if (strncmp(cue_ext, ".cue", 4))
	{
		logger_debug(cue_log, "cue: extension test failed");
		return FALSE;
	}

	/* Check that track name consists entirely of digits */
	for ( s = dir_sep + 1; *s; s ++ )
	{
		if (!isdigit(*s))
		{
			logger_debug(cue_log, "cue: not a digit");
			return FALSE;
		}
	}
	tn = atoi(dir_sep + 1);
	if (tn == 0)
	{
		logger_debug(cue_log, "cue: track is 0");
		return FALSE;
	}

	/* Save */
	if (cuename != NULL)
		(*cuename) = strndup(name, dir_sep - name);
	if (track_num != NULL)
		(*track_num) = tn;
	logger_debug(cue_log, "cue: all tests passed");
	return TRUE;
} /* End of 'cue_parse_track_name' function */

/* Get song info */
song_info_t *cue_get_info( char *file_name, int *len )
{
	char *cuename;
	int track_num;
	cue_sheet_t *cs;
	cue_track_t *track, *master;
	song_info_t *si;
	char track_num_str[10];

	(*len) = 0;

	logger_debug(cue_log, "cue: cue_get_info(%s)", file_name);

	/* Load cue sheet */
	if (!cue_parse_track_name(file_name, &cuename, &track_num))
		return NULL;
	logger_debug(cue_log, "cue: cuename is %s, track num is %d", 
			cuename, track_num);
	cs = cue_sheet_parse(cuename);
	if (cs == NULL)
	{
		free(cuename);
		return NULL;
	}

	/* Get track */
	if (track_num < 1 || track_num >= cs->m_num_tracks)
	{
		cue_sheet_free(cs);
		free(cuename);
		return NULL;
	}
	track = cs->m_tracks[track_num];
	master = cs->m_tracks[0];

	/* Make info */
	si = si_new();
	if (si == NULL)
	{
		cue_sheet_free(cs);
		free(cuename);
		return NULL;
	}
	si_set_album(si, master->m_title);
	si_set_artist(si, track->m_performer);
	si_set_name(si, track->m_title);
	snprintf(track_num_str, sizeof(track_num_str), "%02d", track_num);
	si_set_track(si, track_num_str);

	/* Free memory */
	cue_sheet_free(cs);
	free(cuename);
	return si;
} /* End of 'cue_get_info' function */

/* Check if given file is a cue track */
bool_t cue_is_our_file( char *name )
{
	return (!strcmp(util_extension(name), "cue")) || 
		cue_parse_track_name(name, NULL, NULL);
} /* End of 'cue_is_our_file' function */

/* Open VFS directory */
void *cue_opendir( char *name )
{
	cue_dir_data_t *dd;

	logger_debug(cue_log, "cue_opendir(%s)", name);

	/* Do cue staff if file has 'cue' extension and is
	 * in fact not a directory */
	if (!strcmp(util_extension(name), "cue"))
	{
		cue_sheet_t *cs = cue_sheet_parse(name);
		if (cs == NULL)
		{
			logger_error(cue_log, 0, "cue: failed to parse %s", name);
			return NULL;
		}

		/* Create data */
		dd = (cue_dir_data_t *)malloc(sizeof(*dd));
		if (dd == NULL)
		{
			logger_error(cue_log, 0, "cue: no enough memory");
			cue_sheet_free(cs);
			return NULL;
		}
		dd->m_is_cue = TRUE;
		dd->m_data.m_cue.m_sheet = cs;
		dd->m_data.m_cue.m_cuename = strdup(name);
		if (dd->m_data.m_cue.m_cuename == NULL)
		{
			logger_error(cue_log, 0, "cue: no enough memory");
			free(dd);
			cue_sheet_free(cs);
			return NULL;
		}
		dd->m_data.m_cue.m_cur_track = 0;
		return dd;
	}

	/* A normal directory */
	dd = (cue_dir_data_t *)malloc(sizeof(*dd));
	if (dd == NULL)
	{
		logger_error(cue_log, 0, "cue: no enough memory");
		return NULL;
	}
	dd->m_is_cue = FALSE;
	dd->m_data.m_dir = opendir(name);
	return dd;
} /* End of 'cue_opendir' function */

/* Close VFS directory */
void cue_closedir( void *dir )
{
	cue_dir_data_t *dd = (cue_dir_data_t *)dir;

	if (dd == NULL)
		return;

	if (dd->m_is_cue)
	{
		cue_sheet_free(dd->m_data.m_cue.m_sheet);
		if (dd->m_data.m_cue.m_cuename != NULL)
			free(dd->m_data.m_cue.m_cuename);
	}
	else 
		closedir(dd->m_data.m_dir);
	free(dd);
} /* End of 'cue_closedir' function */

/* Read VFS directory */
char *cue_readdir( void *dir )
{
	cue_dir_data_t *dd = (cue_dir_data_t *)dir;
	struct dirent *de;

	if (dd == NULL)
		return NULL;

	if (dd->m_is_cue)
	{
		char track_name[256];

		int track = ++dd->m_data.m_cue.m_cur_track;
		if (track >= dd->m_data.m_cue.m_sheet->m_num_tracks)
			return NULL;

		snprintf(track_name, sizeof(track_name), "%02d", track);
		return strdup(track_name);
	}

	de = readdir(dd->m_data.m_dir);
	return (de == NULL) ? NULL : strdup(de->d_name);
} /* End of 'cue_readdir' function */

/* Get VFS file parameters */
int cue_stat( char *name, struct stat *sb )
{
	char *ext;

	logger_debug(cue_log, "cue_stat(%s)", name);

	memset(sb, 0, sizeof(*sb));

	/* A cue file is treated as a directory */
	if (!strcmp(util_extension(name), "cue"))
	{
		sb->st_mode = S_IFDIR;
		return 0;
	}
	/* A cue track */
	else if (cue_parse_track_name(name, NULL, NULL))
	{
		sb->st_mode = S_IFREG;
		return 0;
	}

	/* A normal file */
	return stat(name, sb);
} /* End of 'cue_stat' function */

/* Redirect song */
char *cue_redirect( char *filename, inp_redirect_params_t *rp )
{
	char *sharp_pos;
	char *cue_name = NULL;
	int track_num;
	cue_sheet_t *cs = NULL;
	cue_track_t *track;
	char redir_name[MAX_FILE_NAME];

	logger_debug(cue_log, "cue: cue_redirect %s", filename);

	/* Get corresponding cue name and track number */
	if (!cue_parse_track_name(filename, &cue_name, &track_num))
		return NULL;

	/* Load cue sheet */
	cs = cue_sheet_parse(cue_name);
	if (cs == NULL)
	{
		logger_error(cue_log, 0, "cue: failed to load cue sheet %s", cue_name);
		goto fail;
	}

	/* Get track */
	if (track_num < 1 || track_num >= cs->m_num_tracks)
	{
		logger_error(cue_log, 0, "cue: no such track: %d", track_num);
		goto fail;
	}
	track = cs->m_tracks[track_num];

	/* Create song object */
	snprintf(redir_name, sizeof(redir_name), "%s/%s", 
			dirname(cue_name), cs->m_file_name);
	logger_debug(cue_log, "cue: redirection name is %s", redir_name);

	/* Set redirection parameters */
	if (rp != NULL)
	{
		memset(rp, 0, sizeof(*rp));
		rp->m_start_time = track->m_indices[1] / 75;
		rp->m_end_time = (track_num < cs->m_num_tracks - 1) ? 
			cs->m_tracks[track_num + 1]->m_indices[1] / 75 : -1;
		logger_debug(cue_log, "cue: start time is %d, end time is %d", 
				rp->m_start_time, rp->m_end_time);
	}

	/* Free memory */
	cue_sheet_free(cs);
	free(cue_name);
	return strdup(redir_name);

fail:
	if (cs != NULL)
		cue_sheet_free(cs);
	if (cue_name != NULL)
		free(cue_name);
	return NULL;
} /* End of 'cue_redirect' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = cue_desc;
	pd->m_author = cue_author;
	INP_DATA(pd)->m_flags = INP_VFS;
	INP_DATA(pd)->m_get_info = cue_get_info;
	INP_DATA(pd)->m_vfs_opendir = cue_opendir;
	INP_DATA(pd)->m_vfs_closedir = cue_closedir;
	INP_DATA(pd)->m_vfs_readdir = cue_readdir;
	INP_DATA(pd)->m_vfs_stat = cue_stat;
	INP_DATA(pd)->m_is_our_file = cue_is_our_file;
	INP_DATA(pd)->m_redirect = cue_redirect;
	cue_pmng = pd->m_pmng;
	cue_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'cue.c' file */

