/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : getlen.h
 * PURPOSE     : SG MPFC. MP3 plugin song length function
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2004
 * NOTE        : Code is taken from XMMS mpg123 plugin.
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
#include "types.h"
#include "getlen.h"
#include "file.h"

/* Some types */
typedef unsigned char guint8;
typedef unsigned long guint32;
typedef int guint;
typedef unsigned char guchar;
typedef double real;

/*
 * structure to receive extracted header
 */ 
typedef struct
{
	int frames;		/* total bit stream frames from Xing header data */
	int bytes;		/* total bit stream bytes from Xing header data */
	unsigned char toc[100];	/* "table of contents" */
} xing_header_t;

struct frame
{
	struct al_table *alloc;
	int (*synth) (real *, int, unsigned char *, int *);
	int (*synth_mono) (real *, unsigned char *, int *);
#ifdef USE_3DNOW
	void (*dct36)(real *,real *,real *,real *,real *);
#endif
	int stereo;
	int jsbound;
	int single;
	int II_sblimit;
	int down_sample_sblimit;
	int lsf;
	int mpeg25;
	int down_sample;
	int header_change;
	int lay;
	int (*do_layer) (struct frame * fr);
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int framesize;		/* computed framesize */
};

int tabsel_123[2][3][16] =
{
	{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
       {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}},

	{
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}}
};

long mpg123_freqs[9] =
{44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};

static int grp_3tab[32 * 3] =
{0,};				/* used: 27 */
static int grp_5tab[128 * 3] =
{0,};				/* used: 125 */
static int grp_9tab[1024 * 3] =
{0,};				/* used: 729 */

static int fsizeold = 0, ssize;
real mpg123_muls[27][64];	/* also used by layer 1 */

#define g_malloc malloc
#define g_free free

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define GET_INT32BE(b) \
(i = (b[0] << 24) | (b[1] << 16) | b[2] << 8 | b[3], b += 4, i)

#define MAXFRAMESIZE 1792

