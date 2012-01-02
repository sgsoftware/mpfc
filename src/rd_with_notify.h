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

#ifndef __SG_MPFC_RD_WITH_NOTIFY_H__
#define __SG_MPFC_RD_WITH_NOTIFY_H__

#include "types.h"

typedef struct
{
	/* Main file we are reading from */
	int m_fd;

	/* Pipe for communicating notifications */
	int m_notify_pipe[2];
} rd_with_notify_t;

#define RDWN_FD(x)				((x)->m_fd)
#define RDWN_NOTIFY_READ_FD(x)	((x)->m_notify_pipe[0])
#define RDWN_NOTIFY_WRITE_FD(x)	((x)->m_notify_pipe[1])

#define RDWN_READ_READY		(1 << 0)
#define RDWN_NOTIFY_READY	(1 << 1)

/* Create a new object */
rd_with_notify_t *rd_with_notify_new( int fd );

/* Create a new object */
void rd_with_notify_free( rd_with_notify_t *rdwn );

/* Wait for input or notification */
int rd_with_notify_wait( rd_with_notify_t *rdwn );

#endif

/* End of 'rd_with_notify.h' file */

