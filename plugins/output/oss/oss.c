/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. OSS output plugin functions implementation.
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

#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include "types.h"
#include "cfg.h"
#include "outp.h"
#include "pmng.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

/* Audio device file handle */
static int oss_fd = -1;

/* Some function declarations */
static bool_t oss_get_dev( char *name );

/* Plugins manager  */
static pmng_t *oss_pmng = NULL;

/* Plugin description */
static char *oss_desc = "OSS output plugin";

/* Plugin author */
static char *oss_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Configuration list */
static cfg_node_t *oss_cfg = NULL;

/* Start plugin */
bool_t oss_start( void )
{
	char name[MAX_FILE_NAME];

	/* Check if we have access to sound card */
	if (!oss_get_dev(name))
	{
		oss_fd = -1;
		return FALSE;
	}

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
	int v;

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
	int v;

	fd = open("/dev/mixer", O_RDONLY);
	if (fd < 0)
		return;
	ioctl(fd, SOUND_MIXER_READ_VOLUME, &v);
	close(fd);
	*left = ((v >> 8) & 0xFF);
	*right = (v & 0xFF);
} /* End of 'oss_get_volume' function */

/* Handle 'ok_clicked' message for configuration dialog */
wnd_msg_retcode_t oss_on_configure( wnd_t *wnd )
{
	editbox_t *eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "device"));
	assert(eb);
	cfg_set_var(oss_cfg, "device", EDITBOX_TEXT(eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'oss_on_configure' function */

/* Launch configuration dialog */
void oss_configure( wnd_t *parent )
{
	dialog_t *dlg;
	editbox_t *eb;

	dlg = dialog_new(parent, _("Configure OSS plugin"));
	eb = editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("&Device: "), 
			"device", cfg_get_var(oss_cfg, "device"), 'd', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", oss_on_configure);
	dialog_arrange_children(dlg);
} /* End of 'oss_configure' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = oss_desc;
	pd->m_author = oss_author;
	pd->m_configure = oss_configure;
	OUTP_DATA(pd)->m_start = oss_start;
	OUTP_DATA(pd)->m_end = oss_end;
	OUTP_DATA(pd)->m_play = oss_play;
	OUTP_DATA(pd)->m_set_channels = oss_set_channels;
	OUTP_DATA(pd)->m_set_freq = oss_set_freq;
	OUTP_DATA(pd)->m_set_fmt = oss_set_fmt;
	OUTP_DATA(pd)->m_flush = oss_flush;
	OUTP_DATA(pd)->m_set_volume = oss_set_volume;
	OUTP_DATA(pd)->m_get_volume = oss_get_volume;
	oss_pmng = pd->m_pmng;
	oss_cfg = pd->m_cfg;
} /* End of 'plugin_exchange_data' function */

/* Some function declarations */
bool_t oss_get_dev( char *name )
{
	char *dev, *s;
	int fd;
	
	/* Get respective variable value */
	dev = cfg_get_var(oss_cfg, "device");
	if (dev == NULL)
		dev = "/dev/dsp;/dev/dsp1";

	/* Search specified devices */
	for ( s = dev; *s; )
	{
		int i = 0;

		/* Get name */
		while ((*s) && ((*s) != ';'))
		{
			name[i ++] = *(s ++);
		}
		name[i] = 0;

		/* Check this device */
		fd = open(name, O_WRONLY | O_NONBLOCK);
		if (fd >= 0)
		{
			close(fd);
			return TRUE;
		}

		/* Skip symbols until slash */
		while ((*s) != '/' && (*s))
			s ++;
	}
	return FALSE;
} /* End of 'oss_get_dev' function */

/* End of 'oss.c' file */