static guint32 convert_to_header(guint8 * buf)
{

	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

void mpg123_init_layer2(void)
{
	static double mulmul[27] = {
		0.0, -2.0 / 3.0, 2.0 / 3.0, 2.0 / 7.0, 2.0 / 15.0,
		2.0 / 31.0, 2.0 / 63.0, 2.0 / 127.0, 2.0 / 255.0,
		2.0 / 511.0, 2.0 / 1023.0, 2.0 / 2047.0, 2.0 / 4095.0,
		2.0 / 8191.0, 2.0 / 16383.0, 2.0 / 32767.0, 2.0 / 65535.0,
		-4.0 / 5.0, -2.0 / 5.0, 2.0 / 5.0, 4.0 / 5.0, -8.0 / 9.0,
		-4.0 / 9.0, -2.0 / 9.0, 2.0 / 9.0, 4.0 / 9.0, 8.0 / 9.0	};
	static int base[3][9] = {
		{1, 0, 2,},
		{17, 18, 0, 19, 20,},
		{21, 1, 22, 23, 0, 24, 25, 2, 26}};
	int i, j, k, l, len;
	real *table;
	static int tablen[3] = {3, 5, 9};
	static int *itable, *tables[3] =
	{grp_3tab, grp_5tab, grp_9tab};

	for (i = 0; i < 3; i++)
	{
		itable = tables[i];
		len = tablen[i];
		for (j = 0; j < len; j++)
			for (k = 0; k < len; k++)
				for (l = 0; l < len; l++)
				{
					*itable++ = base[i][l];
					*itable++ = base[i][k];
					*itable++ = base[i][j];
				}
	}

	for (k = 0; k < 27; k++)
	{
		double m = mulmul[k];

		table = mpg123_muls[k];
		for (j = 3, i = 0; i < 63; i++, j--)
			*table++ = m * pow(2.0, (double) j / 3.0);
		*table++ = 0.0;
	}
}

double mpg123_compute_bpf(struct frame *fr)
{
	double bpf;

	switch (fr->lay)
	{
		case 1:
			bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		default:
			bpf = 1.0;
	}

	return bpf;
}

int mpg123_get_xing_header(xing_header_t * xing, unsigned char *buf)
{	
	int i, head_flags;
	int id, mode;
	
	memset(xing, 0, sizeof(xing_header_t));
	
	/* get selected MPEG header data */ 
	id = (buf[1] >> 3) & 1;
	mode = (buf[3] >> 6) & 3;
	buf += 4;
	
	/* Skip the sub band data */
	if (id)
	{
		/* mpeg1 */
		if (mode != 3)
			buf += 32;
		else
			buf += 17;
	}
	else
	{
		/* mpeg2 */
		if (mode != 3)
			buf += 17;
		else
			buf += 9;
	}
	
	if (strncmp(buf, "Xing", 4))
		return 0;
	buf += 4;
		
	head_flags = GET_INT32BE(buf);
	
	if (head_flags & FRAMES_FLAG)
		xing->frames = GET_INT32BE(buf);
	if (xing->frames < 1)
		xing->frames = 1;
	if (head_flags & BYTES_FLAG)
		xing->bytes = GET_INT32BE(buf);
	
	if (head_flags & TOC_FLAG)
	{
		for (i = 0; i < 100; i++)
			xing->toc[i] = buf[i];
		buf += 100;
	}
	
#ifdef XING_DEBUG
	for (i = 0; i < 100; i++)
	{
		if ((i % 10) == 0)
			fprintf(stderr, "\n");
		fprintf(stderr, " %3d", xing->toc[i]);
	}
#endif
	
	return 1;
}

double mpg123_compute_tpf(struct frame *fr)
{
	const int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = bs[fr->lay];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

/*
 * the code a header and write the information
 * into the frame structure
 */
int mpg123_decode_header(struct frame *fr, unsigned long newhead)
{
	if (newhead & (1 << 20))
	{
		fr->lsf = (newhead & (1 << 19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
	}
	else
	{
		fr->lsf = 1;
		fr->mpeg25 = 1;
	}
	fr->lay = 4 - ((newhead >> 17) & 3);
	if (fr->mpeg25)
	{
		fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
	}
	else
		fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->lsf * 3);
	fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

	fr->bitrate_index = ((newhead >> 12) & 0xf);
	fr->padding = ((newhead >> 9) & 0x1);
	fr->extension = ((newhead >> 8) & 0x1);
	fr->mode = ((newhead >> 6) & 0x3);
	fr->mode_ext = ((newhead >> 4) & 0x3);
	fr->copyright = ((newhead >> 3) & 0x1);
	fr->original = ((newhead >> 2) & 0x1);
	fr->emphasis = newhead & 0x3;

	fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

	ssize = 0;

	if (!fr->bitrate_index)
		return (0);

	switch (fr->lay)
	{
		case 1:
			fr->do_layer = NULL;
			mpg123_init_layer2();	/* inits also shared tables with layer1 */
			fr->framesize = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
			break;
		case 2:
			fr->do_layer = NULL;
			mpg123_init_layer2();	/* inits also shared tables with layer1 */
			fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize += fr->padding - 4;
			break;
		case 3:
			fr->do_layer = NULL;
			if (fr->lsf)
				ssize = (fr->stereo == 1) ? 9 : 17;
			else
				ssize = (fr->stereo == 1) ? 17 : 32;
			if (fr->error_protection)
				ssize += 2;
			fr->framesize = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			fr->framesize = fr->framesize + fr->padding - 4;
			break;
		default:
			return (0);
	}
	if(fr->framesize > MAXFRAMESIZE)
		return 0;
	return 1;
}

int mpg123_head_check(unsigned long head)
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
	if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
		return FALSE;
	if ((head & 0xffff0000) == 0xfffe0000)
		return FALSE;
	
	return TRUE;
}

/* Get song length */
int mp3_get_len( char *filename )
{
	guint32 head;
	guchar tmp[4], *buf;
	struct frame frm;
	xing_header_t xing_header;
	double tpf, bpf;
	guint32 len;
	FILE *file;

	/* Supported only for regular files */
	if (file_get_type(filename) != FILE_TYPE_REGULAR)
		return 0;
	
	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;

	fseek(file, 0, SEEK_SET);
	if (fread(tmp, 1, 4, file) != 4)
		goto error;
	head = convert_to_header(tmp);
	while (!mpg123_head_check(head))
	{
		head <<= 8;
		if (fread(tmp, 1, 1, file) != 1)
			goto error;
		head |= tmp[0];
	}
	if (mpg123_decode_header(&frm, head))
	{
		buf = g_malloc(frm.framesize + 4);
		fseek(file, -4, SEEK_CUR);
		fread(buf, 1, frm.framesize + 4, file);
		tpf = mpg123_compute_tpf(&frm);
		if (mpg123_get_xing_header(&xing_header, buf))
		{
			g_free(buf);
			fclose(file);
			return ((guint) (tpf * xing_header.frames));
		}
		g_free(buf);
		bpf = mpg123_compute_bpf(&frm);
		fseek(file, 0, SEEK_END);
		len = ftell(file);
		fseek(file, -128, SEEK_END);
		fread(tmp, 1, 3, file);
		if (!strncmp(tmp, "TAG", 3))
			len -= 128;
		fclose(file);
		return ((guint) ((guint)(len / bpf) * tpf));
	}
error:
	if (file != NULL)
		fclose(file);
	return 0;
} /* End of 'mp3_get_len' function */

/* End of 'getlen.c' file */

