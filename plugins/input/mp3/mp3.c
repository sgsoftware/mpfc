
/* FILE NAME   : mp3.c
 * PURPOSE     : SG Konsamp. MP3 input plugin functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 13.08.2003
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
#include <id3tag.h>
#include <mad.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/soundcard.h>
#include "types.h"
#include "cfg.h"
#include "genre_list.h"
#include "inp.h"
#include "mp3.h"
#include "song_info.h"
#include "util.h"

#define MP3_IN_BUF_SIZE (5*8192)

/* Declare some functions */
struct id3_frame *id3_frame_new(char const *);

/* libmad structures */
struct mad_stream mp3_stream;
struct mad_frame mp3_frame;
struct mad_synth mp3_synth;
mad_timer_t mp3_timer;

/* Input buffer */
byte mp3_in_buf[MP3_IN_BUF_SIZE];

/* Decoded frames counter */
dword mp3_frame_count = 0;

/* Samples pointer */
dword mp3_samples = 0;

/* Current seek value */
int mp3_seek_val = -1;

/* Current song audio parameters */
int mp3_channels = 0, mp3_freq = 0, mp3_bitrate = 0;
dword mp3_fmt = 0;

/* Current file name */
char mp3_file_name[128] = "";

/* ID3 tag scheduled for saving */
byte *mp3_tag = NULL;
int mp3_tag_size = 0;

/* Current song information */
song_info_t mp3_cur_info;

/* Genres list */
genre_list_t *mp3_glist = NULL;

/* Current song length */
int mp3_len = 0;

/* Multipliers for equalizer */
mad_fixed_t mp3_eq_mul[32];

/* This is pointer to global variables list */
cfg_list_t *mp3_var_list = NULL;

/* Is ID3 tag present */
bool mp3_tag_present = FALSE;

/* Current time */
int mp3_time = 0;

/* File descriptor */
FILE *mp3_fd = NULL;

/* Start play function */
bool mp3_start( char *filename )
{
	/* Remember tag */
	mp3_tag_present = mp3_get_info(filename, &mp3_cur_info);
	//mp3_len = mp3_get_len(filename);

	/* Open file */
	mp3_fd = fopen(filename, "rb");
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
	strcpy(mp3_file_name, filename);
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
		fclose(mp3_fd);
		mp3_fd = NULL;

		/* Save tag */
		if (mp3_tag != NULL)
		{
			mp3_save_tag(mp3_file_name, mp3_tag, mp3_tag_size);
			free(mp3_tag);
			mp3_tag = NULL;
			mp3_tag_size = 0;
		}
		
		strcpy(mp3_file_name, "");
		mp3_len = 0;
		mp3_time = 0;
	}
} /* End of 'mp3_end' function */

/* Get supported formats function */
void mp3_get_formats( char *buf )
{
	strcpy(buf, "mp3");
} /* End of 'mp3_get_formats' function */

/* Correct though long version of get_len function */
int mp3_get_len_correct( char *filename )
{
	struct mad_stream stream;
	struct mad_header header;
	mad_timer_t timer;
	unsigned char buffer[8192];
	unsigned int buflen = 0;
	FILE *fd;
	int i = 0;

	/* Check if we want to get current song length */
	if (!strcmp(filename, mp3_file_name))
		return mp3_len;
	
	/* Open file */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return 0;

	/* Initialize our structures */
	mad_stream_init(&stream);
	mad_header_init(&header);
	timer = mad_timer_zero;

	/* Scan file */
	for ( ;; )
	{
		/* Read data */
		if (buflen < sizeof(buffer))
		{
			dword bytes;
			
			bytes = fread(buffer + buflen, 1, sizeof(buffer) - buflen, fd);
			if (bytes <= 0)
				break;
			buflen += bytes;
		}

		/* Pipe data to MAD stream */
		mad_stream_buffer(&stream, buffer, buflen);

		/* Decode frame header */
		mad_header_decode(&header, &stream);

		/* Increase length */
		mad_timer_add(&timer, header.duration);

		/* Move rest data (not decoded) to buffer start */
		memmove(buffer, stream.next_frame, &buffer[buflen] - stream.next_frame);
		buflen -= stream.next_frame - &buffer[0];
	} 

	/* Finish process */
	mad_header_finish(&header);
	mad_stream_finish(&stream);

	/* Close file */
	fclose(fd);

	/* Return */
	return mad_timer_count(timer, MAD_UNITS_SECONDS);
} /* End of 'mp3_get_len_correct' function */

