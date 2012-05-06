/******************************************************************
 * Copyright (C) 2011 by SG Software.
 *
 * SG MPFC. Abstraction for a reader with external notification.
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

#include <stdlib.h>
#include <unistd.h>
#include "types.h"
#include "rd_with_notify.h"

/* Create a new object */
rd_with_notify_t *rd_with_notify_new( int fd )
{
	/* Allocate memory */
	rd_with_notify_t *rdwn = (rd_with_notify_t *)malloc(sizeof(rd_with_notify_t));
	if (!rdwn)
		return NULL;
	rdwn->m_fd = fd;

	/* Create the external notification pipe */
	if (pipe(rdwn->m_notify_pipe) == -1)
	{
		free(rdwn);
		return NULL;
	}

	return rdwn;
} /* End of 'rd_with_notify_new' function */

/* Create a new object */
void rd_with_notify_free( rd_with_notify_t *rdwn )
{
	close(rdwn->m_notify_pipe[0]);
	close(rdwn->m_notify_pipe[1]);
	free(rdwn);
} /* End of 'rd_with_notify_free' function */

/* Wait for input or notification */
int rd_with_notify_wait( rd_with_notify_t *rdwn )
{
	fd_set rdfs;
	int retv;
	int fd = RDWN_FD(rdwn);
	int notify_fd = RDWN_NOTIFY_READ_FD(rdwn);
	int nfd = ((notify_fd > fd) ? notify_fd : fd) + 1;
	int res = 0;

	/* Setup files for select */
	FD_ZERO(&rdfs);
	FD_SET(notify_fd, &rdfs);
	FD_SET(fd, &rdfs);

	/* Wait for some activity */
	retv = select(nfd, &rdfs, NULL, NULL, NULL);
	if (retv == -1)
		return 0;

	if (FD_ISSET(notify_fd, &rdfs))
		res |= RDWN_NOTIFY_READY;
	if (FD_ISSET(fd, &rdfs))
		res |= RDWN_READ_READY;
	return res;
} /* End of 'rd_with_notify_wait' function */

/* End of 'rd_with_notify.c' file */

