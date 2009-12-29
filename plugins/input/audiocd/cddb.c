/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC AudioCD input plugin. CDDB support functions 
 * implementation.
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
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "types.h"
#include "audiocd.h"
#include "cddb.h"
#include "genre_list.h"
#include "song_info.h"

/* Maximal line length */
#define CDDB_MAX_LINE_LEN 256

#define CDDB_BUF_SIZE 65536
#define CDDB_PORT 8880

/* Data */
static char **cddb_data = NULL;
static int cddb_data_len = 0;
static bool_t cddb_server_found = TRUE;

/* Categories list */
char *cddb_cats[] = { "blues", "classical", "country", "data", "folk",
						"jazz", "misc", "newage", "reggae", "rock",
						"soundtrack" };
int cddb_num_cats = sizeof(cddb_cats) / sizeof(cddb_cats[0]);

/* Initialize CDDB entry for currently playing disc */
bool_t cddb_read( void )
{
	dword id;

	/* Skip if already read */
	if (cddb_data != NULL)
		return TRUE;
	
	/* Calculate disc ID */
	id = cddb_id();

	/* Search for entry on local machine */
	if (cddb_read_local(id))
		return TRUE;
//	else if (cddb_read_server(id))
//		return TRUE;
	return FALSE;
} /* End of 'cddb_read' function */

/* Fill song info for specified track */
song_info_t *cddb_get_trk_info( int track )
{
	int i;
	song_info_t *si;
	char tr[10];

	if (cddb_data == NULL)
		return si_new();

	si = si_new();
	for ( i = 0; i < cddb_data_len; i ++ )
	{
		char *str = cddb_data[i];
		int j;
		
		if (!strncmp(str, "DTITLE", 6))
		{
			for ( j = 0; str[j] && str[j] != '/'; j ++ );
			if (str[j] == '/')
			{
				si_set_artist(si, &str[7]);
				si->m_artist[j - 8] = 0;
			}
			else
			{
				si_set_artist(si, &str[7]);
				continue;
			}
			si_set_album(si, &str[j + 2]);
		}	
		else if (!strncmp(str, "DYEAR", 5))
		{
			si_set_year(si, &str[6]);
		}
		else if (!strncmp(str, "DGENRE", 6))
		{
			si_set_genre(si, &str[7]);
		}
		else if (!strncmp(str, "TTITLE", 6))
		{
			char t[10];
			int k;
			
			for ( j = 6, t[k = 0] = 0; 
					str[j] && str[j] >= '0' && str[j] <= '9'; 
					t[k ++] = str[j], j ++ );
			t[k] = 0;
			if (track == atoi(t))
				si_set_name(si, &str[j + 1]);
		}
	}
	sprintf(tr, "%i", track + 1);
	si_set_track(si, tr);
	return si;
} /* End of 'cddb_get_trk_info' function */

/* Calculate disc ID */
dword cddb_id( void )
{
	int i, t, n;
	struct acd_trk_info_t *toc = acd_tracks_info;

	for ( i = 0, n = 0; i < acd_num_tracks; i ++ )
		n += cddb_sum(toc[i].m_start.minute * 60 + toc[i].m_start.second);
	t = toc[acd_num_tracks - 1].m_end.minute * 60 + 
		toc[acd_num_tracks - 1].m_end.second - 
			(toc[0].m_start.minute * 60 + toc[0].m_start.second);
	return ((n % 0xFF) << 24 | t << 8 | acd_num_tracks);
} /* End of 'cddb_id' function */

/* Find sum for disc ID */
int cddb_sum( int n )
{
	int ret = 0;

	while (n > 0)
	{
		ret += (n % 10);
		n /= 10;
	}
	return ret;
} /* End of 'cddb_sum' function */

