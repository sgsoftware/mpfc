/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : http.c
 * PURPOSE     : SG MPFC. File library http files managament 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 25.10.2003
 * NOTE        : Module prefix 'fhttp'.
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
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "file.h"
#include "http.h"

/* Get file data */
#define FHTTP_GET_DATA(data, file) \
	file_http_data_t *(data) = (file_http_data_t *)((file)->m_data)

/* Open a file */
file_t *fhttp_open( file_t *f, char *mode )
{
	int i;
	file_http_data_t *data;
	struct hostent *he;
	struct sockaddr_in their_addr;
	char str[1024];
	char *header = NULL, *ph = NULL;
	int n, end, hs, code;
	char *name, *host_name, *file_name;
	int port;
	int read_size;

	/* Allocate memory for additional data */
	f->m_data = (void *)malloc(sizeof(*data));
	data = (file_http_data_t *)(f->m_data);
	memset(data, 0, sizeof(*data));
	data->m_tid = -1;
	data->m_sock = -1;

	/* Initialize buffer */
	data->m_real_size = 1048576;
	data->m_buf_size = data->m_real_size >> 1;
	data->m_buf = (byte *)malloc(data->m_real_size);
	data->m_read_size = 0;
	data->m_portion_size = 1024;
	data->m_actual_ptr = data->m_buf;
	data->m_min_buf_size = 0;
	data->m_file_pos = 0;

	/* Connect to host and get file header */
	for ( name = strdup(f->m_name);; ) 
	{
		/* Parse URL */
		fhttp_parse_url(name, &host_name, &file_name, &port);

		/* Get host address */
		file_print_msg(f, _("Getting address of host %s"), host_name);
		he = gethostbyname(host_name);
		if (he == NULL)
		{
			goto close;
		}
		file_print_msg(f, _("OK"));

		/* Initialize socket and connect */
		data->m_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (data->m_sock < 0)
			goto close;
		their_addr.sin_family = AF_INET;
		their_addr.sin_port = htons(port);
		their_addr.sin_addr = *((struct in_addr *)he->h_addr);
		memset(&(their_addr.sin_zero), 0, 8);
		file_print_msg(f, _("Connecting to %s"), host_name);
		if (connect(data->m_sock, (struct sockaddr *)&their_addr, 
					sizeof(struct sockaddr)) < 0)
			goto close;
		file_print_msg(f, _("OK"));

		/* Send request for file we need */
		file_print_msg(f, _("Sending request for file %s"), file_name);
		sprintf(str, "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: mpfc/1.0"
				"\r\n\r\n", file_name, host_name);
		if (send(data->m_sock, str, strlen(str) + 1, 0) < 0)
			goto close;

		/* Get HTTP header */
		file_print_msg(f, _("Getting HTTP header"));
		ph = header = (char *)malloc(hs = data->m_portion_size);
		for ( read_size = 0;; )
		{
			/* Receive portion of data and break if header is complete */
			n = recv(data->m_sock, ph, data->m_portion_size - read_size, 0);
			read_size += n;
			ph += n;
			if (n <= 0 || fhttp_header_complete(header, read_size) >= 0)
				break;

			/* Reallocate memory */
			if (read_size >= hs)
			{
				hs += data->m_portion_size;
				header = (char *)realloc(header, hs);
				ph = header + hs - data->m_portion_size;
			}
		}

		/* If last recv received nothing and header is not complete - error */
		end = fhttp_header_complete(header, read_size);
		if (n <= 0 && end < 0)
			goto close;
		
		/* Analyze header */
		code = fhttp_get_return(header, read_size);
		if (code == 200)
		{
			char *str;
			
			/* Save some parameters */
			free(name);
			data->m_file_name = file_name;
			data->m_host_name = host_name;
			str = fhttp_get_field(header, read_size, "Content-Type");
			if (str != NULL)
				data->m_content_type = strdup(str);
			else
				data->m_content_type = NULL;

			/* Copy rest of data to buffer */
			data->m_read_size = read_size - end - 1;
			memcpy(data->m_buf, &header[end + 1], data->m_read_size);
			free(header);
			file_print_msg(f, _("OK"));
			break;
		}
		else if (code / 100 == 3)
		{
			char *str;

			free(name);
			free(host_name);
			free(file_name);
			str = fhttp_get_field(header, hs, "Location");
			if (str == NULL)
				name = strdup(str);
			else
				name = strdup("");
			free(header);
			file_print_msg(f, _("Redirect to %s"), name);
			continue;
		}
close:
		if (header != NULL)
			free(header);
		free(name);
		free(file_name);
		free(host_name);
		file_print_msg(f, _("Failure!"));
		file_close(f);
		return NULL;
	}

	/* Create a mutex for synchronizing buffer operations */
	pthread_mutex_init(&data->m_mutex, NULL);

	/* Create a thread that will be receive data */
	data->m_end_thread = FALSE;
	data->m_finished = FALSE;
	data->m_eof = FALSE;
	pthread_create(&data->m_tid, NULL, fhttp_thread, f);
	return f;
} /* End of 'fhttp_open' function */