/* Quick though not very correct version of get_len function */
int mp3_get_len_quick( char *filename )
{
	struct mad_stream stream;
	struct mad_header header;
	unsigned char buffer[8192];
	unsigned int buflen = 0, file_size = 0, bitrate = 0;
	FILE *fd;
	int i = 0;

	/* Check if we want to get current song length */
	if (!strcmp(filename, mp3_file_name))
		return mp3_len;
	
	/* Open file */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return 0;

	/* Initialize our structures */
	mad_stream_init(&stream);
	mad_header_init(&header);

	/* Find first frame with non-zero bitrate */
	for ( ;; )
	{
		/* Read data */
		if (buflen < sizeof(buffer))
		{
			dword bytes;
			
			bytes = fread(buffer + buflen, 1, sizeof(buffer) - buflen, fd);
			if (bytes <= 0)
				break;
			buflen += bytes;
		}

		/* Pipe data to MAD stream */
		mad_stream_buffer(&stream, buffer, buflen);

		/* Decode frame header */
		mad_header_decode(&header, &stream);

		/* Check bitrate */
		if (header.bitrate)
		{
			bitrate = header.bitrate;
			break;
		}

		/* Move rest data (not decoded) to buffer start */
		memmove(buffer, stream.next_frame, &buffer[buflen] - stream.next_frame);
		buflen -= stream.next_frame - &buffer[0];
	} 

	/* Finish process */
	mad_header_finish(&header);
	mad_stream_finish(&stream);

	/* Get total file size */
	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);

	/* Close file */
	fclose(fd);

	/* Calculate song length */
	return (bitrate ? ((float)file_size / bitrate) * 8 : 0);
} /* End of 'mp3_get_len_quick' function */

/* Get length */
int mp3_get_len( char *filename )
{
	/* See what user wants */
	if (cfg_get_var_int(mp3_var_list, "mp3_quick_get_len"))
		return mp3_get_len_quick(filename);
	else
		return mp3_get_len_correct(filename);
} /* End of 'mp3_get_len' function */

/* Convert MAD fixed point sample to 16-bit */
short mp3_mad_fixed_to_short( mad_fixed_t sample )
{
	sample += (1L << (MAD_F_FRACBITS - 16));
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;
	return(sample >> (MAD_F_FRACBITS + 1 - 16));
} /* End of 'mp3_mad_fixed_to_short' function */

/* Set value of ID3 tag frame */
void mp3_set_tag_frame( struct id3_tag *tag, char *id, char *val )
{
	struct id3_frame *f;
	id3_ucs4_t *ucs4;

	/* Find frame with specified ID and convert value to ucs4 */
	f = id3_tag_findframe(tag, id, 0);
	ucs4 = (id3_ucs4_t *)malloc((strlen(val) + 1) * sizeof(*ucs4));
	id3_latin1_decode(val, ucs4);

	/* Write frame data */
	if (f == NULL)
	{
		f = id3_frame_new(id);
		id3_field_settextencoding(&f->fields[0], 
				ID3_FIELD_TEXTENCODING_ISO_8859_1);
		id3_field_setstrings(&f->fields[1], 1, &ucs4);
		id3_tag_attachframe(tag, f);
	}
	else
	{
		id3_field_settextencoding(&f->fields[0], 
				ID3_FIELD_TEXTENCODING_ISO_8859_1);
		id3_field_setstrings(&f->fields[1], 1, &ucs4);
	}
	free(ucs4);
} /* End of 'mp3_set_tag_frame' function */

/* Save tag to file */
void mp3_save_tag( char *filename, byte *tag, int size )
{
	FILE *fd;
	int file_size;
	byte *data;
	byte magic[4];
	int prev_size;

	/* Open file */
	fd = fopen(filename, "rb");
	if (fd == NULL)
		return;

	/* Get information about current tag */
	fread(magic, 1, 3, fd);
	prev_size = 0;
	if (magic[0] == 'I' && magic[1] == 'D' && magic[2] == '3')
	{
		byte s[4];
		byte f;
		fseek(fd, 5, SEEK_SET);
		fread(&f, 1, 1, fd);
		fread(&s, 1, 4, fd);
		prev_size = s[3] | (s[2] << 7) | (s[1] << 14) | (s[0] << 21);
		prev_size += 10;
		if (f & 16)
			prev_size += 10;
	}

	/* Get file size */
	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd) - prev_size;
	data = (byte *)malloc(file_size + size + 10);
	if (data == NULL)
	{
		fclose(fd);
		return;
	}

	/* Fill data */
	fseek(fd, prev_size, SEEK_SET);
	fread(&data[size], 1, file_size, fd);
	memcpy(data, tag, size);
	fclose(fd);

	/* Write file */
	fd = fopen(filename, "wb");
	if (fd == NULL)
	{
		free(data);
		return;
	}
	fwrite(data, 1, file_size + size, fd);
	fclose(fd);
	free(data);
} /* End of 'mp3_save_tag' function */

