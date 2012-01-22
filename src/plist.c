/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Play list manipulation functions.
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
#include <errno.h>
#include <glib.h>
#include <glob.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __USE_GNU
#include <unistd.h>
#include "types.h"
#include "file.h"
#include "file_utils.h"
#include "inp.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "song.h"
#include "util.h"
#include "undo.h"
#include "wnd.h"
#include "info_rw_thread.h"

/* Create a new play list */
plist_t *plist_new( int start_pos )
{
	plist_t *pl;

	/* Try to allocate memory for play list object */
	pl = (plist_t *)malloc(sizeof(plist_t));
	if (pl == NULL)
		return NULL;

	/* Set play list fields */
	pl->m_start_pos = start_pos;
	pl->m_scrolled = 0;
	pl->m_sel_start = pl->m_sel_end = 0;
	pl->m_cur_song = -1;
	pl->m_visual = FALSE;
	pl->m_len = 0;
	pl->m_list = NULL;
	pthread_mutex_init(&pl->m_mutex, NULL);
	return pl;
} /* End of 'plist_new' function */

/* Destroy play list */
void plist_free( plist_t *pl )
{
	if (pl != NULL)
	{
		if (pl->m_list != NULL)
		{
			int i;
			
			plist_lock(pl);
			for ( i = 0; i < pl->m_len; i ++ )
				song_free(pl->m_list[i]);
			free(pl->m_list);
			plist_unlock(pl);
		}
		
		pthread_mutex_destroy(&pl->m_mutex);
		free(pl);
	}
} /* End of 'plist_free' function */

/* Add a file to play list */
bool_t plist_add( plist_t *pl, char *filename )
{
	plist_set_t *set;
	bool_t ret;

	/* Initialize one-element set and add it */
	set = plist_set_new(TRUE);
	plist_set_add(set, filename);
	ret = plist_add_set(pl, set);
	plist_set_free(set);

	pmng_hook(player_pmng, "playlist");
	return ret;
} /* End of 'plist_add' function */

/* Save play list */
bool_t plist_save( plist_t *pl, char *filename )
{
	char *ext = util_extension(filename);
	assert(pl);

	if (!strcasecmp(ext, "m3u"))
		return plist_save_m3u(pl, filename);
	else if (!strcasecmp(ext, "pls"))
		return plist_save_pls(pl, filename);
	return FALSE;
} /* End of 'plist_save' function */