/* Close file */
int fhttp_close( file_t *f )
{
	FHTTP_GET_DATA(data, f);

	if (data != NULL)
	{
		if (data->m_tid >= 0)
		{
			data->m_end_thread = TRUE;
			pthread_join(data->m_tid, NULL);
		}
		pthread_mutex_destroy(&data->m_mutex);
		if (data->m_content_type != NULL)
			free(data->m_content_type);
		if (data->m_buf != NULL)
			free(data->m_buf);
		if (data->m_host_name != NULL)
			free(data->m_host_name);
		if (data->m_file_name != NULL)
			free(data->m_file_name);
		if (data->m_sock >= 0)
			close(data->m_sock);
		free(data);
	}
	return 0;
} /* End of 'fhttp_close' function */

/* Read from file */
size_t fhttp_read( void *buf, size_t size, size_t nmemb, file_t *f )
{
	int s;
	
	FHTTP_GET_DATA(data, f);

	/* No data available */
	size *= nmemb;
	if (!data->m_min_buf_size)
	{
		while (!data->m_read_size && !data->m_finished)
			util_delay(1000000);
	}
	else if (data->m_read_size <= (data->m_min_buf_size >> 2))
	{
		int pp;
		
		/* Data is over */
		if (data->m_finished)
			return 0;
		
		/* Wait */
		pp = data->m_read_size * 100 / data->m_min_buf_size;
		file_print_msg(f, _("Filling buffer: %d%% done"), pp);
		while (data->m_read_size <= data->m_min_buf_size && !data->m_finished)
		{
			int p = data->m_read_size * 100 / data->m_min_buf_size;
			if ((p / 10) != (pp / 10))
				file_print_msg(f, _("Filling buffer: %d%% done"), p);
			pp = p;
			util_delay(1000000);
		}
	}

	/* Lock mutex */
	pthread_mutex_lock(&data->m_mutex);

	/* Read data */
	s = size < data->m_read_size ? size : data->m_read_size;
	memcpy(buf, data->m_actual_ptr, s);
	data->m_actual_ptr += s;
	data->m_read_size -= s;
	if (data->m_actual_ptr - data->m_buf >= data->m_buf_size)
	{
		memmove(data->m_buf, data->m_actual_ptr, data->m_read_size);
		data->m_actual_ptr = data->m_buf;
	}

	/* Unlock mutex */
	pthread_mutex_unlock(&data->m_mutex);
	data->m_file_pos += s;
	return s;
} /* End of 'fhttp_read' function */

/* Write to file */
size_t fhttp_write( void *buf, size_t size, size_t nmemb, file_t *f )
{
	return 0;
} /* End of 'fhttp_write' function */

/* Seek file */
int fhttp_seek( file_t *f, long offset, int whence )
{
	fhttp_close(f);
	fhttp_open(f, "");
	return offset;
} /* End of 'fhttp_seek' function */

/* Tell file position */
long fhttp_tell( file_t *f )
{
	FHTTP_GET_DATA(data, f);
	return data->m_file_pos;
} /* End of 'fhttp_tell' function */