/* Save song information */
void mp3_save_info( char *filename, song_info_t *info )
{
	struct id3_file *file;
	struct id3_tag *tag;
	byte *data = NULL;
	int size;
	char str[80];

	/* Open file and read current tag */
	file = id3_file_open(filename, ID3_FILE_MODE_READONLY);
	if (file == NULL)
		return;
	tag = id3_file_tag(file);
	if (tag == NULL)
	{
		id3_file_close(file);
		return;
	}

	/* Update tag fields */
	mp3_set_tag_frame(tag, ID3_FRAME_TITLE, info->m_name);
	mp3_set_tag_frame(tag, ID3_FRAME_ARTIST, info->m_artist);
	mp3_set_tag_frame(tag, ID3_FRAME_ALBUM, info->m_album);
	mp3_set_tag_frame(tag, ID3_FRAME_COMMENT, info->m_comments);
	mp3_set_tag_frame(tag, ID3_FRAME_YEAR, info->m_year);
	mp3_set_tag_frame(tag, ID3_FRAME_TRACK, info->m_track);
	if (info->m_genre == GENRE_ID_OWN_STRING)
		strcpy(str, info->m_genre_data.m_text);
	else
		sprintf(str, "%i", (info->m_genre == GENRE_ID_UNKNOWN) ? 
				info->m_genre_data.m_data : 
				mp3_glist->m_list[info->m_genre].m_data);
	mp3_set_tag_frame(tag, ID3_FRAME_GENRE, str);

	/* Get tag raw data */
	size = id3_tag_render(tag, NULL);
	if (size)
	{
		data = (byte *)malloc(size);
		id3_tag_render(tag, data);
	}

	/* Close file */
	id3_file_close(file);

	/* Save tag or save it later */
	if (!strcmp(filename, mp3_file_name))
	{
		mp3_tag = data;
		mp3_tag_size = size;
		memcpy(&mp3_cur_info, info, sizeof(song_info_t));
	}
	else
	{
		mp3_save_tag(filename, data, size);
		free(data);
	}
} /* End of 'mp3_save_info' function */

/* Extract a string from frame */
void mp3_extract_str_from_frame( char *str, struct id3_frame *f )
{
	char *dup_str;
	int pos;

	/* Get text encoding information */
	if (f->fields[0].type == ID3_FIELD_TYPE_TEXTENCODING)
	{
		if (f->fields[0].number.value != ID3_FIELD_TEXTENCODING_ISO_8859_1)
		{
			strcpy(str, "");
			return;
		}
		pos = 1;
	}
	else
		pos = 0;

	/* Fill string */
	switch (f->fields[pos].type)
	{
	case ID3_FIELD_TYPE_LATIN1:
	case ID3_FIELD_TYPE_LATIN1FULL:
		strcpy(str, f->fields[pos].latin1.ptr);
		break;
	case ID3_FIELD_TYPE_LATIN1LIST:
		if (!f->fields[pos].latin1list.nstrings)
			strcpy(str, "");
		else
			strcpy(str, f->fields[pos].latin1list.strings[0]);
		break;
	case ID3_FIELD_TYPE_STRING:
	case ID3_FIELD_TYPE_STRINGFULL:
		dup_str = id3_ucs4_latin1duplicate(f->fields[pos].string.ptr);
		strcpy(str, dup_str);
		free(dup_str);
		break;
	case ID3_FIELD_TYPE_STRINGLIST:
		if (!f->fields[pos].stringlist.nstrings)
			strcpy(str, "");
		else
		{
			dup_str = id3_ucs4_latin1duplicate(
				f->fields[pos].stringlist.strings[0]);
			strcpy(str, dup_str);
			free(dup_str);
		}
		break;
	default:
		strcpy(str, "");
		break;
	}
} /* End of 'mp3_extract_str_from_frame' function */

