/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : cddb.c
 * PURPOSE     : SG MPFC AudioCD input plugin. CDDB support 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 22.11.2003
 * NOTE        : Module prefix 'cddb'.
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
	else if (cddb_read_server(id))
		return TRUE;
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
		n += cddb_sum(toc[i].m_start_min * 60 + toc[i].m_start_sec);
	t = toc[acd_num_tracks - 1].m_end_min * 60 + 
		toc[acd_num_tracks - 1].m_end_sec - 
			(toc[0].m_start_min * 60 + toc[0].m_start_sec);
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
	sprintf(fn, "%s/.mpfc/cddb/%x", getenv("HOME"), id);
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
	acd_print(_("Getting address of %s"), host);
	he = gethostbyname(host);
	if (he == NULL)
		goto close;

	/* Initialize socket and connect */
	acd_print(_("Connecting to %s"), host);
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
	acd_print(_("Sending query to server"));
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	sprintf(buf, "cddb hello %s %s mpfc 1.1\n", getenv("USER"), 
			getenv("HOSTNAME"));
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	sprintf(buf, "cddb query %x %i ", id, acd_num_tracks);
	for ( i = 0; i < acd_num_tracks; i ++ )
		sprintf(&buf[strlen(buf)], "%i ", acd_get_trk_offset(i));
	i --;
	sprintf(&buf[strlen(buf)], "%i\n", acd_get_disc_len());
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (strncmp(buf, "200", 3))
		goto close;
	sscanf(&buf[4], "%s", category);
	sprintf(buf, "cddb read %s %x\n", category, id);
	if (!cddb_server_send(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	close(sockfd);

	/* Convert to CDDB data */
	cddb_server2data(buf);

	/* Save data */
	acd_print(_("Saving data"));
	cddb_save_data(id);
	acd_print(_("Success"));
	return TRUE;
close:
	acd_print(_("Failed!"));
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
void cddb_save_data( dword id )
{
	char fn[MAX_FILE_NAME];
	FILE *fd;
	int i;

	if (cddb_data == NULL)
		return;

	/* Construct file name and open it */
	sprintf(fn, "%s/.mpfc/cddb/%x", getenv("HOME"), id);
	fd = fopen(fn, "wt");
	if (fd == NULL)
		return;

	/* Write data */
	for ( i = 0; i < cddb_data_len; i ++ )
		fprintf(fd, "%s\n", cddb_data[i]);

	/* Close file */
	fclose(fd);
} /* End of 'cddb_save_data' function */

/* Save track info */
void cddb_save_trk_info( int track, song_info_t *info )
{
	dword id;
	int i;

	/* Calculate disc ID */
	id = cddb_id();
	
	/* Create new CDDB data */
	if (cddb_data == NULL)
	{
		char str[256];

		sprintf(str, "# xcmd");
		cddb_data_add(str, -1);
		sprintf(str, "#");
		cddb_data_add(str, -1);
		sprintf(str, "# Track frame offsets:");
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			sprintf(str, "#        %i", acd_get_trk_offset(i));
			cddb_data_add(str, -1);
		}
		sprintf(str, "#");
		cddb_data_add(str, -1);
		sprintf(str, "# Disc length: %i seconds", acd_get_disc_len());
		cddb_data_add(str, -1);
		sprintf(str, "#");
		cddb_data_add(str, -1);
		sprintf(str, "DISCID=%x", id);
		cddb_data_add(str, -1);
		sprintf(str, "DTITLE=%s / %s", info->m_artist, info->m_album);
		cddb_data_add(str, -1);
		sprintf(str, "DYEAR=%s", info->m_year);
		cddb_data_add(str, -1);
		sprintf(str, "DGENRE=%s", info->m_genre);
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			sprintf(str, "TTITLE%i=%s", i, (i == track) ? info->m_name : "");
			cddb_data_add(str, -1);
		}
		sprintf(str, "EXTD=");
		cddb_data_add(str, -1);
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			sprintf(str, "EXTT%i=", i);
			cddb_data_add(str, -1);
		}
		sprintf(str, "PLAYORDER=");
		cddb_data_add(str, -1);
	}
	/* Or update existing */
	else
	{
		char str[256];
		char title[80];
		bool_t dtitle = FALSE, dgenre = FALSE, dyear = FALSE, ttitle = FALSE;
			
		sprintf(title, "TTITLE%i=", track);
		for ( i = 0; i < cddb_data_len; i ++ )
		{
			if (!strncmp(cddb_data[i], "DTITLE=", 7))
			{
				free(cddb_data[i]);
				sprintf(str, "DTITLE=%s / %s", info->m_artist, info->m_album);
				cddb_data[i] = strdup(str);
				dtitle = TRUE;
			}
			else if (!strncmp(cddb_data[i], "DGENRE=", 7))
			{
				free(cddb_data[i]);
				sprintf(str, "DGENRE=%s", info->m_genre);
				cddb_data[i] = strdup(str);
				dgenre = TRUE;
			}
			else if (!strncmp(cddb_data[i], "DYEAR=", 6))
			{
				free(cddb_data[i]);
				sprintf(str, "DYEAR=%s", info->m_year);
				cddb_data[i] = strdup(str);
				dyear = TRUE;
			}
			else if (!strncmp(cddb_data[i], title, strlen(title)))
			{
				free(cddb_data[i]);
				sprintf(str, "%s%s", title, info->m_name);
				cddb_data[i] = strdup(str);
				ttitle = TRUE;
			}
		}

		/* If not found any of important fields - insert them */
		if (!dtitle)
		{
			sprintf(str, "DTITLE=%s / %s", info->m_artist, info->m_album);
			cddb_data_add(str, -1);
		}
		if (!dgenre)
		{
			sprintf(str, "DGENRE=%s", info->m_genre);
			cddb_data_add(str, -1);
		}
		if (!dyear)
		{
			sprintf(str, "DYEAR=%s", info->m_year);
			cddb_data_add(str, -1);
		}
		if (!ttitle)
		{
			sprintf(str, "%s%s", title, info->m_name);
			cddb_data_add(str, -1);
		}
	}

	/* Save data */
	cddb_save_data(id);
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
		acd_print_msg(_("CDDB submit error: no existing info found"));
		return;
	}
	
	/* Check email and category (set through variables) */
	email = cfg_get_var(pmng_get_cfg(acd_pmng), "cddb-email");
	if (strlen(email) <= 1)
	{
		acd_print_msg(_("CDDB submit error: you must specify your email "
					"address"));
		return;
	}
	category = cfg_get_var(pmng_get_cfg(acd_pmng), "cddb-category");
	if (!cddb_valid_category(category))
	{
		acd_print_msg(_("CDDB submit error: you must specify your category"));
		return;
	}

	/* Get host name */
	cddb_get_host_name(host);

	/* Get host address */
	acd_print(_("Getting address of %s"), host);
	he = gethostbyname(host);
	if (he == NULL)
		goto close;

	/* Initialize socket and connect */
	acd_print(_("Connecting to %s"), host);
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
	acd_print(_("Posting data to server"));
	post_str = cddb_make_post_string(email, category);
	if (post_str == NULL)
		goto close;
	if (!cddb_server_send(sockfd, post_str, strlen(post_str)))
	{
		free(post_str);
		goto close;
	}

	/* Get response */
	acd_print(_("Getting response"));
	if (!cddb_server_recv(sockfd, buf, CDDB_BUF_SIZE - 1))
		goto close;
	close(sockfd);
	acd_print("%s", buf);
	return;
	
close:
	acd_print(_("Failure!"));
	if (sockfd >= 0)
		close(sockfd);
} /* End of 'cddb_submit' function */

/* Get host name */
void cddb_get_host_name( char *name )
{
	char *host = cfg_get_var(pmng_get_cfg(acd_pmng), "cddb-host");
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
	sprintf(buf, "POST /~cddb/submit.cgi HTTP/1.0\r\nCategory: %s\r\n"
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
	return (!strcmp(cat, "blues") || !strcmp(cat, "classical") || 
			!strcmp(cat, "country") || !strcmp(cat, "data") ||
			!strcmp(cat, "folk") || !strcmp(cat, "jazz") ||
			!strcmp(cat, "misc") || !strcmp(cat, "newage") ||
			!strcmp(cat, "reggae") || !strcmp(cat, "rock") ||
			!strcmp(cat, "soundtrack"));
} /* End of 'cdbd_valid_category' function */

/* End of 'cddb.c' file */

