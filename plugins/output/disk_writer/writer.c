/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : disk_writer.c
 * PURPOSE     : SG MPFC. Disk writer output plugin functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.08.2003
 * NOTE        : Module prefix 'dw'.
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
#include <sys/soundcard.h>
#include "types.h"
#include "file.h"
#include "outp.h"
#include "pmng.h"

/* Header size */
#define DW_HEAD_SIZE 44

/* Output file handler */
static file_t *dw_fd = NULL;

/* Song parameters */
static short dw_channels = 2;
static long dw_freq = 44100;
static dword dw_fmt = 0;
static long dw_file_size = 0;

/* Plugins manager */
static pmng_t *dw_pmng = NULL;

/* Start plugin */
bool_t dw_start( void )
{
	char name[MAX_FILE_NAME], full_name[MAX_FILE_NAME];
	char *str;
	int i;

	/* Get output file name */
	str = cfg_get_var(pmng_get_cfg(dw_pmng), "cur-song-name");
	if (str == NULL)
		return FALSE;
	util_strncpy(name, str, sizeof(name));
	str = strrchr(name, '.');
	if (str != NULL)
		strcpy(str, ".wav");
	else
		strcat(name, ".wav");
	util_replace_char(name, ':', '_');
	str = cfg_get_var(pmng_get_cfg(dw_pmng), "disk-writer-path");
	if (str != NULL)
		snprintf(full_name, sizeof(full_name), "%s/%s", str, name);
	else
		snprintf(full_name, sizeof(full_name), "%s", name);

	/* Try to open file */
	dw_fd = file_open(full_name, "w+b", NULL);
	if (dw_fd == NULL)
		return FALSE;

	/* Leave space for header */
	file_seek(dw_fd, DW_HEAD_SIZE, SEEK_SET);
	dw_file_size = DW_HEAD_SIZE;
	return TRUE;
} /* End of 'dw_start' function */

/* Write WAV header */
static void dw_write_head( void )
{
	long chunksize1 = 16, chunksize2 = dw_file_size - DW_HEAD_SIZE;
	short format_tag = 1;
	short databits, block_align;
	long avg_bps;
	
	if (dw_fd == NULL)
		return;
	
	/* Set some variables */
	chunksize1 = 16;
	chunksize2 = dw_file_size - DW_HEAD_SIZE;
	format_tag = 1;
	databits = (dw_fmt == AFMT_U8 || dw_fmt == AFMT_S8) ? 8 : 16;
	block_align = databits * dw_channels / 8;
	avg_bps = dw_freq * block_align;

	/* Write header */
	file_seek(dw_fd, 0, SEEK_SET);
	file_write("RIFF", 1, 4, dw_fd);
	file_write(&dw_file_size, 4, 1, dw_fd);
	file_write("WAVE", 1, 4, dw_fd);
	file_write("fmt ", 1, 4, dw_fd);
	file_write(&chunksize1, 4, 1, dw_fd);
	file_write(&format_tag, 2, 1, dw_fd);
	file_write(&dw_channels, 2, 1, dw_fd);
	file_write(&dw_freq, 4, 1, dw_fd);
	file_write(&avg_bps, 4, 1, dw_fd);
	file_write(&block_align, 2, 1, dw_fd);
	file_write(&databits, 2, 1, dw_fd);
	file_write("data", 1, 4, dw_fd);
	file_write(&chunksize2, 4, 1, dw_fd);
} /* End of 'dw_write_head' function */

/* Stop plugin */
void dw_end( void )
{
	if (dw_fd != NULL)
	{
		dw_write_head();
		file_close(dw_fd);
		dw_fd = NULL;
	}
} /* End of 'dw_end' function */

/* Play stream */
void dw_play( void *buf, int size )
{
	if (dw_fd == NULL)
		return;
	
	file_write(buf, 1, size, dw_fd);
	dw_file_size += size;
} /* End of 'dw_play' function */

/* Set channels number */
void dw_set_channels( int ch )
{
	dw_channels = ch;
} /* End of 'dw_set_channels' function */

/* Set playing frequency */
void dw_set_freq( int freq )
{
	dw_freq = freq;
} /* End of 'dw_set_freq' function */

/* Set playing format */
void dw_set_fmt( dword fmt )
{
	dw_fmt = fmt;
} /* End of 'dw_set_bits' function */

/* Get function list */
void outp_get_func_list( outp_func_list_t *fl )
{
	fl->m_start = dw_start;
	fl->m_end = dw_end;
	fl->m_play = dw_play;
	fl->m_set_channels = dw_set_channels;
	fl->m_set_freq = dw_set_freq;
	fl->m_set_fmt = dw_set_fmt;
	fl->m_flags = OUTP_NO_SOUND;
	dw_pmng = fl->m_pmng;
} /* End of 'outp_get_func_list' function */

/* End of 'writer.c' file */

