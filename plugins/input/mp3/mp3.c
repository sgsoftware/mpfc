/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : mp3.c
 * PURPOSE     : SG MPFC. MP3 input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 06.02.2004
 * NOTE        : Module prefix 'mp3'.
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

#include <errno.h>
#include <mad.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/soundcard.h>
#include "types.h"
#include "file.h"
#include "genre_list.h"
#include "inp.h"
#include "mp3.h"
#include "myid3.h"
#include "pmng.h"
#include "song_info.h"
#include "util.h"

#define MP3_IN_BUF_SIZE (5*8192)

/* libmad structures */
static struct mad_stream mp3_stream;
static struct mad_frame mp3_frame;
static struct mad_synth mp3_synth;
static mad_timer_t mp3_timer;

/* Input buffer */
static byte mp3_in_buf[MP3_IN_BUF_SIZE];

/* Decoded frames counter */
static dword mp3_frame_count = 0;

/* Samples pointer */
static dword mp3_samples = 0;

/* Current seek value */
static int mp3_seek_val = -1;

/* Current song audio parameters */
static int mp3_channels = 0, mp3_freq = 0, mp3_bitrate = 0;
static dword mp3_fmt = 0;

/* Current file name */
static char mp3_file_name[MAX_FILE_NAME] = "";

/* ID3 tag scheduled for saving */
static id3_tag_t *mp3_tag = NULL;

/* Current song information */
static song_info_t *mp3_cur_info = NULL;

/* Whether we are about to remove tag? */
static bool_t mp3_need_rem_tag = FALSE;

/* Genres list */
static genre_list_t *mp3_glist = NULL;

/* Current song length */
static int mp3_len = 0;

/* Current song number of frames */
static int mp3_total_num_frames = 0;

/* Multipliers for equalizer */
static mad_fixed_t mp3_eq_mul[32];

/* Current time */
static int mp3_time = 0;

/* File object */
static file_t *mp3_fd = NULL;

/* Plugins manager object */
static pmng_t *mp3_pmng = NULL;

/* Current song header */
struct mad_header mp3_head;

/* Message printer */
void (*mp3_print_msg)( char *fmt, ... );

/* Get song information */
static song_info_t *mp3_read_info( char *filename, int *len, int *nf );
	
/* Start play function */
bool_t mp3_start( char *filename )
{
	/* Remember tag */
	mp3_cur_info = mp3_read_info(filename, &mp3_len, &mp3_total_num_frames);

	/* Open file */
	mp3_fd = file_open(filename, "rb", mp3_print_msg);
	if (mp3_fd == NULL)
		return FALSE;
	
	/* Initialize libmad structures */
	mp3_frame_count = 0;
	mad_stream_init(&mp3_stream);
	mad_frame_init(&mp3_frame);
	mad_synth_init(&mp3_synth);
	mad_timer_reset(&mp3_timer);
	mp3_timer = mad_timer_zero;

	/* Save song parameters */
	mp3_time = 0;
	mp3_channels = 2;
	mp3_freq = 44100;
	mp3_fmt = AFMT_S16_LE;
	mp3_samples = 0;
	mp3_seek_val = -1;
	mp3_bitrate = 0;
	util_strncpy(mp3_file_name, filename, sizeof(mp3_file_name));

	/* Read song parameters */
	memset(&mp3_head, 0, sizeof(mp3_head));
	mp3_read_song_params();
	file_set_min_buf_size(mp3_fd, mp3_bitrate);
	return TRUE;
} /* End of 'mp3_start' function */

/* End playing function */
void mp3_end( void )
{
	if (mp3_fd != NULL)
	{
		mad_synth_finish(&mp3_synth);
		mad_frame_finish(&mp3_frame);
		mad_stream_finish(&mp3_stream);
		file_close(mp3_fd);
		mp3_fd = NULL;

		/* Remove tag */
		if (mp3_need_rem_tag)
		{
			id3_remove(mp3_file_name);
			mp3_need_rem_tag = FALSE;
		}
		/* Save tag */
		else if (mp3_tag != NULL)
		{
			id3_write(mp3_tag, mp3_file_name);
			id3_free(mp3_tag);
			mp3_tag = NULL;
		}
		
		si_free(mp3_cur_info);
		mp3_cur_info = NULL;
		strcpy(mp3_file_name, "");
		mp3_len = 0;
		mp3_time = 0;
	}
} /* End of 'mp3_end' function */

/* Get supported formats function */
void mp3_get_formats( char *extensions, char *content_type )
{
	if (extensions != NULL)
		strcpy(extensions, "mp3");
	if (content_type != NULL)
		strcpy(content_type, "audio/mpeg");
} /* End of 'mp3_get_formats' function */

