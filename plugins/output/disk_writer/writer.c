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
#include "cfg.h"
#include "outp.h"

/* Header size */
#define DW_HEAD_SIZE 44

/* Output file handler */
FILE *dw_fd = NULL;

/* Song parameters */
short dw_channels = 2;
long dw_freq = 44100;
dword dw_fmt = 0;
long dw_file_size = 0;

/* Configuration variables list */
cfg_list_t *dw_var_list = NULL;

/* Start plugin */
bool dw_start( void )
{
	char name[256], full_name[256];
	char *cur_song;
	int i;

	/* Get output file name */
	cur_song = cfg_get_var(dw_var_list, "cur_song_name");
	for ( i = strlen(cur_song) - 1; i >= 0 && cur_song[i] != '.'; i -- );
	if (i < 0)
		i = strlen(cur_song) - 1;
	memcpy(name, cur_song, i + 1);
	name[i + 1] = 0;
	sprintf(full_name, "%s/%swav", cfg_get_var(dw_var_list, 
				"disk_writer_path"), name);

	/* Try to open file */
	dw_fd = fopen(full_name, "w+b");
	if (dw_fd == NULL)
		return FALSE;

	/* Leave space for header */
	fseek(dw_fd, DW_HEAD_SIZE, SEEK_SET);
	dw_file_size = DW_HEAD_SIZE;
	return TRUE;
} /* End of 'dw_start' function */

/* Write WAV header */
void dw_write_head( void )
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
	fseek(dw_fd, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, dw_fd);
	fwrite(&dw_file_size, 4, 1, dw_fd);
	fwrite("WAVE", 1, 4, dw_fd);
	fwrite("fmt ", 1, 4, dw_fd);
	fwrite(&chunksize1, 4, 1, dw_fd);
	fwrite(&format_tag, 2, 1, dw_fd);
	fwrite(&dw_channels, 2, 1, dw_fd);
	fwrite(&dw_freq, 4, 1, dw_fd);
	fwrite(&avg_bps, 4, 1, dw_fd);
	fwrite(&block_align, 2, 1, dw_fd);
	fwrite(&databits, 2, 1, dw_fd);
	fwrite("data", 1, 4, dw_fd);
	fwrite(&chunksize2, 4, 1, dw_fd);
} /* End of 'dw_write_head' function */

/* Stop plugin */
void dw_end( void )
{
	if (dw_fd != NULL)
	{
		dw_write_head();
		fclose(dw_fd);
		dw_fd = NULL;
	}
} /* End of 'dw_end' function */

/* Play stream */
void dw_play( void *buf, int size )
{
	if (dw_fd == NULL)
		return;
	
	fwrite(buf, 1, size, dw_fd);
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
} /* End of 'outp_get_func_list' function */

/* Save variables list */
void outp_set_vars( cfg_list_t *list )
{
	dw_var_list = list;
} /* End of 'outp_set_vars' function */

/* End of 'writer.c' file */