/* Search for CDDB entry on local machine */
bool_t cddb_read_local( dword id )
{
	char fn[MAX_FILE_NAME];
	FILE *fd;

	/* Construct file name and open it */
	snprintf(fn, sizeof(fn), "%s/.mpfc/cddb/%x", getenv("HOME"), id);
	fd = fopen(fn, "rt");
	if (fd == NULL)
		return FALSE;

	/* Read data */
	cddb_data_len = 0;
	while (!feof(fd))
	{
		char s[256];
		
		/* Reallocate memory for data */
		if (cddb_data == NULL)
			cddb_data = (char **)malloc(sizeof(char *) * (cddb_data_len + 1));
		else
			cddb_data = (char **)realloc(cddb_data, 
					sizeof(char *) * (cddb_data_len + 1));

		/* Read line */
		fgets(s, CDDB_MAX_LINE_LEN, fd);
		if (feof(fd))
			break;
		while (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')
			s[strlen(s) - 1] = 0;
		cddb_data[cddb_data_len ++] = strdup(s);
	}

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'cddb_read_local' function */

/* Search for CDDB entry on server */
bool_t cddb_read_server( dword id )
{
	int sockfd = -1, i;
	char buf[CDDB_BUF_SIZE];
	struct hostent *he;
	struct sockaddr_in their_addr;
	char host[MAX_FILE_NAME];
	char category[80];

	if (!cddb_server_found)
		return FALSE;

	/* Get host name */
	cddb_get_host_name(host);

	/* Get host address */
	logger_message(acd_log, 1, _("Getting address of %s"), host);
	he = gethostbyname(host);
	if (he == NULL)
		goto close;

	/* Initialize socket and connect */
	logger_message(acd_log, 1, _("Connecting to %s"), host);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		goto close;
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(CDDB_PORT);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), 0, 8);
	if (connect(sockfd, (struct sockaddr *)&their_addr, 
				sizeof(struct sockaddr)) < 0)
		goto close;

	/* Communicate with server */
	logger_message(acd_log, 1, _("Sending query to server"));
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	snprintf(buf, sizeof(buf), "cddb hello %s %s mpfc 1.1\n", getenv("USER"), 
			getenv("HOSTNAME"));
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	snprintf(buf, sizeof(buf), "cddb query %x %i ", id, acd_num_tracks);
	for ( i = 0; i < acd_num_tracks; i ++ )
		snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf), 
				"%i ", acd_get_trk_offset(i));
	i --;
	snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf),
			"%i\n", acd_get_disc_len());
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (strncmp(buf, "200", 3))
		goto close;
	sscanf(&buf[4], "%s", category);
	snprintf(buf, sizeof(buf), "cddb read %s %x\n", category, id);
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	close(sockfd);

	/* Convert to CDDB data */
	cddb_server2data(buf);

	/* Save data */
	logger_message(acd_log, 1, _("Saving data"));
	cddb_save_data(id);
	logger_message(acd_log, 1, _("Success"));
	return TRUE;
close:
	logger_error(acd_log, 1, _("Failed!"));
	cddb_server_found = FALSE;
	if (sockfd >= 0)
		close(sockfd);
	return FALSE;
} /* End of 'cddb_read_server' function */

/* Free stuff */
void cddb_free( void )
{
	if (cddb_data != NULL)
	{
		int i;
		
		for ( i = 0; i < cddb_data_len; i ++ )
			free(cddb_data[i]);
		free(cddb_data);
		cddb_data = NULL;
	}
	cddb_data_len = 0;
	cddb_server_found = TRUE;
} /* End of 'cddb_free' function */

/* Send data to CDDB server */
bool_t cddb_server_send( int fd, char *buf, int size )
{
	int n;

	size = strlen(buf);
	n = send(fd, buf, size, 0);
	if (n < 0)
		return FALSE;
	return TRUE;
} /* End of 'cddb_server_send' function */

/* Receice data from CDDB server */
bool_t cddb_server_recv( int fd, char *buf, int size )
{
	int n;

	n = recv(fd, buf, size, 0);
	if (n < 0)
		return FALSE;
	buf[n] = 0;
	if (*buf != '2')
		return FALSE;
	return TRUE;
} /* End of 'cddb_server_recv' function */

/* Convert data received from server to our format */
void cddb_server2data( char *buf )
{
	char str[256];
	int i;
	bool_t start = FALSE;

	/* Free at first */
	cddb_free();

	/* Add each string */
	for ( str[i = 0] = 0; *buf; buf ++ )
	{
		/* End of line */
		if (*buf == '\n')
		{
			if (str[0] == '#')
				start = TRUE;
			if (start)
			{
				if (cddb_data == NULL)
					cddb_data = (char **)malloc(sizeof(char *) * 
							(cddb_data_len + 1));
				else
					cddb_data = (char **)realloc(cddb_data, sizeof(char *) * 
							(cddb_data_len + 1));
				cddb_data[cddb_data_len ++] = strdup(str);
			}
			str[i = 0] = 0;
		}
		/* Copy next symbol */
		else
		{
			if (*buf != '\r')
			{
				str[i] = *buf;
				str[++i] = 0;
			}
		}
	}
} /* End of 'cddb_server2data' function */