/* Save play list to M3U format */
bool_t plist_save_m3u( plist_t *pl, char *filename )
{
	FILE *fd;
	int i;

	/* Try to create file */
	fd = util_fopen(filename, "wt");
	if (fd == NULL)
		return FALSE;
	
	/* Write list head */
	fprintf(fd, "#EXTM3U\n");
	for ( i = 0; i < pl->m_len; i ++ )
	{
		song_t *s = pl->m_list[i];
		fprintf(fd, "#EXTINF:%i", s->m_len);
		if (s->m_start_time >= 0)
			fprintf(fd, "-%i", s->m_start_time);

		fprintf(fd, ",%s\n%s\n", STR_TO_CPTR(s->m_title), song_get_name(s));
	}

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'plist_save_m3u' function */

/* Save play list to PLS format */
bool_t plist_save_pls( plist_t *pl, char *filename )
{
	FILE *fd;
	int i;

	/* Try to create file */
	fd = util_fopen(filename, "wt");
	if (fd == NULL)
		return FALSE;
	
	/* Write list head */
	fprintf(fd, "[playlist]\nnumberofentries=%d\n", pl->m_len);
	for ( i = 0; i < pl->m_len; i ++ )
		fprintf(fd, "File%d=%s\n", i + 1, song_get_name(pl->m_list[i]));

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'plist_save_pls' function */

/* Compare two songs for sorting */
int plist_song_cmp( song_t *s1, song_t *s2, int criteria )
{
	char dir1[MAX_FILE_NAME], dir2[MAX_FILE_NAME];
	int res;
	
	if (s1 == NULL || s2 == NULL)
		return 0;

	switch (criteria)
	{
	case PLIST_SORT_BY_TITLE:
		return strcmp(STR_TO_CPTR(s1->m_title), STR_TO_CPTR(s2->m_title));
	case PLIST_SORT_BY_NAME:
		return strcmp(util_short_name(song_get_name(s1)),
				util_short_name(song_get_name(s2)));
	case PLIST_SORT_BY_PATH:
		return strcmp(song_get_name(s1), song_get_name(s2));
	case PLIST_SORT_BY_TRACK:
		/* Compare directories first */
		util_get_dir_name(dir1, song_get_name(s1));
		util_get_dir_name(dir2, song_get_name(s2));
		res = strcmp(dir1, dir2);
		if (res != 0)
			return res;

		/* Now compare tracks */
		if (s1->m_info != NULL && s2->m_info != NULL)
		{
			int t1 = atoi(s1->m_info->m_track), t2 = atoi(s2->m_info->m_track);
			if (t1 != t2)
				return t1 - t2;
		}

		/* Now compare file names */
		return strcmp(util_short_name(song_get_name(s1)),
				util_short_name(song_get_name(s2)));
	}
	return 0;
} /* End of 'plist_song_cmp' function */

/* Sort play list with specified bounds */
void plist_sort_bounds( plist_t *pl, int start, int end, int criteria )
{
	int i, j, was_song;
	song_t *cur_song;
	song_t **was_list = NULL;
	bool_t finished = FALSE;

	assert(pl);
	if (start > end)
		return;
	if (start < 0)
		start = 0;
	if (end >= pl->m_len)
		end = pl->m_len - 1;

	/* Wait until info isn't got */
	while ((criteria == PLIST_SORT_BY_TITLE || 
				criteria == PLIST_SORT_BY_TRACK) && !finished)
	{
		finished = TRUE;
		for ( i = 0; i < pl->m_len; i ++ )
		{
			if (pl->m_list[i]->m_flags & SONG_INFO_READ)
			{
				finished = FALSE;
				break;
			}
		}
		util_wait();
	}

	/* Lock play list */
	plist_lock(pl);

	/* Save play list */
	was_list = (song_t **)malloc(sizeof(song_t *) * pl->m_len);
	memcpy(was_list, pl->m_list, sizeof(song_t *) * pl->m_len);
	
	/* Save current song */
	was_song = pl->m_cur_song;
	cur_song = (pl->m_cur_song < 0) ? NULL : pl->m_list[was_song];

	/* Sort */
	for ( i = start; i < end; i ++ )
	{
		int k = i + 1, j;
		song_t *s = pl->m_list[k];

		for ( j = i; j >= start; j -- )
		{
			/* Compare songs and save current preferred position */
			if (plist_song_cmp(s, pl->m_list[j], criteria) < 0)
				k = j;
			else
				break;
		}

		/* Paste string to its place */
		if (k <= i)
		{
			memmove(&pl->m_list[k + 1], &pl->m_list[k],
					(i - k + 1) * sizeof(*pl->m_list));
			pl->m_list[k] = s;
		}
	}

	/* Find current song */
	if (cur_song != NULL)
	{
		int i;

		for ( i = start; i <= end; i ++ )
			if (pl->m_list[i] == cur_song)
			{
				pl->m_cur_song = i;
				break;
			}
	}

	/* Store undo information */
	if (player_store_undo)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_SORT;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_sort.m_was_song = was_song;
		undo->m_data.m_sort.m_transform = (int *)malloc(
				sizeof(int) * pl->m_len);
		for ( i = 0; i < pl->m_len; i ++ )
		{
			for ( j = 0; j < pl->m_len; j ++ )
				if (was_list[i] == pl->m_list[j])
				{
					undo->m_data.m_sort.m_transform[i] = j;
					break;
				}
		}
		undo_add(player_ul, undo);
	}
	free(was_list);

	/* Unlock play list */
	plist_unlock(pl);
} /* End of 'plist_sort_bounds' function */

/* Sort play list */
void plist_sort( plist_t *pl, bool_t global, int criteria )
{
	int start, end;
	assert(pl);

	/* Get sort start and end */
	if (global)
		start = 0, end = pl->m_len - 1;
	else
		PLIST_GET_SEL(pl, start, end);

	/* Sort */
	plist_sort_bounds(pl, start, end, criteria);
	pmng_hook(player_pmng, "playlist");
} /* End of 'plist_sort' function */

/* Remove selected songs from play list */
void plist_rem( plist_t *pl )
{
	int start, end, i, cur;
	assert(pl);

	/* Get real selection bounds */
	PLIST_GET_SEL(pl, start, end);
	if (start >= pl->m_len)
		start = pl->m_len - 1;
	if (end >= pl->m_len)
		end = pl->m_len - 1;
	if (start < 0)
		start = 0;
	if (end < 0)
		end = 0;

	/* Check if we have anything to delete */
	if (!pl->m_len)
		return;

	/* Store undo information */
	if (player_store_undo)
	{
		struct tag_undo_list_item_t *undo;
		struct tag_undo_list_rem_t *data;
		int j;
		song_metadata_t metadata_empty = SONG_METADATA_EMPTY;
		
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_REM;
		undo->m_next = undo->m_prev = NULL;
		data = &undo->m_data.m_rem;
		data->m_num_files = end - start + 1;
		data->m_start_pos = start;
		data->m_files = (struct song_name *)malloc(sizeof(struct song_name) * data->m_num_files);
		for ( i = start, j = 0; i <= end; i ++, j ++ )
		{
			song_t *s = pl->m_list[i];
			struct song_name *sn = &data->m_files[j];
			if (s->m_filename)
			{
				sn->m_filename = strdup(s->m_filename);
				sn->m_fullname = NULL;
			}
			else
			{
				sn->m_fullname = strdup(s->m_fullname);
				sn->m_filename = NULL;
			}

			song_metadata_t *metadata = &sn->m_metadata;
			(*metadata) = metadata_empty;
			metadata->m_start_time = s->m_start_time;
			metadata->m_end_time = s->m_end_time;
			metadata->m_len = s->m_len;
			if (s->m_default_title)
				metadata->m_title = strdup(s->m_default_title);
		}
		undo_add(player_ul, undo);
	}

	/* Stop currently playing song if it is inside area being removed */
	if (pl->m_cur_song >= start && pl->m_cur_song <= end)
	{
		player_end_play(TRUE);
	}

	/* Unlock play list */
	plist_lock(pl);

	/* Free memory */
	for ( i = start; i <= end; i ++ )
		song_free(pl->m_list[i]);

	/* Shift songs list and reallocate memory */
	memmove(&pl->m_list[start], &pl->m_list[end + 1],
			(pl->m_len - end - 1) * sizeof(*pl->m_list));
	pl->m_len -= (end - start + 1);
	if (pl->m_len)
		pl->m_list = (song_t **)realloc(pl->m_list, 
				pl->m_len * sizeof(*pl->m_list));
	else
	{
		free(pl->m_list);
		pl->m_list = NULL;
	}

	/* Fix cursor */
	plist_move(pl, start, FALSE);
	pl->m_sel_start = pl->m_sel_end;
	if (pl->m_cur_song > end)
		pl->m_cur_song -= (end - start + 1);

	/* Unlock play list */
	plist_unlock(pl);

	pmng_hook(player_pmng, "playlist");
} /* End of 'plist_rem' function */

/* Search for string */
bool_t plist_search( plist_t *pl, char *pstr, int dir, int criteria )
{
	int i, count = 0;
	bool_t found = FALSE;

	assert(pl);
	if (!pl->m_len)
		return FALSE;

	/* Search */
	for ( i = pl->m_sel_end, count = 0; count < pl->m_len && !found; count ++ )
	{
		char *str;
		song_t *s;
		
		/* Go to next song */
		i += dir;
		if (i < 0 && dir < 0)
			i = pl->m_len - 1;
		else if (i >= pl->m_len && dir > 0)
			i = 0;

		/* Search for specified string */
		s = pl->m_list[i];
		if (criteria != PLIST_SEARCH_TITLE && s->m_info == NULL)
			continue;
		switch (criteria)
		{
		case PLIST_SEARCH_TITLE:
			str = STR_TO_CPTR(s->m_title);
			break;
		case PLIST_SEARCH_NAME:
			str = s->m_info->m_name;
			break;
		case PLIST_SEARCH_ARTIST:
			str = s->m_info->m_artist;
			break;
		case PLIST_SEARCH_ALBUM:
			str = s->m_info->m_album;
			break;
		case PLIST_SEARCH_YEAR:
			str = s->m_info->m_year;
			break;
		case PLIST_SEARCH_GENRE:
			str = s->m_info->m_genre;
			break;
		case PLIST_SEARCH_COMMENT:
			str = s->m_info->m_comments;
			break;
		case PLIST_SEARCH_OWN:
			str = s->m_info->m_own_data;
			break;
		case PLIST_SEARCH_TRACK:
			str = s->m_info->m_track;
			break;
		}
		found = util_search_regexp(pstr, str, 
				cfg_get_var_int(cfg_list, "search-nocase"));
		if (found)
			plist_move(pl, i, FALSE);
	} 

	return found;
} /* End of 'plist_search' function */

/* Move cursor in play list */
void plist_move( plist_t *pl, int y, bool_t relative )
{
	int old_end;
	assert(pl);

	/* If we have empty list - set position to 0 */
	if (!pl->m_len)
	{
		pl->m_sel_start = pl->m_sel_end = pl->m_scrolled = 0;
	}
	
	/* Change play list selection end */
	old_end = pl->m_sel_end;
	pl->m_sel_end = (relative * pl->m_sel_end) + y;
	if (pl->m_sel_end < 0)
		pl->m_sel_end = 0;
	else if (pl->m_sel_end >= pl->m_len)
		pl->m_sel_end = pl->m_len - 1;

	/* Scroll if need */
	if (pl->m_sel_end < pl->m_scrolled || 
			pl->m_sel_end >= pl->m_scrolled + PLIST_HEIGHT)
	{
		pl->m_scrolled += (pl->m_sel_end - old_end);
		if (pl->m_scrolled < 0)
			pl->m_scrolled = 0;
	}
	if (pl->m_scrolled >= pl->m_len - PLIST_HEIGHT)
		pl->m_scrolled = pl->m_len - PLIST_HEIGHT;
	if (pl->m_scrolled < 0)
		pl->m_scrolled = 0;

	/* Let selection start follow the end in non-visual mode */
	if (!pl->m_visual)
		pl->m_sel_start = pl->m_sel_end;
} /* End of 'plist_move' function */

/* Centrize view */
void plist_centrize( plist_t *pl, int index )
{
	assert(pl);

	if (!pl->m_len)
		return;

	if (index < 0)
		index = pl->m_cur_song;
	if (index >= 0)
	{
		pl->m_sel_end = index;
		if (!pl->m_visual)
			pl->m_sel_start = pl->m_sel_end;

		pl->m_scrolled = pl->m_sel_end - (PLIST_HEIGHT + 1) / 2;
		if (pl->m_scrolled >= pl->m_len - PLIST_HEIGHT)
			pl->m_scrolled = pl->m_len - PLIST_HEIGHT;
		if (pl->m_scrolled < 0)
			pl->m_scrolled = 0;
	}
} /* End of 'plist_centrize' function */

/* Display play list */
void plist_display( plist_t *pl, wnd_t *wnd )
{
	int i, j, start, end, l_time = 0, s_time = 0;
	char time_text[80];

	assert(pl);
	PLIST_GET_SEL(pl, start, end);

	/* Display each song */
	for ( i = 0, j = pl->m_scrolled; i < PLIST_HEIGHT; i ++, j ++ )
	{
		int attrib;
		
		/* Set respective print attributes */
		if (j >= start && j <= end)
		{
			if (j == pl->m_cur_song)
				wnd_apply_style(wnd, "plist-sel-and-play-style");
			else
				wnd_apply_style(wnd, "plist-selected-style");
		}
		else
		{
			if (j == pl->m_cur_song)
				wnd_apply_style(wnd, "plist-playing-style");
			else
				wnd_apply_style(wnd, "plist-style");
		}
		
		/* Print song title */
		if (j < pl->m_len)
		{
			song_t *s = pl->m_list[j];
			char len[10];
			int x;
			int queueList;
			
			wnd_move(wnd, 0, 0, pl->m_start_pos + i);
			wnd_printf(wnd, WND_PRINT_ELLIPSES, WND_WIDTH(wnd) - 8, 
					"%i. %s", j + 1, STR_TO_CPTR(s->m_title));
			for(queueList=0;queueList<num_queued_songs;queueList++)
			{
				if(queued_songs[queueList] == j)
					wnd_printf(wnd, 0, 0, "    #%i in queue...", queueList+1);
			}
			sprintf(len, "%i:%02i", s->m_len / 60, s->m_len % 60);
			wnd_move(wnd, WND_MOVE_ADVANCE, WND_WIDTH(wnd) - strlen(len) - 1, 
					pl->m_start_pos + i);
			wnd_printf(wnd, 0, 0, "%s", len);
		}
	}

	/* Display play list time */
	if (pl->m_len)
	{
		for ( i = 0; i < pl->m_len; i ++ )
			l_time += pl->m_list[i]->m_len;
		for ( i = start; i <= end; i ++ )
			s_time += pl->m_list[i]->m_len;
	}
	wnd_apply_style(wnd, "plist-time-style");
	sprintf(time_text, ngettext("%i/%i song; %i:%02i:%02i/%i:%02i:%02i",
				"%i/%i songs; %i:%02i:%02i/%i:%02i:%02i", pl->m_len),
			(end >= 0 && pl->m_len > 0) ? end - start + 1 : 0, pl->m_len,
			s_time / 3600, (s_time % 3600) / 60, s_time % 60,
			l_time / 3600, (l_time % 3600) / 60, l_time % 60);
	wnd_move(wnd, 0, WND_WIDTH(wnd) - strlen(time_text) - 1, 
			pl->m_start_pos + PLIST_HEIGHT);
	wnd_printf(wnd, 0, 0, "%s", time_text);
} /* End of 'plist_display' function */

/* Lock play list */
void plist_lock( plist_t *pl )
{
	pthread_mutex_lock(&pl->m_mutex);
} /* End of 'plist_lock' function */

/* Unlock play list */
void plist_unlock( plist_t *pl )
{
	pthread_mutex_unlock(&pl->m_mutex);
} /* End of 'plist_unlock' function */

/* Move selection in play list */
void plist_move_sel( plist_t *pl, int y, bool_t relative )
{
	int start, end, i, j, num_songs;
	song_t *cur_song;
	
	if (pl == NULL)
		return;

	PLIST_GET_SEL(pl, start, end);
	if (start < 0 || end < 0)
		return;

	/* Lock play list */
	plist_lock(pl);
	
	/* Check boundaries */
	if (relative)
		y = start + y;
	if (y < 0)
		y = 0;
	else if (y >= pl->m_len - (end - start))
		y = pl->m_len - (end - start) - 1;
	if (pl->m_cur_song >= 0)
		cur_song = pl->m_list[pl->m_cur_song];
	else
		cur_song = NULL;
	num_songs = end - start + 1;

	/* Store undo information */
	if (player_store_undo)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_MOVE;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_move_plist.m_start = start;
		undo->m_data.m_move_plist.m_end = end;
		undo->m_data.m_move_plist.m_to = y;
		undo_add(player_ul, undo);
	}

	/* Move */
	if (y - start < 0)
	{
		for ( i = start; i > y; i -- )
		{
			song_t *s = pl->m_list[i - 1];
			memmove(&pl->m_list[i - 1], &pl->m_list[i], 
					num_songs * sizeof(song_t *));
			pl->m_list[i + num_songs - 1] = s;
		}
	}
	else
	{
		for ( i = start; i < y; i ++ )
		{
			song_t *s = pl->m_list[i + num_songs];
			memmove(&pl->m_list[i + 1], &pl->m_list[i], 
					num_songs * sizeof(song_t *));
			pl->m_list[i] = s;
		}
	}

	/* Update selection indecies and current song */
	pl->m_sel_start += (y - start);
	pl->m_sel_end += (y - start);
	for ( i = 0; i < pl->m_len; i ++ )
		if (pl->m_list[i] == cur_song)
		{
			pl->m_cur_song = i;
			break;
		}

	/* Scroll if need */
	if (pl->m_sel_end < pl->m_scrolled || 
			pl->m_sel_end >= pl->m_scrolled + PLIST_HEIGHT)
	{
		pl->m_scrolled += (y - start);
		if (pl->m_scrolled < 0)
			pl->m_scrolled = 0;
		else if (pl->m_scrolled >= pl->m_len)
			pl->m_scrolled = pl->m_len - 1;
	}

	/* Unlock play list */
	plist_unlock(pl);
} /* End of 'plist_move_sel' function */