/* Get song information */
bool mp3_get_info( char *filename, song_info_t *info )
{
	struct id3_file *file;
	struct id3_tag *tag;
	int i;

	/* Return temporary tag if we request for current playing song tag */
	if (!strcmp(filename, mp3_file_name))
	{
		if (mp3_tag_present)
		{
			memcpy(info, &mp3_cur_info, sizeof(song_info_t));
			return TRUE;
		}
		else
			return FALSE;
	}

	/* Open file */
	file = id3_file_open(filename, ID3_FILE_MODE_READONLY);
	if (file == NULL)
	{
		memset(info, 0, sizeof(*info));
		return FALSE;
	}

	/* Read tag */
	tag = id3_file_tag(file);
	if (tag == NULL || !tag->nframes)
	{
		id3_file_close(file);
		memset(info, 0, sizeof(*info));
		return FALSE;
	}

	/* Initialize info fields with empty values first */
	memset(info, 0, sizeof(*info));
	info->m_genre = GENRE_ID_UNKNOWN;

	/* Scan tag frames */
	for ( i = 0; i < tag->nframes; i ++ )
	{
		struct id3_frame *f = tag->frames[i];
		if (!strcmp(f->id, ID3_FRAME_TITLE))
			mp3_extract_str_from_frame(info->m_name, f);
		else if (!strcmp(f->id, ID3_FRAME_ARTIST))
			mp3_extract_str_from_frame(info->m_artist, f);
		else if (!strcmp(f->id, ID3_FRAME_ALBUM))
			mp3_extract_str_from_frame(info->m_album, f);
		else if (!strcmp(f->id, ID3_FRAME_YEAR))
			mp3_extract_str_from_frame(info->m_year, f);
		else if (!strcmp(f->id, ID3_FRAME_TRACK))
			mp3_extract_str_from_frame(info->m_track, f);
		else if (!strcmp(f->id, ID3_FRAME_COMMENT))
			mp3_extract_str_from_frame(info->m_comments, f);
		else if (!strcmp(f->id, ID3_FRAME_GENRE))
		{
			char str[80];
			char *s;
			mp3_extract_str_from_frame(str, f);
			info->m_genre_data.m_data = strtol(str, &s, 10);
			if (!(*str) || *s)
			{
				info->m_genre = GENRE_ID_OWN_STRING;
				strcpy(info->m_genre_data.m_text, str);
			}
			else
				info->m_genre = 
					glist_get_id(mp3_glist, info->m_genre_data.m_data);
		}
	}

	/* Close file */
	id3_file_close(file);
	return TRUE;
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
		if (mp3_seek_val != -1)
		{
			fseek(mp3_fd, mp3_seek_val * mp3_bitrate / 8, SEEK_SET);
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
			read_size = fread(read_start, 1, read_size, mp3_fd);
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
			sample = mp3_mad_fixed_to_short(
					mp3_synth.pcm.samples[1][mp3_samples]);
		*(out_ptr ++) = sample & 0xFF;
		*(out_ptr ++) = sample >> 8;
		len += 2;
	}

	if (mp3_samples >= mp3_synth.pcm.length)
		mp3_samples = 0;

	return len;
} /* End of 'mp3_get_stream' function */

/* Seek song */
void mp3_seek( int shift )
{
	if (mp3_fd != NULL)
	{
		mp3_seek_val = shift;
	}
} /* End of 'mp3_seek' function */

/* Get audio parameters */
void mp3_get_audio_params( int *ch, int *freq, dword *fmt )
{
	*ch = mp3_channels;
	*freq = mp3_freq;
	*fmt = mp3_fmt;
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

/* Get functions list */
void inp_get_func_list( inp_func_list_t *fl )
{
	fl->m_start = mp3_start;
	fl->m_end = mp3_end;
	fl->m_get_stream = mp3_get_stream;
	fl->m_get_len = mp3_get_len;
	fl->m_get_info = mp3_get_info;
	fl->m_save_info = mp3_save_info;
	fl->m_seek = mp3_seek;
	fl->m_get_audio_params = mp3_get_audio_params;
	fl->m_get_formats = mp3_get_formats;
	fl->m_set_eq = mp3_set_eq;
	fl->m_glist = mp3_glist;
	fl->m_get_cur_time = mp3_get_cur_time;
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

/* Save variables list */
void inp_set_vars( cfg_list_t *list )
{
	mp3_var_list = list;
} /* End of 'inp_set_vars' function */

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
		
		sprintf(name, "eq_band%i", map[i] + 1);
		val = cfg_get_var_float(mp3_var_list, "eq_preamp") + 
			cfg_get_var_float(mp3_var_list, name);
		if (val > 18.)
			val = 18.;
		val = pow(10., val / 20.);
		mp3_eq_mul[i] = mad_f_tofixed(val);
	}
} /* End of 'mp3_set_eq' function */

/* Apply equalizer to frame */
void mp3_apply_eq( void )
{
	unsigned int nch, ch, ns, s, sb;

	/* Do nothing if equalizer is disabled */
	if (cfg_get_var_int(mp3_var_list, "equalizer_off"))
		return;
	
	nch = MAD_NCHANNELS(&mp3_frame.header);
	ns = MAD_NSBSAMPLES(&mp3_frame.header);
	for ( ch = 0; ch < nch; ++ch )
		for ( s = 0; s < ns; ++s )
			for ( sb = 0; sb < 32; ++sb )
				mp3_frame.sbsample[ch][s][sb] = 
					mad_f_mul(mp3_frame.sbsample[ch][s][sb], mp3_eq_mul[sb]);
} /* End of 'mp3_apply_eq' function */

/* End of 'mp3.c' file */

