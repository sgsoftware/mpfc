/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : plist.c
 * PURPOSE     : SG MPFC. Play list manipulation
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.09.2003
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

#include <dirent.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "types.h"
#include "colors.h"
#include "error.h"
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
	int i, num = 0;
	char fname[256];
	struct stat stat_info;

	/* Do nothing if path is empty */
	if (!filename[0])
		return FALSE;

	/* Get full path of filename */
	strcpy(fname, filename);
	if (filename[0] != '/' && filename[0] != '~')
	{
		char wd[256];
		char fn[256];
		
		getcwd(wd, 256);
		strcpy(fn, fname);
		sprintf(fname, "%s/%s", wd, fn);
	}

	/* Check some symbols in path to respective using escapes */
	util_escape_fname(fname, fname);

	/* Determine file type (directory or regular) and make respective
	 * actions */
	stat(fname, &stat_info);
	if (S_ISDIR(stat_info.st_mode))
		num = plist_add_dir(pl, fname);
	else if (S_ISREG(stat_info.st_mode))
		num = plist_add_one_file(pl, fname);

	/* Set info */
	for ( i = 0; i < pl->m_len; i ++ )
		if (pl->m_list[i]->m_flags & SONG_SCHEDULE)
		{
			sat_push(pl, i);
			pl->m_list[i]->m_flags &= (~SONG_SCHEDULE);
		}
	
	/* Store undo information */
	if (player_store_undo && num)
	{
		struct tag_undo_list_item_t *undo;
		undo = (struct tag_undo_list_item_t *)malloc(sizeof(*undo));
		undo->m_type = UNDO_ADD;
		undo->m_next = undo->m_prev = NULL;
		undo->m_data.m_add.m_num_songs = num;
		undo->m_data.m_add.m_file_name = strdup(filename);
		undo_add(player_ul, undo);
	}

	return TRUE;
} /* End of 'plist_add' function */

/* Add single file to play list */
int plist_add_one_file( plist_t *pl, char *filename )
{
	int i;
	char *ext;

	PLIST_ASSERT_RET(pl, FALSE);

	/* Add song */
	if (!strcmp((char *)util_get_ext(filename), "m3u"))
		return plist_add_list(pl, filename);
	else
		return plist_add_song(pl, filename, NULL, 0, -1);
} /* End of 'plist_add_one_file' function */

/* Add a directory to play list */
int plist_add_dir( plist_t *pl, char *filename )
{
	DIR *d;
	struct dirent *de;
	int num = 0, len;
	
	PLIST_ASSERT_RET(pl, 0);

	/* Open directory */
	len = strlen(filename);
	d = opendir(filename);
	if (d == NULL)
		return 0;

	/* Read directory */
	while (de = readdir(d))
	{
		struct stat s;
		char *str;

		/* Skip '.' and '..' */
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		/* Add file or directory */
		str = (char *)malloc(len + strlen(de->d_name) + 2);
		strcpy(str, filename);
		str[len] = '/';
		strcpy(&str[len + 1], de->d_name);
		stat(str, &s);
		if (S_ISDIR(s.st_mode))
			num += plist_add_dir(pl, str);
		else if (S_ISREG(s.st_mode))
			num += plist_add_one_file(pl, str);
		free(str);
	}
	
	/* Close directory */
	closedir(d);
	return num;
} /* End of 'plist_add_dir' function */