/* Reload all songs information */
void plist_reload_info( plist_t *pl, bool_t global )
{
	int i, start, end;
	
	if (pl == NULL || !pl->m_len)
		return;

	/* Update info */
	if (global)
	{
		start = 0;
		end = pl->m_len - 1;
	}
	else
	{
		PLIST_GET_SEL(pl, start, end);
	}

	for ( i = start; i <= end; i ++ )
	{
		song_t *s = pl->m_list[i];
		irw_push(s, SONG_INFO_READ);
	}
} /* End of 'plist_reload_info' function */

/* Check if specified file name belongs to an object */
bool_t plist_is_obj( char *filename )
{
	char *s = strchr(filename, ':');
	
	if (s != NULL && *(s + 1) != '/')
		return TRUE;
	else
		return FALSE;
} /* End of 'plist_is_obj' function */

/* Set info for all scheduled songs */
void plist_flush_scheduled( plist_t *pl )
{
	int i;

	for ( i = 0; i < pl->m_len; i ++ )
	{
		song_t *s = pl->m_list[i];
		if (s->m_flags & SONG_SCHEDULE)
		{
			irw_push(s, SONG_INFO_READ);
			s->m_flags &= (~SONG_SCHEDULE);
		}
	}
} /* End of 'plist_flush_scheduled' function */

