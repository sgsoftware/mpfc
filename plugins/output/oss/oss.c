/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : oss.c
 * PURPOSE     : SG MPFC. OSS output plugin functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.09.2003
 * NOTE        : Module prefix 'oss'.
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

#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include "types.h"
#include "outp.h"

/* Audio device file handle */
int oss_fd = -1;

/* Start plugin */
bool_t oss_start( void )
{
	int fmt;
	int i;
	char name[256];

	/* Check if we have access to sound card */
	strcpy(name, "/dev/dsp");
	oss_fd = open(name, O_WRONLY | O_NONBLOCK);
	if (oss_fd < 0)
	{
		for ( i = 1; i < 256; i ++ )
		{
			sprintf(name, "/dev/dsp%i", i);
			oss_fd = open(name, O_WRONLY | O_NONBLOCK);
			if (oss_fd >= 0)
				break;
		}
		if (oss_fd < 0)
			return FALSE;
	}
	close(oss_fd);
	oss_fd = -1;

	/* Open device */
	oss_fd = open(name, O_WRONLY);
	if (oss_fd < 0)
		return FALSE;
	
	return TRUE;
} /* End of 'oss_start' function */

/* Stop plugin */
void oss_end( void )
{
	if (oss_fd != -1)
	{
		close(oss_fd);
		oss_fd = -1;
	}
} /* End of 'oss_end' function */

/* Play stream */
void oss_play( void *buf, int size )
{
	if (oss_fd == -1)
		return;
	
	write(oss_fd, buf, size);
} /* End of 'oss_play' function */

/* Set channels number */
void oss_set_channels( int ch )
{
	if (oss_fd == -1)
		return;

	ioctl(oss_fd, SNDCTL_DSP_CHANNELS, &ch);
} /* End of 'oss_set_channels' function */

/* Set playing frequency */
void oss_set_freq( int freq )
{
	if (oss_fd == -1)
		return;

	ioctl(oss_fd, SNDCTL_DSP_SPEED, &freq);
} /* End of 'oss_set_freq' function */

/* Set playing format */
void oss_set_fmt( dword fmt )
{
	if (oss_fd == -1)
		return;

	ioctl(oss_fd, SNDCTL_DSP_SETFMT, &fmt);
} /* End of 'oss_set_bits' function */

/* Flush function */
void oss_flush( void )
{
	if (oss_fd == -1)
		return;

	ioctl(oss_fd, SNDCTL_DSP_SYNC, 0);
} /* End of 'oss_flush' function */

/* Set volume */
void oss_set_volume( int left, int right )
{
	int fd;
	word v;

	fd = open("/dev/mixer", O_WRONLY);
	if (fd < 0)
		return;
	v = right | (left << 8);
	ioctl(fd, SOUND_MIXER_WRITE_VOLUME, &v);
	close(fd);
} /* End of 'oss_set_volume' function */

/* Get volume */
void oss_get_volume( int *left, int *right )
{
	int fd;
	word v;

	fd = open("/dev/mixer", O_RDONLY);
	if (fd < 0)
		return;
	ioctl(fd, SOUND_MIXER_READ_VOLUME, &v);
	close(fd);
	*left = ((v >> 8) & 0xFF);
	*right = (v & 0xFF);
} /* End of 'oss_get_volume' function */

/* Get function list */
void outp_get_func_list( outp_func_list_t *fl )
{
	fl->m_start = oss_start;
	fl->m_end = oss_end;
	fl->m_play = oss_play;
	fl->m_set_channels = oss_set_channels;
	fl->m_set_freq = oss_set_freq;
	fl->m_set_fmt = oss_set_fmt;
	fl->m_flush = oss_flush;
	fl->m_set_volume = oss_set_volume;
	fl->m_get_volume = oss_get_volume;
} /* End of 'outp_get_func_list' function */

/* End of 'oss.c' file */