/* Save CDDB data */
bool_t cddb_save_data( dword id )
{
	char fn[MAX_FILE_NAME];
	FILE *fd;
	int i;

	if (cddb_data == NULL)
		return TRUE;

	/* Construct file name and open it */
	snprintf(fn, sizeof(fn), "%s/.mpfc/cddb/%x", getenv("HOME"), id);
	fd = fopen(fn, "wt");
	if (fd == NULL)
	{
		logger_error(acd_log, 1, _("Unable to open file %s for writing"), fn);
		return FALSE;
	}

	/* Write data */
	for ( i = 0; i < cddb_data_len; i ++ )
		fprintf(fd, "%s\n", cddb_data[i]);

	/* Close file */
	fclose(fd);
	return TRUE;
} /* End of 'cddb_save_data' function */

/* Save track info */
bool_t cddb_save_trk_info( int track, song_info_t *info )
{
	dword id;
	int i;

	/* Calculate disc ID */
	id = cddb_id();
	
	/* Create new CDDB data */
	if (cddb_data == NULL)
	{
		char str[256];

		snprintf(str, sizeof(str), "# xcmd");
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "#");
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "# Track frame offsets:");
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			snprintf(str, sizeof(str), "#        %i", acd_get_trk_offset(i));
			cddb_data_add(str, -1);
		}
		snprintf(str, sizeof(str), "#");
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), 
				"# Disc length: %i seconds", acd_get_disc_len());
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "#");
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "DISCID=%x", id);
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), 
				"DTITLE=%s / %s", info->m_artist, info->m_album);
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "DYEAR=%s", info->m_year);
		cddb_data_add(str, -1);
		snprintf(str, sizeof(str), "DGENRE=%s", info->m_genre);
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			snprintf(str, sizeof(str), 
					"TTITLE%i=%s", i, (i == track) ? info->m_name : "");
			cddb_data_add(str, -1);
		}
		snprintf(str, sizeof(str), "EXTD=");
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			snprintf(str, sizeof(str), "EXTT%i=", i);
			cddb_data_add(str, -1);
		}
		snprintf(str, sizeof(str), "PLAYORDER=");
		cddb_data_add(str, -1);
	}
	/* Or update existing */
	else
	{
		char str[256];
		char title[80];
		bool_t dtitle = FALSE, dgenre = FALSE, dyear = FALSE, ttitle = FALSE;
			
		snprintf(title, sizeof(title), "TTITLE%i=", track);
		for ( i = 0; i < cddb_data_len; i ++ )
		{
			if (!strncmp(cddb_data[i], "DTITLE=", 7))
			{
				free(cddb_data[i]);
				snprintf(str, sizeof(str),
						"DTITLE=%s / %s", info->m_artist, info->m_album);
				cddb_data[i] = strdup(str);
				dtitle = TRUE;
			}
			else if (!strncmp(cddb_data[i], "DGENRE=", 7))
			{
				free(cddb_data[i]);
				snprintf(str, sizeof(str), "DGENRE=%s", info->m_genre);
				cddb_data[i] = strdup(str);
				dgenre = TRUE;
			}
			else if (!strncmp(cddb_data[i], "DYEAR=", 6))
			{
				free(cddb_data[i]);
				snprintf(str, sizeof(str), "DYEAR=%s", info->m_year);
				cddb_data[i] = strdup(str);
				dyear = TRUE;
			}
			else if (!strncmp(cddb_data[i], title, strlen(title)))
			{
				free(cddb_data[i]);
				snprintf(str, sizeof(str), "%s%s", title, info->m_name);
				cddb_data[i] = strdup(str);
				ttitle = TRUE;
			}
		}

		/* If not found any of important fields - insert them */
		if (!dtitle)
		{
			snprintf(str, sizeof(str),
					"DTITLE=%s / %s", info->m_artist, info->m_album);
			cddb_data_add(str, -1);
		}
		if (!dgenre)
		{
			snprintf(str, sizeof(str), "DGENRE=%s", info->m_genre);
			cddb_data_add(str, -1);
		}
		if (!dyear)
		{
			snprintf(str, sizeof(str), "DYEAR=%s", info->m_year);
			cddb_data_add(str, -1);
		}
		if (!ttitle)
		{
			snprintf(str, sizeof(str), "%s%s", title, info->m_name);
			cddb_data_add(str, -1);
		}
	}

	/* Save data */
	return cddb_save_data(id);
} /* End of 'cddb_save_trk_info' function */

/* Add a string to data */
void cddb_data_add( char *str, int index )
{
	/* Reallocate memory for data */
	cddb_data = (char **)realloc(cddb_data, 
				sizeof(char *) * (cddb_data_len + 1));

	/* Add string */
	cddb_data[cddb_data_len ++] = strdup(str);
} /* End of 'cddb_data_add' function */