/* Correct though long version of get_len function */
static int mp3_get_len_correct( char *filename, int *num_frames )
{
	struct mad_stream stream;
	struct mad_header header;
	mad_timer_t timer;
	unsigned char buffer[8192];
	unsigned int buflen = 0;
	file_t *fd;
	int i = 0;

	/* Check if we want to get current song length */
	if (!strcmp(filename, mp3_file_name))
	{
		*num_frames = mp3_total_num_frames;
		return mp3_len;
	}
	
	/* Open file */
	fd = file_open(filename, "rb", mp3_print_msg);
	if (fd == NULL)
		return 0;

	/* Initialize our structures */
	mad_stream_init(&stream);
	mad_header_init(&header);
	timer = mad_timer_zero;

	/* Scan file */
	for ( *num_frames = 0;; )
	{
		/* Read data */
		if (buflen < sizeof(buffer))
		{
			dword bytes;
			
			bytes = file_read(buffer + buflen, 1, sizeof(buffer) - buflen, fd);
			if (bytes <= 0)
				break;
			buflen += bytes;
		}

		/* Pipe data to MAD stream */
		mad_stream_buffer(&stream, buffer, buflen);

		/* Decode frame header */
		if (mad_header_decode(&header, &stream) >= 0)
		{
			/* Gather statistics */
			mad_timer_add(&timer, header.duration);
			(*num_frames) ++;
		}

		/* Move rest data (not decoded) to buffer start */
		memmove(buffer, stream.next_frame, &buffer[buflen] - stream.next_frame);
		buflen -= stream.next_frame - &buffer[0];
	} 

	/* Finish process */
	mad_header_finish(&header);
	mad_stream_finish(&stream);

	/* Close file */
	file_close(fd);

	/* Return */
	return mad_timer_count(timer, MAD_UNITS_SECONDS);
} /* End of 'mp3_get_len_correct' function */

/* Convert MAD fixed point sample to 16-bit */
static short mp3_mad_fixed_to_short( mad_fixed_t sample )
{
	sample += (1L << (MAD_F_FRACBITS - 16));
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;
	return(sample >> (MAD_F_FRACBITS + 1 - 16));
} /* End of 'mp3_mad_fixed_to_short' function */

/* Save song information */
void mp3_save_info( char *filename, song_info_t *info )
{
	id3_tag_t *tag;
	byte *data = NULL;
	int size;

	/* Supported only for regular files */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
		return;

	/* Save this tag */
	if (!strcmp(filename, mp3_file_name))
	{
		si_free(mp3_cur_info);
		mp3_cur_info = si_dup(info);
	}

	/* Read tag */
	tag = id3_read(filename);
	if (tag == NULL)
	{
		tag = id3_new();
		if (tag == NULL)
			return;
	}

	/* Update tag fields */
	id3_set_frame(tag, ID3_FRAME_TITLE, info->m_name, info->m_charset);
	id3_set_frame(tag, ID3_FRAME_ARTIST, info->m_artist, info->m_charset);
	id3_set_frame(tag, ID3_FRAME_ALBUM, info->m_album, info->m_charset);
	id3_set_frame(tag, ID3_FRAME_COMMENT, info->m_comments, info->m_charset);
	id3_set_frame(tag, ID3_FRAME_YEAR, info->m_year, info->m_charset);
	id3_set_frame(tag, ID3_FRAME_TRACK, info->m_track, info->m_charset);
	id3_set_genre(tag, info->m_genre, 
			glist_get_id_by_name(mp3_glist, info->m_genre), info->m_charset);

	/* Save tag or save it later */
	mp3_need_rem_tag = FALSE;
	if (!strcmp(filename, mp3_file_name))
	{
		mp3_tag = tag;
	}
	else
	{
		id3_write(tag, filename);
		id3_free(tag);
	}
} /* End of 'mp3_save_info' function */

/* Check if given buffer is frame header */
bool_t mp3_check_header( dword head )
{
	if ((head & 0xffe00000) != 0xffe00000)
		return FALSE;
	if (!((head >> 17) & 3))
		return FALSE;
	if (((head >> 12) & 0xf) == 0xf)
		return FALSE;
	if (!((head >> 12) & 0xf))
		return FALSE;
	if (((head >> 10) & 0x3) == 0x3)
		return FALSE;
	if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && 
			((head >> 16) & 1) == 1)
		return FALSE;
	if ((head & 0xffff0000) == 0xfffe0000)
		return FALSE;
	
	return TRUE;
} /* End of 'mp3_check_header' function */