typedef struct
{
	plist_t *pl;
	char *m_pl_name;
	int num_added;
} plist_cb_ctx_t;

/* Cue sheets often have a .wav file specified
 * while actually relating to an encoded file
 * Try to fix this.
 * By the way return full name */
static bool_t plist_fix_wrong_file_ext( char *name )
{
	/* Find extension start */
	int ext_pos = strlen(name) - 1;
	for ( ; ext_pos >= 0; ext_pos-- )
	{
		/* No extension found */
		if (name[ext_pos] == '/')
		{
			ext_pos = -1;
			break;
		}
		if (name[ext_pos] == '.')
			break;
	}
	if (ext_pos < 0)
		return FALSE;
	ext_pos++; /* after the dot */

	/* Try supported extensions */
	char *ext_start = name + ext_pos;
	char *ext = pmng_first_media_ext(player_pmng);
	for ( ; ext; ext = pmng_next_media_ext(ext) )
	{
		/* Try this extension */
		strcpy(ext_start, ext);

		struct stat st;
		if (!stat(name, &st))
		{
			return TRUE;
		}
	}
	assert(!ext);

	return FALSE;
} /* End of 'cue_fix_wrong_file_ext' function */

/* Playlist item adding callback */
static void plist_add_playlist_item( void *ctxv, char *name, song_metadata_t *metadata )
{
	plist_cb_ctx_t *ctx = (plist_cb_ctx_t *)ctxv;

	/* Get full path relative to the play list if it is not absolute */
	char *full_name = name;
	if ((*name) != '/')
	{
		full_name = (char*)malloc(strlen(ctx->m_pl_name) + strlen(name) +
				player_pmng->m_media_ext_max_len + 2);
		strcpy(full_name, ctx->m_pl_name);
		char *sep = strrchr(full_name, '/');
		if (sep)
			strcpy(sep + 1, name);
		else
			strcpy(full_name, name);
	}

	/* Fix extension if it is incorrect */
	struct stat st;
	if (stat(full_name, &st))
	{
		/* Enlarge memory for extension probing.
		 * But only if it was not allocated in the abs path handling block
		 * because there we already take this into account */
		if (full_name == name)
		{
			full_name = (char*)malloc(strlen(name) +
					player_pmng->m_media_ext_max_len + 1);
		}

		/* If neither extension worked don't add this item */
		if (!plist_fix_wrong_file_ext(full_name))
			goto finish;
	}

	ctx->num_added += plist_add_one_file(ctx->pl, full_name, metadata, -1);

finish:
	if (full_name != name)
		free(full_name);
} /* End of 'plist_add_playlist_item' function */