/* Add a song to play list */
int plist_add_song( plist_t *pl, char *filename, char *title, int len, 
		int where )
{
	song_t *song;
	int was_len;
	
	PLIST_ASSERT_RET(pl, FALSE);

	/* Lock play list */
	plist_lock(pl);

	/* Try to reallocate memory for play list */
	was_len = pl->m_len;
	if (pl->m_list == NULL)
	{
		pl->m_list = (song_t **)malloc(sizeof(song_t *));
	}
	else
	{
		pl->m_list = (song_t **)realloc(pl->m_list,
				sizeof(song_t *) * (pl->m_len + 1));
	}
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
	FILE *fd;
	char str[256];
	int num = 0;

	PLIST_ASSERT_RET(pl, FALSE);
	
	/* Try to open file */
	fd = util_fopen(filename, "rt");
	if (fd == NULL)
	{
		error_set_code(ERROR_NO_SUCH_FILE);
		return FALSE;
	}

	/* Read file head */
	fgets(str, sizeof(str), fd);
	if (strcmp(str, "#EXTM3U\n"))
	{
		error_set_code(ERROR_UNKNOWN_FILE_TYPE);
		return FALSE;
	}
		
	/* Read file contents */
	while (!feof(fd))
	{
		char len[10], title[80];
		int i, j, song_len;
		
		/* Read song length and title string */
		fgets(str, sizeof(str), fd);
		if (feof(fd))
			break;

		/* Extract song length from string read */
		for ( i = 8, j = 0; str[i] && str[i] != ','; i ++, j ++ )
			len[j] = str[i];
		len[j] = 0;
		if (str[i])
			song_len = atoi(len);

		/* Extract song title from string read */
		strcpy(title, &str[i + 1]);
		title[strlen(title) - 1] = 0;

		/* Read song file name */
		fgets(str, sizeof(str), fd);
		str[strlen(str) - 1] = 0;

		/* Add this song to list */
		num += plist_add_song(pl, str, title, song_len, -1);
	}

	/* Close file */
	fclose(fd);
	return num;
} /* End of 'plist_add_list' function */

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
	if (pl->m_list == NULL)
	{
		pl->m_list = (song_t **)malloc(sizeof(song_t *));
	}
	else
	{
		pl->m_list = (song_t **)realloc(pl->m_list,
				sizeof(song_t *) * (pl->m_len + 1));
	}
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
	FILE *fd;
	int i;

	PLIST_ASSERT_RET(pl, FALSE);
	
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
				pl->m_list[i]->m_title, pl->m_list[i]->m_file_name);
	}

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'plist_save' function */