/* Find frame start in mp3 file */
bool_t mp3_find_frame( file_t *fd )
{
	byte buf[4];
	dword head;

	if (file_read(buf, 1, 4, fd) != 4)
		return FALSE;
	head = ((byte)buf[0] << 24) | ((byte)buf[1] << 16) | 
		((byte)buf[2] << 8) | ((byte)buf[3]);
	for ( ; !mp3_check_header(head); )
	{
		byte buf[1];
		if (file_read(buf, 1, 1, fd) != 1)
			return FALSE;
		head <<= 8;
		head |= (byte)buf[0];
	}
	file_seek(fd, -4, SEEK_CUR);
	return TRUE;
} /* End of 'mp3_find_frame' function */

/* Get number of frames from XING header (if it exists) */
int mp3_get_xing_frames( char *filename )
{
	file_t *fd;
	int i, nf = 0, buflen;
	byte buf[8192], *xing;
	bool_t found = FALSE;
	
	/* Open file */
	fd = file_open(filename, "rb", mp3_print_msg);
	if (fd == NULL)
		goto end;

	/* Find first frame */
	if (!mp3_find_frame(fd))
		goto end;

	/* Find XING header in this frame */
	buflen = file_read(buf, 1, sizeof(buf), fd);
	for ( i = 0, xing = buf; i < buflen - 12; i ++, xing ++ )
	{
		if (!strncmp(xing, "Xing", 4))
		{
			found = TRUE;
			break;
		}
	}
	if (!found)
		goto end;

	/* Extract number of frames */
	xing += 7;
	if ((*xing) & 0x01)
	{
		xing ++;
		nf |= ((byte)(*xing) << 24); xing ++;
		nf |= ((byte)(*xing) << 16); xing ++;
		nf |= ((byte)(*xing) << 8); xing ++;
		nf |= ((byte)(*xing)); 
	}

end:
	/* Close file */
	file_close(fd);
	return nf;
} /* End of 'mp3_get_xing_frames' function */