void plist_add_song( plist_t *pl, song_t *song, int where )
{
	/* Lock play list */
	plist_lock(pl);

	/* Try to reallocate memory for play list */
	int was_len = pl->m_len;
	pl->m_list = (song_t **)realloc(pl->m_list,
			sizeof(song_t *) * (pl->m_len + 1));
	if (pl->m_list == NULL)
	{
		pl->m_len = 0;
		plist_unlock(pl);
		return;
	}

	if (where < 0 || where >= pl->m_len)  
		where = pl->m_len;
	memmove(&pl->m_list[where + 1], &pl->m_list[where], 
			sizeof(song_t *) * (pl->m_len - where));
	pl->m_list[where] = song;
	pl->m_len ++;

	/* Update current song index */
	if (pl->m_cur_song >= where)
		pl->m_cur_song ++;

	/* If list was empty - put cursor to the first song */
	if (!was_len)
	{
		pl->m_sel_start = pl->m_sel_end = 0;
		pl->m_visual = FALSE;
	}

	/* Unlock play list */
	plist_unlock(pl);
}

/* Add single file to play list */
int plist_add_one_file( plist_t *pl, char *file, song_metadata_t *metadata,
		int where )
{
	song_t *song;
	int was_len;
	assert(pl);

	/* Choose if file is play list */
	char *ext = strrchr(file, '.');
	plist_plugin_t *plp = pmng_is_playlist(player_pmng, ext ? ext + 1 : "");
	if (plp)
	{
		plist_cb_ctx_t ctx = { pl, file, 0 };
		plp_status_t status = plp_for_each_item(plp, file, &ctx,
				plist_add_playlist_item);
		if (status != PLP_STATUS_OK)
			return 0;

		return ctx.num_added;
	}

	/* Initialize new song and add it to list */
	song = song_new_from_file(file, metadata);
	if (song == NULL)
	{
		plist_unlock(pl);
		return 0;
	}

	/* Schedule song for setting its info and length */
	if (!metadata->m_title)
		song->m_flags |= SONG_SCHEDULE;

	plist_add_song(pl, song, where);

	return 1;
} /* End of 'plist_add_one_file' function */

