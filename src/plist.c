/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : plist.c
 * PURPOSE     : SG MPFC. Play list manipulation
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.03.2004
 * NOTE        : Module prefix 'plist'.
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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "colors.h"
#include "error.h"
#include "file.h"
#include "finder.h"
#include "inp.h"
#include "player.h"
#include "plist.h"
#include "pmng.h"
#include "sat.h"
#include "song.h"
#include "util.h"
#include "undo.h"
#include "window.h"

/* Check play list validity macro */
#define PLIST_ASSERT(pl) if ((pl) == NULL) return
#define PLIST_ASSERT_RET(pl, ret) if ((pl) == NULL) return (ret)

/* Create a new play list */
plist_t *plist_new( int start_pos, int height )
{
	plist_t *pl;

	/* Try to allocate memory for play list object */
	pl = (plist_t *)malloc(sizeof(plist_t));
	if (pl == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Set play list fields */
	pl->m_start_pos = start_pos;
	pl->m_height = height;
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
	set = plist_set_new();
	plist_set_add(set, filename);
	ret = plist_add_set(pl, set);
	plist_set_free(set);
	return ret;
} /* End of 'plist_add' function */

/* Add single file to play list */
int plist_add_one_file( plist_t *pl, char *filename )
{
	int i;

	PLIST_ASSERT_RET(pl, FALSE);

	/* Add song */
	i = plist_add_list(pl, filename);
	if (i < 0)
		return plist_add_song(pl, filename, NULL, 0, -1);
	else
		return i;
} /* End of 'plist_add_one_file' function */

/* Add a song to play list */
int plist_add_song( plist_t *pl, char *filename, char *title, int len, 
		int where )
{
	song_t *song;
	int was_len;
	
	PLIST_ASSERT_RET(pl, FALSE);

	/* Add object */
	if (plist_is_obj(filename))
		return plist_add_obj(pl, filename, title, where);

	/* Lock play list */
	plist_lock(pl);

	/* Try to reallocate memory for play list */
	was_len = pl->m_len;
	pl->m_list = (song_t **)realloc(pl->m_list,
			sizeof(song_t *) * (pl->m_len + 1));
	if (pl->m_list == NULL)
	{
		pl->m_len = 0;
		error_set_code(ERROR_NO_MEMORY);
		plist_unlock(pl);
		return 0;
	}

	/* Initialize new song and add it to list */
	song = song_new(filename, title, len);
	if (song == NULL)
	{
		plist_unlock(pl);
		return 0;
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

	/* Schedule song for setting its info and length */
	if (title == NULL)
		pl->m_list[where]->m_flags |= SONG_SCHEDULE;
	return 1;
} /* End of 'plist_add_song' function */

/* Add a play list file to play list */
int plist_add_list( plist_t *pl, char *filename )
{
	char *ext = util_extension(filename);
	PLIST_ASSERT_RET(pl, FALSE);

	/* Choose play list format */
	if (!strcasecmp(ext, "m3u"))
		return plist_add_m3u(pl, filename);
	else if (!strcasecmp(ext, "pls"))
		return plist_add_pls(pl, filename);
	/*else if (!strcasecmp(ext, "mpl"))
		return plist_add_mpl(pl, filename);*/
	else
		return -1;
} /* End of 'plist_add_list' function */

/* Add M3U play list */
int plist_add_m3u( plist_t *pl, char *filename )
{
	file_t *fd;
	char str[1024];
	int num = 0;

	/* Try to open file */
	fd = file_open(filename, "rt", NULL);
	if (fd == NULL)
	{
		error_set_code(ERROR_NO_SUCH_FILE);
		return 0;
	}

	/* Read file head */
	file_gets(str, sizeof(str), fd);
	if (strncmp(str, "#EXTM3U", 7))
	{
		error_set_code(ERROR_UNKNOWN_FILE_TYPE);
		return 0;
	}
		
	/* Read file contents */
	while (!file_eof(fd))
	{
		char len[10], *title;
		int i, j, song_len, str_len;
		
		/* Read song length and title string */
		file_gets(str, sizeof(str), fd);
		if (file_eof(fd) || strlen(str) < 10)
			break;

		/* Extract song length from string read */
		for ( i = 8, j = 0; str[i] && str[i] != ',' && j < sizeof(len); 
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

		/* Check if this is an object */
		if (plist_is_obj(str))
			num += plist_add_obj(pl, str, title, -1);
		/* Add this song to list */
		else
			num += plist_add_song(pl, str, title, song_len, -1);
		free(title);
	}

	/* Close file */
	file_close(fd);
	return num;
} /* End of 'plist_add_m3u' function */

/* Add PLS play list */
int plist_add_pls( plist_t *pl, char *filename )
{
	file_t *fd;
	int num = 0;
	char str[1024];

	/* Try to open file */
	fd = file_open(filename, "rt", NULL);
	if (fd == NULL)
	{
		error_set_code(ERROR_NO_SUCH_FILE);
		return 0;
	}

	/* Read header */
	file_gets(str, sizeof(str), fd);
	util_del_nl(str, str);
	if (strcasecmp(str, "[playlist]"))
	{
		file_close(fd);
		return 0;
	}
	file_gets(str, sizeof(str), fd);
	util_del_nl(str, str);
	if (strncasecmp(str, "numberofentries=", 16))
	{
		file_close(fd);
		return 0;
	}

	/* Read data */
	while (!file_eof(fd))
	{
		char *s;
		
		file_gets(str, sizeof(str), fd);
		util_del_nl(str, str);
		s = strchr(str, '=');
		if (s == NULL)
			s = str;
		else
			s ++;

		/* Check if this is an object */
		if (plist_is_obj(str))
			num += plist_add_obj(pl, s, NULL, -1);
		/* Add song */
		else
			num += plist_add_song(pl, s, NULL, 0, -1);
	}

	/* Close file */
	file_close(fd);
	return num;
} /* End of 'plist_add_pls' function */

/* Add MPL play list */
int plist_add_mpl( plist_t *pl, char *filename )
{
} /* End of 'plist_add_mpl' function */

/* Low level song adding */
bool_t __plist_add_song( plist_t *pl, char *filename, char *title, int len, 
	   int where )
{
	song_t *song;
	int was_len;
	
	PLIST_ASSERT_RET(pl, FALSE);

	/* Lock play list */
	plist_lock(pl);

	/* Try to reallocate memory for play list */
	was_len = pl->m_len;
	pl->m_list = (song_t **)realloc(pl->m_list,
			sizeof(song_t *) * (pl->m_len + 1));
	if (pl->m_list == NULL)
	{
		pl->m_len = 0;
		error_set_code(ERROR_NO_MEMORY);
		plist_unlock(pl);
		return FALSE;
	}

	/* Initialize new song and add it to list */
	song = song_new(filename, title, len);
	if (song == NULL)
	{
		plist_unlock(pl);
		return FALSE;
	}
	if (where < 0 || where >= pl->m_len)  
		pl->m_list[pl->m_len ++] = song;
	else
	{
		memmove(&pl->m_list[where + 1], &pl->m_list[where], 
				sizeof(song_t *) * (pl->m_len - where));
		pl->m_list[where] = song;
		pl->m_len ++;
	}

	/* If list was empty - put cursor to the first song */
	if (!was_len)
	{
		pl->m_sel_start = pl->m_sel_end = 0;
		pl->m_visual = FALSE;
	}

	/* Unlock play list */
	plist_unlock(pl);

	/* Update screen */
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
	
	return TRUE;
} /* End of '__plist_add_song' function */

/* Save play list */
bool_t plist_save( plist_t *pl, char *filename )
{
	char *ext = util_extension(filename);
	PLIST_ASSERT_RET(pl, FALSE);

	if (!strcasecmp(ext, "m3u"))
		return plist_save_m3u(pl, filename);
	else if (!strcasecmp(ext, "pls"))
		return plist_save_pls(pl, filename);
/*	else if (!strcasecmp(ext, "mpl"))
		return plist_save_mpl(pl, filename);*/
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
	{
		error_set_code(ERROR_NO_SUCH_FILE);
		return FALSE;
	}
	
	/* Write list head */
	fprintf(fd, "#EXTM3U\n");
	for ( i = 0; i < pl->m_len; i ++ )
	{
		fprintf(fd, "#EXTINF:%i,%s\n%s\n", pl->m_list[i]->m_len,
				STR_TO_CPTR(pl->m_list[i]->m_title), 
				pl->m_list[i]->m_file_name);
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
	{
		error_set_code(ERROR_NO_SUCH_FILE);
		return FALSE;
	}
	
	/* Write list head */
	fprintf(fd, "[playlist]\nnumberofentries=%d\n", pl->m_len);
	for ( i = 0; i < pl->m_len; i ++ )
		fprintf(fd, "File%d=%s\n", i + 1, pl->m_list[i]->m_file_name);

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'plist_save_pls' function */

/* Save play list to MPL format */
bool_t plist_save_mpl( plist_t *pl, char *filename )
{
} /* End of 'plist_save_mpl' function */

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
		return strcmp(s1->m_short_name, s2->m_short_name);
	case PLIST_SORT_BY_PATH:
		return strcmp(s1->m_file_name, s2->m_file_name);
	case PLIST_SORT_BY_TRACK:
		/* Compare directories first */
		util_get_dir_name(dir1, s1->m_file_name);
		util_get_dir_name(dir2, s2->m_file_name);
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
		return strcmp(s1->m_short_name, s2->m_short_name);
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

	PLIST_ASSERT(pl);
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
			if (pl->m_list[i]->m_flags & SONG_GET_INFO)
			{
				finished = FALSE;
				break;
			}
		}
		util_delay(0, 1000000);
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
	
	PLIST_ASSERT(pl);

	/* Get sort start and end */
	if (global)
		start = 0, end = pl->m_len - 1;
	else
		PLIST_GET_SEL(pl, start, end);

	/* Sort */
	plist_sort_bounds(pl, start, end, criteria);
} /* End of 'plist_sort' function */

/* Remove selected songs from play list */
void plist_rem( plist_t *pl )
{
	int start, end, i, cur;
	
	PLIST_ASSERT(pl);

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
		
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_REM;
		undo->m_next = undo->m_prev = NULL;
		data = &undo->m_data.m_rem;
		data->m_num_files = end - start + 1;
		data->m_start_pos = start;
		data->m_files = (char **)malloc(sizeof(char *) * data->m_num_files);
		for ( i = start, j = 0; i <= end; i ++, j ++ )
			data->m_files[j] = strdup(pl->m_list[i]->m_file_name);
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
} /* End of 'plist_rem' function */

/* Search for string */
bool_t plist_search( plist_t *pl, char *pstr, int dir, int criteria )
{
	int i, count = 0;
	bool_t found = FALSE;

	PLIST_ASSERT_RET(pl, FALSE);
	if (!pl->m_len)
		return;

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
	
	PLIST_ASSERT(pl);

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
			pl->m_sel_end >= pl->m_scrolled + pl->m_height)
	{
		pl->m_scrolled += (pl->m_sel_end - old_end);
		if (pl->m_scrolled < 0)
			pl->m_scrolled = 0;
	}
	if (pl->m_scrolled >= pl->m_len - pl->m_height)
		pl->m_scrolled = pl->m_len - pl->m_height;
	if (pl->m_scrolled < 0)
		pl->m_scrolled = 0;

	/* Let selection start follow the end in non-visual mode */
	if (!pl->m_visual)
		pl->m_sel_start = pl->m_sel_end;
} /* End of 'plist_move' function */

/* Centrize view */
void plist_centrize( plist_t *pl, int index )
{
	PLIST_ASSERT(pl);

	if (!pl->m_len)
		return;

	if (index < 0)
		index = pl->m_cur_song;
	if (index >= 0)
	{
		pl->m_sel_end = index;
		if (!pl->m_visual)
			pl->m_sel_start = pl->m_sel_end;

		pl->m_scrolled = pl->m_sel_end - (pl->m_height + 1) / 2;
		if (pl->m_scrolled >= pl->m_len - pl->m_height)
			pl->m_scrolled = pl->m_len - pl->m_height;
		if (pl->m_scrolled < 0)
			pl->m_scrolled = 0;
	}
} /* End of 'plist_centrize' function */

/* Display play list */
void plist_display( plist_t *pl, wnd_t *wnd )
{
	int i, j, start, end, l_time = 0, s_time = 0;
	char time_text[80];

	PLIST_ASSERT(pl);
	PLIST_GET_SEL(pl, start, end);

	/* Display each song */
	for ( i = 0, j = pl->m_scrolled; i < pl->m_height; i ++, j ++ )
	{
		int attrib;
		
		/* Set respective print attributes */
		if (j >= start && j <= end)
		{
			if (j == pl->m_cur_song)
				col_set_color(wnd, COL_EL_PLIST_TITLE_CUR_SEL);
			else
				col_set_color(wnd, COL_EL_PLIST_TITLE_SEL);
		}
		else
		{
			if (j == pl->m_cur_song)
				col_set_color(wnd, COL_EL_PLIST_TITLE_CUR);
			else
				col_set_color(wnd, COL_EL_PLIST_TITLE);
		}
		
		/* Print song title */
		if (j < pl->m_len)
		{
			song_t *s = pl->m_list[j];
			char len[10];
			int x;
			
			wnd_advance(wnd, 0, pl->m_start_pos + i);
			wnd_printf_bound(wnd, wnd->m_width - 13, WND_PRINT_ELLIPSES, 
					"%i. %s", j + 1, STR_TO_CPTR(s->m_title));
			sprintf(len, "%i:%02i", s->m_len / 60, s->m_len % 60);
			wnd_advance(wnd, wnd->m_width - strlen(len) - 1, 
					pl->m_start_pos + i);
			wnd_printf(wnd, "%s", len);
		}
	}
	col_set_color(wnd, COL_EL_DEFAULT);
//	wnd_set_attrib(wnd, A_NORMAL);

	/* Display play list time */
	if (pl->m_len)
	{
		for ( i = 0; i < pl->m_len; i ++ )
			l_time += pl->m_list[i]->m_len;
		for ( i = start; i <= end; i ++ )
			s_time += pl->m_list[i]->m_len;
	}
	col_set_color(wnd, COL_EL_PLIST_TIME);
	sprintf(time_text, _("%i/%i songs; %i:%02i:%02i/%i:%02i:%02i"),
			(end >= 0 && pl->m_len > 0) ? end - start + 1 : 0, pl->m_len,
			s_time / 3600, (s_time % 3600) / 60, s_time % 60,
			l_time / 3600, (l_time % 3600) / 60, l_time % 60);
	wnd_advance(wnd, wnd->m_width - strlen(time_text) - 1, 
			pl->m_start_pos + pl->m_height);
	wnd_printf(wnd, "%s", time_text);
	col_set_color(wnd, COL_EL_DEFAULT);
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

/* Add an object */
int plist_add_obj( plist_t *pl, char *name, char *title, int where )
{
	char *plugin_name, *obj_name, *str;
	in_plugin_t *inp;
	int num_songs, was_len, i, j;
	song_t **s;

	if (pl == NULL)
		return 0;

	/* Get respective input plugin name */
	str = strchr(name, ':');
	if (str == NULL)
	{
		plugin_name = strdup(name);
		obj_name = strdup("");
	}
	else
	{
		int pos = str - name;
		plugin_name = (char *)malloc(pos + 1);
		memcpy(plugin_name, name, pos);
		plugin_name[pos] = 0;
		obj_name = strdup(&name[pos + 1]);
	}

	/* Search for this plugin */
	inp = pmng_search_inp_by_name(player_pmng, plugin_name);
	if (inp == NULL)
	{
		if (plugin_name != NULL)
			free(plugin_name);
		if (obj_name != NULL)
			free(obj_name);
		return 0;
	}

	/* Initialize songs */
	s = inp_init_obj_songs(inp, obj_name, &num_songs);
	if (s == NULL || !num_songs)
		return 0;

	/* Add these songs to play list */
	plist_lock(pl);
	if (where < 0 || where >= pl->m_len)  
		where = pl->m_len;
	was_len = pl->m_len;
	pl->m_len += num_songs;
	pl->m_list = (song_t **)realloc(pl->m_list, sizeof(song_t *) * pl->m_len);
	memmove(&pl->m_list[where + num_songs], &pl->m_list[where],
			sizeof(song_t *) * (was_len - where));
	memcpy(&pl->m_list[where], s, 
			sizeof(song_t *) * num_songs);
	plist_unlock(pl);
	for ( i = where, j = 0; j < num_songs; i ++, j ++ )
	{
		pl->m_list[i]->m_inp = inp;
		if (title == NULL)
			sat_push(pl, i);
		else
			str_copy_cptr(pl->m_list[i]->m_title, title);
	}

	/* If list was empty - put cursor to the first song */
	if (!was_len)
	{
		pl->m_sel_start = pl->m_sel_end = 0;
		pl->m_visual = FALSE;
	}

	/* Store undo information */
	if (player_store_undo && num_songs)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_ADD_OBJ;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_add_obj.m_num_songs = num_songs;
		undo->m_data.m_add_obj.m_obj_name = strdup(name);
		undo_add(player_ul, undo);
	}

	/* Update screen */
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
	return num_songs;
} /* End of 'plist_add_obj' function */

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
			pl->m_sel_end >= pl->m_scrolled + pl->m_height)
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

/* Set song information */
void plist_set_song_info( plist_t *pl, int index )
{
	if (pl == NULL || index < 0 || index >= pl->m_len)
		return;

	plist_lock(pl);
	song_update_info(pl->m_list[index]);
	pl->m_list[index]->m_flags &= (~SONG_GET_INFO);
	plist_unlock(pl);
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'plist_set_song_info' function */

/* Reload all songs information */
void plist_reload_info( plist_t *pl, bool_t global )
{
	int i;
	
	if (pl == NULL || !pl->m_len)
		return;

	/* Update info */
	if (global)
	{
		for ( i = 0; i < pl->m_len; i ++ )
			sat_push(pl, i);
	}
	else
	{
		int start, end;
		PLIST_GET_SEL(pl, start, end);

		for ( i = start; i <= end; i ++ )
			sat_push(pl, i);
	}
} /* End of 'plist_reload_info' function */

/* Handler for file finder */
int plist_find_handler( char *name, void *data )
{
	return plist_add_one_file((plist_t *)data, name);
} /* End of 'plist_find_handler' function */

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
		if (pl->m_list[i]->m_flags & SONG_SCHEDULE)
		{
			sat_push(pl, i);
			pl->m_list[i]->m_flags &= (~SONG_SCHEDULE);
		}
} /* End of 'plist_flush_scheduled' function */

/* Add a set of files to play list */
bool_t plist_add_set( plist_t *pl, plist_set_t *set )
{
	int i, num = 0;
	struct tag_plist_set_t *node;

	/* Do nothing if set is empty */
	if (pl == NULL || set == NULL)
		return FALSE;

	/* Add each file in the set */
	for ( node = set->m_head; node != NULL; node = node->m_next )
	{
		char fname[MAX_FILE_NAME], *filename = node->m_name;

		util_strncpy(fname, filename, sizeof(fname));
		if (file_get_type(filename) == FILE_TYPE_REGULAR && 
				filename[0] != '/' && filename[0] != '~')
		{
			char wd[MAX_FILE_NAME];
			char fn[MAX_FILE_NAME];
			
			getcwd(wd, sizeof(wd));
			util_strncpy(fn, fname, sizeof(fn));
			snprintf(fname, sizeof(fname), "%s/%s", wd, fn);
		}

		/* Find songs */
		num += find_do(fname, "*", plist_find_handler, pl);
	}

	/* Set info */
	plist_flush_scheduled(pl);
	
	/* Store undo information */
	if (player_store_undo && num)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_ADD;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_add.m_num_songs = num;
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
			plist_sort_bounds(pl, pl->m_len - num, pl->m_len - 1, cr);
		}
	}

	return TRUE;
} /* End of 'plist_add_set' function */

/* Initialize a set of files for adding */
plist_set_t *plist_set_new( void )
{
	plist_set_t *set;

	/* Allocate memory */
	set = (plist_set_t *)malloc(sizeof(plist_set_t));
	if (set == NULL)
		return NULL;
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

	s = plist_set_new();
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