/* Get song information */
static song_info_t *mp3_read_info( char *filename, int *len, int *nf )
{
	struct mad_header head;
	int i, filesize;
	id3_tag_t *tag;
	char own_data[1024];
	song_info_t *si = NULL;

	/* Supported only for regular files; for others - 
	 * output audio parameters */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
	{
		int br;
		
		/* May return anything only for current song */
		if (strcmp(filename, mp3_file_name))
		{
			*len = 0;
			if (nf != NULL)
				*nf = 0;
			return NULL;
		}

		*len = 0;
		si = si_new();
		si->m_flags |= SI_ONLY_OWN;
		snprintf(own_data, sizeof(own_data),
			_("MPEG %s, layer %i\n"
			"Bitrate: %i kb/s\n"
			"Samplerate: %i Hz\n"
			"%s\n"
			"Error protection: %s\n"
			"Copyright: %s\n"
			"Original: %s\n"
			"Emphasis: %s"),
			(mp3_head.flags & MAD_FLAG_MPEG_2_5_EXT) ? "2.5" : "1",
			(mp3_head.layer == MAD_LAYER_I) ? 1 : 
				(mp3_head.layer == MAD_LAYER_II ? 2 : 3),
			mp3_head.bitrate / 1000,
			mp3_head.samplerate,
			(mp3_head.mode == MAD_MODE_SINGLE_CHANNEL) ? _("Single channel") :
				(mp3_head.mode == MAD_MODE_DUAL_CHANNEL ? _("Dual channel") :
				 (mp3_head.mode == MAD_MODE_JOINT_STEREO ? _("Joint Stereo") : 
				  _("Stereo"))),
			(mp3_head.flags & MAD_FLAG_PROTECTION) ? _("Yes") : _("No"),
			(mp3_head.flags & MAD_FLAG_COPYRIGHT) ? _("Yes") : _("No"),
			(mp3_head.flags & MAD_FLAG_ORIGINAL) ? _("Yes") : _("No"),
			mp3_head.emphasis == MAD_EMPHASIS_NONE ? _("None") :
		  		(mp3_head.emphasis == MAD_EMPHASIS_50_15_US ? 
				 _("50/15 microseconds") : _("CCITT J.17")));
		si_set_own_data(si, own_data);
		if (nf != NULL)
			*nf = 0;
		return si;
	}
	
	/* Return temporary tag if we request for current playing song tag */
	if (!strcmp(filename, mp3_file_name))
	{
		*len = mp3_len;
		if (nf != NULL)
			*nf = mp3_total_num_frames;
		return si_dup(mp3_cur_info);
	}

	/* Read tag */
	si = si_new();
	si->m_glist = mp3_glist;
	tag = id3_read(filename);
	if (tag != NULL)
	{
		/* Scan tag frames */
		for ( ;; )
		{	
			id3_frame_t f;
			
			/* Get next frame */
			id3_next_frame(tag, &f);

			/* Handle frame */
			if (!strcmp(f.m_name, ID3_FRAME_TITLE))
			{
				si_set_name(si, f.m_val);
				if (f.m_encoding == ID3_UTF8)
					si_set_charset(si, "utf-8");
			}
			else if (!strcmp(f.m_name, ID3_FRAME_ARTIST))
				si_set_artist(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_ALBUM))
				si_set_album(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_YEAR))
				si_set_year(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_TRACK))
				si_set_track(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_COMMENT))
				si_set_comments(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_GENRE))
				si_set_genre(si, f.m_val);
			else if (!strcmp(f.m_name, ID3_FRAME_FINISHED))
			{
				id3_free_frame(&f);
				break;
			}

			id3_free_frame(&f);
		}

		/* Free tag */
		id3_free(tag);
	}

	/* Obtain additional song parameters */
	mad_header_init(&head);
	mp3_read_header(filename, &head);
	if (head.bitrate)
	{
		int tpf;
		bool_t frames_exact = FALSE;
		int num_frames = 0;
		
		filesize = util_get_file_size(filename);
		tpf = mad_timer_count(head.duration, MAD_UNITS_MILLISECONDS);

		/* Obtain length */
		if (cfg_get_var_int(pmng_get_cfg(mp3_pmng), "mp3-quick-get-len"))
		{
			/* Get number of frames from XING header and calculate 
			 * song length */
			if (num_frames = mp3_get_xing_frames(filename))
			{
				frames_exact = TRUE;
				*len = (tpf * num_frames) / 1000;
			}
			else if (head.bitrate)
			{
				*len = filesize / (head.bitrate >> 3);
				num_frames = (tpf == 0 ? 0 : (*len) * 1000 / tpf);
			}
		}
		else
		{
			*len = mp3_get_len_correct(filename, &num_frames);
			frames_exact = TRUE;
		}
		if (nf != NULL)
			*nf = num_frames;
		
		snprintf(own_data, sizeof(own_data),
			_("MPEG %s, layer %i\n"
			"Bitrate: %i kb/s\n"
			"Samplerate: %i Hz\n"
			"%s\n"
			"Error protection: %s\n"
			"Copyright: %s\n"
			"Original: %s\n"
			"Emphasis: %s\n"
			"%s%i frames\n"
			"Length: %i seconds\n"
			"File size: %i bytes"),
			(head.flags & MAD_FLAG_MPEG_2_5_EXT) ? "2.5" : "1",
			(head.layer == MAD_LAYER_I) ? 1 : (head.layer == MAD_LAYER_II ?
											   2 : 3),
			head.bitrate / 1000,
			head.samplerate,
			(head.mode == MAD_MODE_SINGLE_CHANNEL) ? _("Single channel") :
				(head.mode == MAD_MODE_DUAL_CHANNEL ? _("Dual channel") :
				 (head.mode == MAD_MODE_JOINT_STEREO ? _("Joint Stereo") : 
				  _("Stereo"))),
			(head.flags & MAD_FLAG_PROTECTION) ? _("Yes") : _("No"),
			(head.flags & MAD_FLAG_COPYRIGHT) ? _("Yes") : _("No"),
			(head.flags & MAD_FLAG_ORIGINAL) ? _("Yes") : _("No"),
			head.emphasis == MAD_EMPHASIS_NONE ? _("None") :
		  		(head.emphasis == MAD_EMPHASIS_50_15_US ? 
				 _("50/15 microseconds") : _("CCITT J.17")),
			frames_exact ? "" : _("approx. "), num_frames, *len,
			filesize);
			si_set_own_data(si, own_data);
	}
	mad_header_finish(&head);
	return si;
} /* End of 'mp3_read_info' function */

/* Get song information (exported) */
song_info_t *mp3_get_info( char *filename, int *len )
{
	return mp3_read_info(filename, len, NULL);
} /* End of 'mp3_get_info' function */