static int plist_add_file( plist_t *pl, char *full_path )
{
	song_metadata_t metadata = SONG_METADATA_EMPTY;
	return plist_add_one_file(pl, full_path, &metadata, -1);
}

static int plist_add_dir( plist_t *pl, char *dir_path );

static int plist_add_real_path( plist_t *pl, char *full_path )
{
	/* Check if this is a directory */
	bool_t is_dir;
	if (!fu_file_type(full_path, &is_dir))
		return 0;

	if (is_dir)
		return plist_add_dir(pl, full_path);
	else
		return plist_add_file(pl, full_path);
}

static int plist_add_dir( plist_t *pl, char *dir_path )
{
	/* Open directory */
	fu_dir_t *dir = fu_opendir(dir_path);
	if (!dir)
		return 0;

	int num_added = 0;
	for ( ;; )
	{
		struct dirent *de = fu_readdir(dir);
		if (!de)
			break;
		char *name = de->d_name;

		if (fu_is_special_dir(name))
			continue;

		char *full_path = util_strcat(dir_path, "/", name, NULL);
		num_added += plist_add_real_path(pl, full_path);
		free(full_path);
	}

	fu_closedir(dir);
	return num_added;
}

static int plist_add_uri( plist_t *pl, char *uri )
{
	song_metadata_t metadata = SONG_METADATA_EMPTY;
	song_t *s = song_new_from_uri(uri, &metadata);
	assert(s);
	plist_add_song(pl, s, -1);
	return 1;
}