/* Thread function */
void *fhttp_thread( void *arg )
{
	file_t *f = (file_t *)arg;
	FHTTP_GET_DATA(data, f);
	int n;

	for ( ; !data->m_end_thread; )
	{
		/* Lock buffer */
		pthread_mutex_lock(&data->m_mutex);

		/* Determine amount of data we must read */
		n = data->m_buf_size - data->m_read_size;
		if (n > data->m_portion_size)
			n = data->m_portion_size;
		if (!n)
		{
			pthread_mutex_unlock(&data->m_mutex);
			util_delay(0, 1000000);
			continue;
		}

		/* Read socket */
		n = recv(data->m_sock, &data->m_actual_ptr[data->m_read_size], n, 0);
		data->m_read_size += n;
		pthread_mutex_unlock(&data->m_mutex);
		if (n <= 0)
			break;
	}
	data->m_tid = -1;
	data->m_finished = TRUE;
	return NULL;
} /* End of 'fhttp_thread' function */

/* Check if HTTP header is complete and return its end offset */
int fhttp_header_complete( char *header, int size )
{
	int i;

	for ( i = 3; i < size; i ++ )
		if (header[i - 3] == '\r' && header[i - 2] == '\n' &&
				header[i - 1] == '\r' && header[i] == '\n')
			return i;
	return -1;
} /* End of 'fhttp_header_complete' function */

/* Set minimal buffer size */
void fhttp_set_min_buf_size( file_t *f, int size )
{
	FHTTP_GET_DATA(data, f);
	data->m_min_buf_size = size;
} /* End of 'fhttp_set_min_buf_size' function */

/* Get content type */
char *fhttp_get_content_type( file_t *f )
{
	FHTTP_GET_DATA(data, f);
	return data->m_content_type;
} /* End of 'fhttp_get_content_type' function */

/* Parse URL */
void fhttp_parse_url( char *url, char **host_name, char **file_name, 
		int *port )
{
	int i, port_delim, name_delim;

	/* Skip 'http://' */
	url += 7;

	/* Find first ':' and first '/' */
	for ( i = 0, port_delim = name_delim = -1; url[i]; i ++ )
	{
		if (url[i] == ':' && port_delim < 0)
			port_delim = i;
		else if (url[i] == '/' && name_delim < 0)
			name_delim = i;
	}

	/* Get port number */
	if (port_delim >= 0)
		*port = atoi(&url[port_delim + 1]);
	else
	{
		*port = 80;
		port_delim = (name_delim < 0) ? i : name_delim;
	}

	/* Get host and file names */
	*host_name = strndup(url, port_delim);
	*file_name = strdup(name_delim < 0 ? "" : &url[name_delim + 1]);
} /* End of 'fhttp_parse_url' function */

/* Get HTTP response return value */
int fhttp_get_return( char *header, int size )
{
	char ret[4], *s;

	s = strchr(header, ' ');
	memcpy(ret, s + 1, 3);
	ret[3] = 0;
	return atoi(ret);
} /* End of 'fhttp_get_return' function */

/* Get HTTP header field */
char *fhttp_get_field( char *header, int size, char *field )
{
	char str[256], *s1, *s2;
	int len;

	len = sprintf(str, "\r\n%s: ", field);
	s1 = strstr(header, str);
	if (s1 == NULL)
	{
		len = sprintf(str, "\r\n%s:", field);
		s1 = strstr(header, str);
		if (s1 == NULL)
			return NULL;
	}
	s2 = strchr(s1 + 1, '\r');
	if (s2 == NULL)
		return NULL;
	return strndup(s1 + len, s2 - s1 - len);
} /* End of 'fhttp_get_field' function */

/* Get line from file */
char *fhttp_gets( char *s, int size, file_t *f )
{
	int i;
	FHTTP_GET_DATA(data, f);

	/* Read file until reach new line or end of file */
	for ( i = 0; i < size - 1; i ++ )
	{
		int j = file_read(&s[i], 1, 1, f);
		if (!j || !s[i] || s[i] == '\n')
		{
			if (!j)
				i --;
			break;
		}
	}
	s[i + 1] = 0;
	if (!(*s))
		data->m_eof = TRUE;
	return s;
} /* End of 'fhttp_gets' function */

/* Check for end of file */
bool_t fhttp_eof( file_t *f )
{
	FHTTP_GET_DATA(data, f);
	return data->m_eof;
} /* End of 'fhttp_eof' function */

/* End of 'http.c' file */