/* Get stream function */
int mp3_get_stream( void *buf, int size )
{
	int len = 0;
	byte *out_ptr = (byte *)buf;

	if (mp3_fd == NULL)
		return 0;

	/* Decode frame if samples counter is zero */
	if (!mp3_samples)
	{
		struct mad_header head;

		/* Seek */
		if (mp3_seek_val != -1 && mp3_bitrate)
		{
			file_seek(mp3_fd, mp3_seek_val * mp3_bitrate / 8, SEEK_SET);
			mp3_time = mp3_seek_val;
			mp3_timer.seconds = mp3_time;
			mp3_timer.fraction = 0;//mp3_time * MAD_TIMER_RESOLUTION;
			mp3_seek_val = -1;
		}

		/* Fill input buffer */
		if (mp3_stream.buffer == NULL || mp3_stream.error == MAD_ERROR_BUFLEN)
		{
			int read_size, remaining;
			byte *read_start;

			/* Prepare input buffer */
			if (mp3_stream.next_frame != NULL)
			{
				remaining = mp3_stream.bufend - mp3_stream.next_frame;
				memmove(mp3_in_buf, mp3_stream.next_frame, remaining);
				read_start = mp3_in_buf + remaining;
				read_size = MP3_IN_BUF_SIZE - remaining;
			}
			else
			{
				read_size = MP3_IN_BUF_SIZE;
				read_start = mp3_in_buf;
				remaining = 0;
			}

			/* Fill buffer */
			read_size = file_read(read_start, 1, read_size, mp3_fd);
			if (read_size <= 0)
			{
				return 0;
			}
	
			/* Pipe new buffer to libmad's stream decoder facility */
			mad_stream_buffer(&mp3_stream, mp3_in_buf, read_size + remaining);
			mp3_stream.error = 0;
		}

		/* Decode frame */
		if (mad_frame_decode(&mp3_frame, &mp3_stream))
			return mp3_get_stream(buf, size);

		/* Apply equalizer */
		mp3_apply_eq();

		/* Update file information */
		head = mp3_frame.header;
		mp3_freq = head.samplerate;
		mp3_fmt = AFMT_S16_LE;
		mp3_channels = (head.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
		mp3_bitrate = mp3_frame.header.bitrate;

		/* Accounting */
		mp3_frame_count ++;
		mad_timer_add(&mp3_timer, mp3_frame.header.duration);
		mp3_time = mp3_timer.seconds;

		/* Synthesize PCM samples */
		mad_synth_frame(&mp3_synth, &mp3_frame);
	}

	/* Convert libmad's samples from fixed point format */
	for ( ;mp3_samples < mp3_synth.pcm.length && len < size; mp3_samples ++ )
	{
		short sample;

		/* Left channel */
		sample = mp3_mad_fixed_to_short(mp3_synth.pcm.samples[0][mp3_samples]);
		*(out_ptr ++) = sample & 0xFF;
		*(out_ptr ++) = sample >> 8;
		len += 2;

		/* Right channel */
		if (MAD_NCHANNELS(&mp3_frame.header) == 2)
		{
			sample = mp3_mad_fixed_to_short(
					mp3_synth.pcm.samples[1][mp3_samples]);
			*(out_ptr ++) = sample & 0xFF;
			*(out_ptr ++) = sample >> 8;
			len += 2;
		}
	}

	if (mp3_samples >= mp3_synth.pcm.length)
		mp3_samples = 0;

	return len;
} /* End of 'mp3_get_stream' function */

/* Seek song */
void mp3_seek( int seconds )
{
	/* Supported only for regular files */
	if (file_get_type(mp3_file_name) != FILE_TYPE_REGULAR)
		return;
	
	if (mp3_fd != NULL)
	{
		mp3_seek_val = seconds;
	}
} /* End of 'mp3_seek' function */

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = mp3_channels;
	*freq = mp3_freq;
	*fmt = mp3_fmt;
	*bitrate = mp3_bitrate;
} /* End of 'mp3_get_audio_params' function */