static int plist_add_path( plist_t *pl, char *path )
{
	/* This is an URI */
	if (fu_is_prefixed(path))
		return plist_add_uri(pl, path);

	/* Construct absolute path */
	char *full_path = path;
	if ((*path) != '/')
	{
		char *dirname = get_current_dir_name();
		if (!dirname)
		{
			logger_error(player_log, 1, "get_current_dir_name failed: %s", strerror(errno));
			return 0;
		}

		full_path = util_strcat(dirname, path, NULL);
		free(dirname);
	}

	int res = plist_add_real_path(pl, full_path);

	if (full_path != path)
		free(full_path);

	return res;
}

/* Add a set of files to play list */
bool_t plist_add_set( plist_t *pl, plist_set_t *set )
{
	/* Do nothing if set is empty */
	if (pl == NULL || set == NULL)
		return FALSE;

	int plist_num = 0;

	for ( struct tag_plist_set_t *node = set->m_head; node; node = node->m_next )
	{
		/* glob patterns */
		if (set->m_patterns && !fu_is_prefixed(node->m_name))
		{
			glob_t gl;
			if (glob(node->m_name, GLOB_TILDE, NULL, &gl))
				continue;

			for ( char **path = gl.gl_pathv; *path; ++path )
				plist_num += plist_add_path(pl, *path);

			globfree(&gl);
		}
		/* or just a path */
		else
			plist_num += plist_add_path(pl, node->m_name);
	}

	/* Set info */
	plist_flush_scheduled(pl);
	
	/* Store undo information */
	if (player_store_undo && plist_num)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_ADD;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_add.m_num_songs = plist_num;
		undo->m_data.m_add.m_set = plist_set_dup(set);
		undo_add(player_ul, undo);
	}

	/* Sort added songs if need */
	if (cfg_get_var_int(cfg_list, "sort-on-load") && player_store_undo)
	{
		char *type = cfg_get_var(cfg_list, "sort-on-load-type");
		int cr = -1;

		/* Determine criteria */
		if (type == NULL)
			cr = PLIST_SORT_BY_PATH;
		else if (!strcmp(type, "sort-by-path-and-file"))
			cr = PLIST_SORT_BY_PATH;
		else if (!strcmp(type, "sort-by-title"))
			cr = PLIST_SORT_BY_TITLE;
		else if (!strcmp(type, "sort-by-file-name"))
			cr = PLIST_SORT_BY_NAME;
		else if (!strcmp(type, "sort-by-path-and-track"))
			cr = PLIST_SORT_BY_TRACK;
		if (cr >= 0)
		{
			plist_sort_bounds(pl, pl->m_len - plist_num, pl->m_len - 1, cr);
		}
	}

	return TRUE;
} /* End of 'plist_add_set' function */