/* Sort play list */
void plist_sort( plist_t *pl, bool_t global, int criteria )
{
	int start, end, i, j, was_song;
	song_t *cur_song;
	song_t **was_list = NULL;
	
	PLIST_ASSERT(pl);

	/* Lock play list */
	plist_lock(pl);

	/* Save play list */
	was_list = (song_t **)malloc(sizeof(song_t *) * pl->m_len);
	memcpy(was_list, pl->m_list, sizeof(song_t *) * pl->m_len);
	
	/* Get sort start and end */
	if (global)
		start = 0, end = pl->m_len - 1;
	else
		PLIST_GET_SEL(pl, start, end);

	/* Save current song */
	was_song = pl->m_cur_song;
	cur_song = (pl->m_cur_song < 0) ? NULL : pl->m_list[was_song];

	/* Sort */
	for ( i = start; i < end; i ++ )
	{
		int k = i + 1, j;
		song_t *s = pl->m_list[k];
		char *str1, *str2;

		/* Get first string */
		switch (criteria)
		{
		case PLIST_SORT_BY_TITLE:
			str1 = s->m_title;
			break;
		case PLIST_SORT_BY_PATH:
			str1 = s->m_file_name;
			break;
		case PLIST_SORT_BY_NAME:
			str1 = util_get_file_short_name(s->m_file_name);
			break;
		}
		
		for ( j = i; j >= start; j -- )
		{
			/* Get second string */
			switch (criteria)
			{
			case PLIST_SORT_BY_TITLE:
				str2 = pl->m_list[j]->m_title;
				break;
			case PLIST_SORT_BY_PATH:
				str2 = pl->m_list[j]->m_file_name;
				break;
			case PLIST_SORT_BY_NAME:
				str2 = util_get_file_short_name(pl->m_list[j]->m_file_name);
				break;
			}

			/* Save current preferred position */
			if (strcmp(str1, str2) < 0)
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
		player_end_play();
		pl->m_cur_song = -1;
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
			str = s->m_title;
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
			str = song_get_genre_name(s);
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
		found = util_search_regexp(pstr, str);
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
		else if (pl->m_scrolled >= pl->m_len)
			pl->m_scrolled = pl->m_len - 1;
	}

	/* Let selection start follow the end in non-visual mode */
	if (!pl->m_visual)
		pl->m_sel_start = pl->m_sel_end;
} /* End of 'plist_move' function */

/* Centrize view */
void plist_centrize( plist_t *pl )
{
	int index;
	
	PLIST_ASSERT(pl);

	if (!pl->m_len)
		return;

	index = pl->m_cur_song;
	if (index >= 0)
	{
		pl->m_sel_end = index;
		if (!pl->m_visual)
			pl->m_sel_start = pl->m_sel_end;

		pl->m_scrolled = pl->m_sel_end - (pl->m_height + 1) / 2;
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
	wnd_move(wnd, 0, pl->m_start_pos);
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
	/*	if (j >= start && j <= end)
			attrib = A_REVERSE;
		else
			attrib = A_NORMAL;
		if (j == pl->m_cur_song)
			attrib |= A_BOLD;
		wnd_set_attrib(wnd, attrib);*/
		
		/* Print song title or blank line (if we are after end of list) */
		if (j < pl->m_len)
		{
			song_t *s = pl->m_list[j];
			char len[10];
			char title[80];
			int x;
			
			if (strlen(s->m_title) >= wnd->m_width - 10)
			{
				memcpy(title, s->m_title, wnd->m_width - 13);
				strcpy(&title[wnd->m_width - 13], "...");
			}
			else
				strcpy(title, s->m_title);
			wnd_printf(wnd, "%i. %s", j + 1, title);
			sprintf(len, "%i:%02i", s->m_len / 60, s->m_len % 60);
			x = wnd->m_width - strlen(len) - 1;
			while (wnd_getx(wnd) != x)
				wnd_printf(wnd, " ");
			wnd_printf(wnd, "%s\n", len);
		}
		else
			wnd_printf(wnd, "\n");
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
	sprintf(time_text, "%i:%02i:%02i/%i:%02i:%02i",
			s_time / 3600, (s_time % 3600) / 60, s_time % 60,
			l_time / 3600, (l_time % 3600) / 60, l_time % 60);
	wnd_move(wnd, wnd->m_width - strlen(time_text) - 1, wnd_gety(wnd));
	wnd_printf(wnd, "%s\n", time_text);
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
void plist_add_obj( plist_t *pl, char *name )
{
	char plugin_name[256], obj_name[256];
	in_plugin_t *inp;
	int num_songs, was_len, i;
	song_t **s;
	
	if (pl == NULL)
		return;

	/* Get respective input plugin name */
	for ( i = 0; name[i] && name[i] != ':'; i ++ );
	memcpy(plugin_name, name, i);
	plugin_name[i] = 0;

	/* Search for this plugin */
	inp = pmng_search_inp_by_name(plugin_name);
	if (inp == NULL)
		return;

	/* Initialize songs */
	if (name[i])
		strcpy(obj_name, &name[i + 1]);
	else
		strcpy(obj_name, "");
	s = inp_init_obj_songs(inp, obj_name, &num_songs);
	if (s == NULL || !num_songs)
		return;

	/* Add these songs to play list */
	plist_lock(pl);
	was_len = pl->m_len;
	pl->m_len += num_songs;
	if (pl->m_list == NULL)
		pl->m_list = (song_t **)malloc(sizeof(song_t *) * pl->m_len);
	else
		pl->m_list = (song_t **)realloc(pl->m_list, 
				sizeof(song_t *) * pl->m_len);
	memcpy(&pl->m_list[was_len], s, 
			sizeof(song_t *) * num_songs);
	plist_unlock(pl);
	for ( i = was_len; i < pl->m_len; i ++ )
	{
		pl->m_list[i]->m_inp = inp;
		sat_push(pl, i);
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
} /* End of 'plist_add_obj' function */

/* Move selection in play list */
void plist_move_sel( plist_t *pl, int y, bool_t relative )
{
	int start, end, i, j;
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
		for ( i = start, j = 0; i <= end; i ++, j ++ )
		{
			song_t *s = pl->m_list[y + j];
			pl->m_list[y + j] = pl->m_list[i];
			pl->m_list[i] = s;
		}
	}
	else
	{
		for ( i = end, j = end - start; i >= start; i --, j -- )
		{
			song_t *s = pl->m_list[y + j];
			pl->m_list[y + j] = pl->m_list[i];
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

	/* Unlock play list */
	plist_unlock(pl);
} /* End of 'plist_move_sel' function */

/* Set song information */
void plist_set_song_info( plist_t *pl, int index )
{
	if (pl == NULL || index < 0 || index >= pl->m_len)
		return;

	plist_lock(pl);
	song_init_info_and_len(pl->m_list[index]);
	pl->m_list[index]->m_flags &= (~SONG_GET_INFO);
	plist_unlock(pl);
	wnd_send_msg(wnd_root, WND_MSG_DISPLAY, 0);
} /* End of 'plist_set_song_info' function */

/* Reload all songs information */
void plist_reload_info( plist_t *pl )
{
	int i;
	
	if (pl == NULL || !pl->m_len)
		return;

	/* Update info */
	for ( i = 0; i < pl->m_len; i ++ )
		sat_push(pl, i);
} /* End of 'plist_reload_info' function */

/* End of 'plist.c' file */