/* Initialize genres list */
void mp3_init_glist( void )
{
	genre_list_t *l;

	/* Create list */
	if ((mp3_glist = glist_new()) == NULL)
		return;

	/* Fill it */
	l = mp3_glist;
	glist_add(l, "", 0xFF);
	glist_add(l, "A Capella", 0x7B);
	glist_add(l, "Acid", 0x22);
	glist_add(l, "Acid Jazz", 0x4A);
	glist_add(l, "Acid Punk", 0x49);
	glist_add(l, "Acoustic", 0x63);
	glist_add(l, "Alt. Rock", 0x28);
	glist_add(l, "Alternative", 0x14);
	glist_add(l, "Ambient", 0x1A);
	glist_add(l, "Anime", 0x91);
	glist_add(l, "Avantgarde", 0x5A);
	glist_add(l, "Ballad", 0x74);
	glist_add(l, "Bass", 0x29);
	glist_add(l, "Beat", 0x87);
	glist_add(l, "Bebop", 0x55);
	glist_add(l, "Big Band", 0x60);
	glist_add(l, "Black Metal", 0x8A);
	glist_add(l, "Bluegrass", 0x59);
	glist_add(l, "Blues", 0x00);
	glist_add(l, "Booty Bass", 0x6B);
	glist_add(l, "BritPop", 0x84);
	glist_add(l, "Cabaret", 0x41);
	glist_add(l, "Celtic", 0x58);
	glist_add(l, "Chamber Music", 0x68);
	glist_add(l, "Christian Gangsta Rap", 0x88);
	glist_add(l, "Christian Rap", 0x3D);
	glist_add(l, "Christian Rock", 0x8D);
	glist_add(l, "Classic Rock", 0x01);
	glist_add(l, "Classical", 0x20);
	glist_add(l, "Club", 0x70);
	glist_add(l, "Club-House", 0x80);
	glist_add(l, "Comedy", 0x39);
	glist_add(l, "Contemporary Christian", 0x8C);
	glist_add(l, "Country", 0x02);
	glist_add(l, "Crossover", 0x8B);
	glist_add(l, "Cult", 0x3A);
	glist_add(l, "Dance", 0x03);
	glist_add(l, "Dance Hall", 0x7D);
	glist_add(l, "Darkwave", 0x32);
	glist_add(l, "Death Metal", 0x16);
	glist_add(l, "Disco", 0x04);
	glist_add(l, "Dream", 0x37);
	glist_add(l, "Drum & Bass", 0x7F);
	glist_add(l, "Drum Solo", 0x7A);
	glist_add(l, "Duet", 0x78);
	glist_add(l, "Easy Listening", 0x62);
	glist_add(l, "Electronic", 0x34);
	glist_add(l, "Ethnic", 0x30);
	glist_add(l, "Eurodance", 0x36);
	glist_add(l, "Euro-House", 0x7C);
	glist_add(l, "Euro-Techno", 0x19);
	glist_add(l, "Fast-Fusion", 0x54);
	glist_add(l, "Folk", 0x50);
	glist_add(l, "Folk/Rock", 0x51);
	glist_add(l, "Folklore", 0x73);
	glist_add(l, "Freestyle", 0x77);
	glist_add(l, "Funk", 0x05);
	glist_add(l, "Fusion", 0x1E);
	glist_add(l, "Game", 0x24);
	glist_add(l, "Gangsta Rap", 0x3B);
	glist_add(l, "Goa", 0x7E);
	glist_add(l, "Gospel", 0x26);
	glist_add(l, "Gothic", 0x31);
	glist_add(l, "Gothic Rock", 0x5B);
	glist_add(l, "Grunge", 0x06);
	glist_add(l, "Hard Rock", 0x4F);
	glist_add(l, "Hardcore", 0x81);
	glist_add(l, "Heavy Metal", 0x89);
	glist_add(l, "Hip-Hop", 0x07);
	glist_add(l, "House", 0x23);
	glist_add(l, "Humour", 0x64);
	glist_add(l, "Indie", 0x83);
	glist_add(l, "Industrial", 0x13);
	glist_add(l, "Instrumental", 0x21);
	glist_add(l, "Instrumental Pop", 0x2E);
	glist_add(l, "Instrumental Rock", 0x2F);
	glist_add(l, "Jazz", 0x08);
	glist_add(l, "Jazz+Funk", 0x1D);
	glist_add(l, "JPop", 0x92);
	glist_add(l, "Jungle", 0x3F);
	glist_add(l, "Latin", 0x56);
	glist_add(l, "Lo-Fi", 0x47);
	glist_add(l, "Meditative", 0x2D);
	glist_add(l, "Merengue", 0x8E);
	glist_add(l, "Metal", 0x09);
	glist_add(l, "Musical", 0x4D);
	glist_add(l, "National Folk", 0x52);
	glist_add(l, "Native American", 0x40);
	glist_add(l, "Negerpunk", 0x85);
	glist_add(l, "New Age", 0x0A);
	glist_add(l, "New Wave", 0x42);
	glist_add(l, "Noise", 0x27);
	glist_add(l, "Oldies", 0x0B);
	glist_add(l, "Opera", 0x67);
	glist_add(l, "Other", 0x0C);
	glist_add(l, "Polka", 0x4B);
	glist_add(l, "Polsk Punk", 0x86);
	glist_add(l, "Pop", 0x0D);
	glist_add(l, "Pop/Funk", 0x3E);
	glist_add(l, "Pop-Folk", 0x35);
	glist_add(l, "Porn Groove", 0x6D);
	glist_add(l, "Power Ballad", 0x75);
	glist_add(l, "Prank", 0x17);
	glist_add(l, "Primus", 0x6C);
	glist_add(l, "Progressive Rock", 0x5C);
	glist_add(l, "Psychedelic", 0x43);
	glist_add(l, "Psychedelic Rock", 0x5D);
	glist_add(l, "Punk", 0x2B);
	glist_add(l, "Punk Rock", 0x79);
	glist_add(l, "R&B", 0x0E);
	glist_add(l, "Rap", 0x0F);
	glist_add(l, "Rave", 0x44);
	glist_add(l, "Reggae", 0x10);
	glist_add(l, "Retro", 0x4C);
	glist_add(l, "Revival", 0x57);
	glist_add(l, "Rythmic Soul", 0x76);
	glist_add(l, "Rock", 0x11);
	glist_add(l, "Rock & Roll", 0x4E);
	glist_add(l, "Salsa", 0x8F);
	glist_add(l, "Samba", 0x72);
	glist_add(l, "Satire", 0x6E);
	glist_add(l, "Showtunes", 0x45);
	glist_add(l, "Ska", 0x15);
	glist_add(l, "Slow Jam", 0x6F);
	glist_add(l, "Slow Rock", 0x5F);
	glist_add(l, "Sonata", 0x69);
	glist_add(l, "Soul", 0x2A);
	glist_add(l, "Sound Clip", 0x25);
	glist_add(l, "Soundtrack", 0x18);
	glist_add(l, "Southern Rock", 0x38);
	glist_add(l, "Space", 0x2C);
	glist_add(l, "Speech", 0x65);
	glist_add(l, "Swing", 0x53);
	glist_add(l, "Symphonic Rock", 0x5E);
	glist_add(l, "Symphony", 0x6A);
	glist_add(l, "Synthpop", 0x93);
	glist_add(l, "Tango", 0x71);
	glist_add(l, "Techno", 0x12);
	glist_add(l, "Techno-Industrial", 0x33);
	glist_add(l, "Terror", 0x82);
	glist_add(l, "Thrash Metal", 0x90);
	glist_add(l, "Top 40", 0x3C);
	glist_add(l, "Trailer", 0x46);
	glist_add(l, "Trance", 0x1F);
	glist_add(l, "Tribal", 0x48);
	glist_add(l, "Trip-Hop", 0x1B);
	glist_add(l, "Vocal", 0x1C);
} /* End of 'mp3_init_glist' function */

