/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : timidi.c
 * PURPOSE     : SG MPFC. TiMidi (playing midi through TiMidity) 
 *               plugin functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 17.01.2004
 * NOTE        : Module prefix 'midi'.
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
#include <sys/soundcard.h>
#include <unistd.h>
#include "types.h"
#include "file.h"
#include "inp.h"
#include "timidi.h"

/* Currently opened pipe descriptor */
FILE *midi_pfd = NULL;

/* Current song name */
char midi_fname[MAX_FILE_NAME] = "";

/* Start playing */
bool_t midi_start( char *filename )
{
	char cmd[512];
	
	/* Run TiMidity */
	sprintf(cmd, "timidity -id -Or1sl -s 44100 %s -o - 2>/dev/null", filename);
	midi_pfd = popen(cmd, "r");
	if (midi_pfd == NULL)
		return FALSE;

	/* Save file name */
	strcpy(midi_fname, filename);
	return TRUE;
} /* End of 'midi_start' function */

/* End playing */
void midi_end( void )
{
	/* Close pipe */
	if (midi_pfd != NULL)
	{
		pclose(midi_pfd);
		midi_pfd = NULL;
	}
	strcpy(midi_fname, "");
} /* End of 'midi_end' function */

/* Get supported file formats */
void midi_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "mid;midi");
	if (content_type != NULL)
		strcpy(content_type, "audio/midi");
} /* End of 'midi_get_formats' function */

/* Get audio stream */
int midi_get_stream( void *buf, int size )
{
	/* Read data from pipe */
	if (midi_pfd != NULL)
		return fread(buf, 1, size, midi_pfd);
	else
		return 0;
} /* End of 'midi_get_stream' function */

/* Seek */
void midi_seek( int shift )
{
} /* End of 'midi_seek' function */

/* Get song audio parameters */
void midi_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = AFMT_S16_LE;
	*bitrate = 0;
} /* End of 'midi_get_audio_params' function */

/* Get current time */
int midi_get_cur_time( void )
{
	return -1;
} /* End of 'midi_get_cur_time' function */

/* Set song title */
str_t *midi_set_song_title( char *filename )
{
	return str_new(filename);
} /* End of 'midi_set_song_title' function */

/* Get song info */
song_info_t *midi_get_info( char *filename, int *len )
{
	file_t *fd;
	
	/* Open file */
	*len = 0;
	fd = file_open(filename, "rb", NULL);
	if (fd == NULL)
		return NULL;
	
	/* Close file */
	file_close(fd);
	return NULL;
} /* End of 'midi_get_info' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = midi_start;
	fl->m_end = midi_end;
	fl->m_get_stream = midi_get_stream;
	fl->m_seek = midi_seek;
	fl->m_get_audio_params = midi_get_audio_params;
	fl->m_get_formats = midi_get_formats;
	fl->m_get_cur_time = midi_get_cur_time;
} /* End of 'inp_get_func_list' function */

/* End of 'timidi.c' file */