/* Reload info */
void cddb_reload( char *filename )
{
	dword id;
	
	/* Free existing info */
	cddb_server_found = TRUE;
	free(cddb_data);
	cddb_data = NULL;

	/* Calculate disc ID */
	id = cddb_id();

	/* Read data from server */
	cddb_read_server(id);
} /* End of 'cddb_reload' function */

/* Submit info to server */
void cddb_submit( char *filename )
{	
	char *email, *category;
	int sockfd = -1, i;
	char buf[CDDB_BUF_SIZE];
	struct hostent *he;
	struct sockaddr_in their_addr;
	char host[MAX_FILE_NAME];
	char *post_str;
	
	/* Check data */
	if (cddb_data == NULL)
	{
		logger_error(acd_log, 1,
				_("CDDB submit error: no existing info found"));
		return;
	}
	
	/* Check email and category (set through variables) */
	email = cfg_get_var(acd_cfg, "cddb-email");
	if (strlen(email) <= 1)
	{
		logger_error(acd_log, 1,
				_("CDDB submit error: you must specify your email address"));
		return;
	}
	category = cfg_get_var(acd_cfg, "cddb-category");
	if (!cddb_valid_category(category))
	{
		logger_error(acd_log, 1,
				_("CDDB submit error: you must specify your category"));
		return;
	}

	/* Get host name */
	cddb_get_host_name(host);

	/* Get host address */
	logger_message(acd_log, 1, _("Getting address of %s"), host);
	he = gethostbyname(host);
	if (he == NULL)
		goto close;

	/* Initialize socket and connect */
	logger_message(acd_log, 1, _("Connecting to %s"), host);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		goto close;
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(CDDB_PORT);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), 0, 8);
	if (connect(sockfd, (struct sockaddr *)&their_addr, 
				sizeof(struct sockaddr)) < 0)
		goto close;

	/* Communicate with server */
	logger_message(acd_log, 1, _("Posting data to server"));
	post_str = cddb_make_post_string(email, category);
	if (post_str == NULL)
		goto close;
	if (!cddb_server_send(sockfd, post_str, strlen(post_str)))
	{
		free(post_str);
		goto close;
	}

	/* Get response */
	logger_message(acd_log, 1, _("Getting response"));
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	close(sockfd);
	logger_message(acd_log, 1, "%s", buf);
	return;
	
close:
	logger_message(acd_log, 1, _("Failure!"));
	if (sockfd >= 0)
		close(sockfd);
} /* End of 'cddb_submit' function */

/* Get host name */
void cddb_get_host_name( char *name )
{
	char *host = cfg_get_var(acd_cfg, "cddb-host");
	if (host != NULL)
		strcpy(name, host);
	else
		strcpy(name, "freedb.freedb.org");
} /* End of 'cddb_get_host_name' function */

/* Create string to submit to CDDB */
char *cddb_make_post_string( char *email, char *category )
{
	char *buf;
	dword id;
	int i, size = 0;

	/* Calculate disc id */
	id = cddb_id();

	/* Get needed buffer size */
	for ( i = 0; i < cddb_data_len; i ++ )
		if (cddb_data[i] != NULL)
			size += strlen(cddb_data[i]) + 2; 

	/* Allocate memory */
	buf = (char *)malloc(size + 1024);
	if (buf == NULL)
		return NULL;

	/* Fill buffer */
	snprintf(buf, sizeof(buf),
			"POST /~cddb/submit.cgi HTTP/1.0\r\nCategory: %s\r\n"
			"Discid: %x\r\nUser-Mail: %s\r\nSubmut-Mode: submit\r\n"
			"Charset: ISO-8859-1\r\n"
			"X-Cddbd-Note: Sent by MPFC\r\n"
			"Content-Length: %d\r\n\r\n",
			category, id, email, size);
	for ( i = 0; i < cddb_data_len; i ++ )
	{
		int last;

		strcat(buf, cddb_data[i]);
		last = strlen(buf);
		buf[last] = '\r';
		buf[last + 1] = '\n';
		buf[last + 2] = 0;
	}
	return buf;
} /* End of 'cddb_make_post_string' function */

/* Check if specified category is valid */
bool_t cddb_valid_category( char *cat )
{
	int i;
	for ( i = 0; i < cddb_num_cats; i ++ )
	{
		if (!strcmp(cat, cddb_cats[i]))
			return TRUE;
	}
	return FALSE;
} /* End of 'cdbd_valid_category' function */

/* End of 'cddb.c' file */