/* Initialize a set of files for adding */
plist_set_t *plist_set_new( bool_t patterns )
{
	plist_set_t *set;

	/* Allocate memory */
	set = (plist_set_t *)malloc(sizeof(plist_set_t));
	if (set == NULL)
		return NULL;
	set->m_patterns = patterns;
	set->m_head = set->m_tail = NULL;
	return set;
} /* End of 'plist_set_new' function */

/* Free files set */
void plist_set_free( plist_set_t *set )
{
	if (set != NULL)
	{
		struct tag_plist_set_t *t, *t1;

		for ( t = set->m_head; t != NULL; )
		{
			t1 = t->m_next;
			free(t->m_name);
			free(t);
			t = t1;
		}
		free(set);
	}
} /* End of 'plist_set_free' function */

/* Add a file to set */
void plist_set_add( plist_set_t *set, char *name )
{
	struct tag_plist_set_t *node;
	
	if (set == NULL)
		return;

	/* Create new node */
	node = (struct tag_plist_set_t *)malloc(sizeof(*node));
	if (node == NULL)
		return;
	node->m_name = strdup(name);
	node->m_next = NULL;

	/* Add this node to the list */
	if (set->m_tail == NULL)
		set->m_head = set->m_tail = node;
	else
	{
		set->m_tail->m_next = node;
		set->m_tail = node;
	}
} /* End of 'plist_set_add' function */

/* Duplicate set */
plist_set_t *plist_set_dup( plist_set_t *set )
{
	plist_set_t *s;
	struct tag_plist_set_t *node;
	
	if (set == NULL)
		return NULL;

	s = plist_set_new(set->m_patterns);
	for ( node = set->m_head; node != NULL; node = node->m_next )
		plist_set_add(s, node->m_name);
	return s;
} /* End of 'plist_set_dup' function */

/* Clear play list */
void plist_clear( plist_t *pl )
{
	if (pl == NULL)
		return;

	pl->m_sel_start = 0;
	pl->m_sel_end = pl->m_len - 1;
	plist_rem(pl);
	pl->m_visual = FALSE;
} /* End of 'plist_clear' function */

/* End of 'plist.c' file */