/* Get current time */
int mp3_get_cur_time( void )
{
	return mp3_time;
} /* End of 'mp3_get_cur_time' function */

/* Remove ID3 tags */
static void mp3_remove_tag( char *filename )
{
	if (mp3_print_msg != NULL)
	{
		mp3_print_msg(_("Removing tag from file %s"), filename);
	}
	if (strcmp(filename, mp3_file_name))
	{
		id3_remove(filename);
	}
	else
	{
		mp3_need_rem_tag = TRUE;
		if (mp3_tag != NULL)
		{
			id3_free(mp3_tag);
			mp3_tag = NULL;
		}
		if (mp3_cur_info != NULL)
		{
			song_info_t *si = si_new();
			si_set_own_data(si, mp3_cur_info->m_own_data);
			si_free(mp3_cur_info);
			mp3_cur_info = si;
		}
	}
	if (mp3_print_msg != NULL)
		mp3_print_msg(_("OK"));
} /* End of 'mp3_remove_tag' function */

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = mp3_start;
	fl->m_end = mp3_end;
	fl->m_get_stream = mp3_get_stream;
	fl->m_get_info = mp3_get_info;
	fl->m_save_info = mp3_save_info;
	fl->m_seek = mp3_seek;
	fl->m_get_audio_params = mp3_get_audio_params;
	fl->m_get_formats = mp3_get_formats;
	fl->m_set_eq = mp3_set_eq;
	fl->m_get_cur_time = mp3_get_cur_time;
	mp3_pmng = fl->m_pmng;
	mp3_print_msg = pmng_get_printer(mp3_pmng);

	fl->m_num_spec_funcs = 1;
	fl->m_spec_funcs = (inp_spec_func_t *)malloc(sizeof(inp_spec_func_t) * 
			fl->m_num_spec_funcs);
	fl->m_spec_funcs[0].m_title = strdup(_("Remove ID3 tags"));
	fl->m_spec_funcs[0].m_flags = 0;
	fl->m_spec_funcs[0].m_func = mp3_remove_tag;
} /* End of 'inp_get_func_list' function */

/* This function is called when initializing module */
void _init( void )
{
	/* Initialize genres list */
	mp3_init_glist();
} /* End of '_init' function */

