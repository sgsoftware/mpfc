/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * SG MPFC. Audio stream conversion functions.
 * Taken from XMMS.
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

#include <sys/soundcard.h>

void convert_free_buffer(void);

struct x_convert_buffers;

struct x_convert_buffers* x_convert_buffers_new(void);
/*
 * Free the data assosiated with the buffers, without destroying the
 * context.  The context can be reused.
 */
void x_convert_buffers_free(struct x_convert_buffers* buf);
void x_convert_buffers_destroy(struct x_convert_buffers* buf);


typedef int (*convert_func_t)(struct x_convert_buffers* buf, void **data, int length);
typedef int (*convert_channel_func_t)(struct x_convert_buffers* buf, void **data, int length);
typedef int (*convert_freq_func_t)(struct x_convert_buffers* buf, void **data, int length, int ifreq, int ofreq);


convert_func_t x_convert_get_func(int output, int input);
convert_channel_func_t x_convert_get_channel_func(int fmt, int output, int input);
convert_freq_func_t x_convert_get_frequency_func(int fmt, int channels);

/* End of 'xconvert.h' file */