/* This function is called when uninitializing module */
void _fini( void )
{
	glist_free(mp3_glist);
} /* End of '_fini' function */

/* Set equalizer parameters */
void mp3_set_eq( void )
{
	byte map[32] = { 0, 1, 2, 3, 4, 5, 6, 6, 7, 7, 7, 7, 8, 8, 8,
  						8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 
						9, 9, 9	};
	int i;
	
	for ( i = 0; i < 32; i ++ )
	{
		char name[20];
		float val;
		
		sprintf(name, "eq-band%i", map[i] + 1);
		val = cfg_get_var_float(pmng_get_cfg(mp3_pmng), "eq-preamp") + 
			cfg_get_var_float(pmng_get_cfg(mp3_pmng), name);
		if (val > 18.)
			val = 18.;
		val = pow(10., val / 20.);
		mp3_eq_mul[i] = mad_f_tofixed(val);
	}
} /* End of 'mp3_set_eq' function */

/* Apply equalizer to frame */
static void mp3_apply_eq( void )
{
	unsigned int nch, ch, ns, s, sb;

	/* Do nothing if equalizer is disabled */
	if (cfg_get_var_int(pmng_get_cfg(mp3_pmng), "equalizer-off"))
		return;

	nch = MAD_NCHANNELS(&mp3_frame.header);
	ns = MAD_NSBSAMPLES(&mp3_frame.header);
	for ( ch = 0; ch < nch; ++ch )
		for ( s = 0; s < ns; ++s )
			for ( sb = 0; sb < 32; ++sb )
				mp3_frame.sbsample[ch][s][sb] = 
					mad_f_mul(mp3_frame.sbsample[ch][s][sb], mp3_eq_mul[sb]);
} /* End of 'mp3_apply_eq' function */

/* Read song parameters */
static void mp3_read_song_params( void )
{
	struct mad_stream stream;
	struct mad_header head;
	byte buffer[8192];
	int buflen = 0;

	/* Initialize MAD structures */
	mad_stream_init(&stream);
	mad_header_init(&head);

	/* Scan file */
	for ( ;; )
	{
		/* Read data */
		if (buflen < sizeof(buffer))
		{
			dword bytes;
			
			bytes = file_read(buffer + buflen, 1, 
					sizeof(buffer) - buflen, mp3_fd);
			if (bytes <= 0)
				break;
			buflen += bytes;
		}

		/* Pipe data to MAD stream */
		mad_stream_buffer(&stream, buffer, buflen);

		/* Decode frame header */
		mad_header_decode(&head, &stream);

		/* Move rest data (not decoded) to buffer start */
		memmove(buffer, stream.next_frame, &buffer[buflen] - stream.next_frame);
		buflen -= stream.next_frame - &buffer[0];

		/* Save parameters */
		if (head.bitrate)
		{
			mp3_freq = head.samplerate;
			mp3_fmt = AFMT_S16_LE;
			mp3_channels = (head.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
			mp3_bitrate = head.bitrate;
			memcpy(&mp3_head, &head, sizeof(head));
			break;
		}
	} 
	
	/* Unitialize all */
	mad_header_finish(&head);
	mad_stream_finish(&stream);
	file_seek(mp3_fd, 0, SEEK_SET);
} /* End of 'mp3_read_song_params' function */

/* Read mp3 file header */
static void mp3_read_header( char *filename, struct mad_header *head )
{
	struct mad_stream stream;
	byte buffer[8192];
	int buflen = 0;
	file_t *fd;

	if (!strcmp(filename, mp3_file_name))
		return;

	/* Open file */
	fd = file_open(filename, "rb", mp3_print_msg);
	if (fd == NULL)
		return;

	/* Initialize MAD structures */
	mad_stream_init(&stream);

	/* Scan file */
	for ( ;; )
	{
		/* Read data */
		if (buflen < sizeof(buffer))
		{
			dword bytes;
			
			bytes = file_read(buffer + buflen, 1, sizeof(buffer) - buflen, fd);
			if (bytes <= 0)
				break;
			buflen += bytes;
		}

		/* Pipe data to MAD stream */
		mad_stream_buffer(&stream, buffer, buflen);

		/* Decode frame header */
		mad_header_decode(head, &stream);

		/* Move rest data (not decoded) to buffer start */
		memmove(buffer, stream.next_frame, &buffer[buflen] - stream.next_frame);
		buflen -= stream.next_frame - &buffer[0];

		/* Save parameters */
		if (head->bitrate)
			break;
	} 

	/* Unitialize all */
	mad_stream_finish(&stream);
	file_close(fd);
} /* End of 'mp3_read_header' function */

/* End of 'mp3.c' file */

