/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Disk writer output plugin functions implementation.
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/soundcard.h>
#include "types.h"
#include "file.h"
#include "outp.h"
#include "pmng.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_checkbox.h"
#include "wnd_editbox.h"
#include "wnd_filebox.h"
#include "util.h"

/* Header size */
#define DW_HEAD_SIZE 44

/* Default encoding fragment size (in seconds) */
#define DW_DEFAULT_FRAGMENT_SIZE 600

/* Output file handler */
static file_t *dw_fd = NULL;
static char dw_file_name[MAX_FILE_NAME];

/* Song parameters */
static short dw_channels = 2;
static long dw_freq = 44100;
static dword dw_fmt = 0;
static long dw_file_size = 0;

/* Plugin data */
static pmng_t *dw_pmng = NULL;
static cfg_node_t *dw_cfg, *dw_root_cfg;

/* Plugin description */
static char *dw_desc = "Disk Writer plugin";

/* Plugin author */
static char *dw_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Logger object */
static logger_t *dw_log = NULL;

/* Encoder thread data */
static pthread_t dw_encoder_tid = 0;
static bool_t dw_encoder_stop = FALSE;
static bool_t dw_encode = FALSE;
static int dw_fragment_index = 0, dw_head_fragment = 0, dw_fragment_size;

/* Forward declarations */
static void *dw_encoder_thread( void *arg );
static void dw_write_head( void );

/* Prepare a new file to write audio data to */
bool_t dw_prepare_file( void )
{
	char full_name[MAX_FILE_NAME], suffix[30] = ".wav";

	/* Get output file name */
	util_strncpy(full_name, dw_file_name, sizeof(full_name));
	if (dw_encode)
		snprintf(suffix, sizeof(suffix), "-%03d.wav", dw_fragment_index);
	strcat(full_name, suffix);

	/* Try to open file */
	dw_fd = file_open(full_name, "w+b", NULL);
	if (dw_fd == NULL)
	{
		logger_error(dw_log, 1, _("Unable to create file %s"), full_name);
		return FALSE;
	}

	/* Leave space for header */
	file_seek(dw_fd, DW_HEAD_SIZE, SEEK_SET);
	dw_file_size = DW_HEAD_SIZE;
	return TRUE;
} /* End of 'dw_prepare_file' function */

/* Finish work with a file */
void dw_finish_file( void )
{
	dw_write_head();
	file_close(dw_fd);
	dw_fd = NULL;
} /* End of 'dw_finish_file' function */

/* Start plugin */
bool_t dw_start( void )
{
	char name[MAX_FILE_NAME];
	char *str = NULL;
	int i;

	/* Get output file name (without extension and fragment index) */
	if (cfg_get_var_int(dw_cfg, "name-as-title"))
		str = cfg_get_var(dw_root_cfg, "cur-song-title");
	else 
		str = cfg_get_var(dw_root_cfg, "cur-song-name");
	if (str == NULL)
		return FALSE;
	util_strncpy(name, str, sizeof(name));
	str = strrchr(name, '.');
	if (str != NULL)
		*str = 0;
	util_replace_char(name, ':', '_');
	str = cfg_get_var(dw_cfg, "path");
	if (str != NULL)
		snprintf(dw_file_name, sizeof(dw_file_name), "%s/%s", str, name);
	else
		snprintf(dw_file_name, sizeof(dw_file_name), "%s", name);

	/* Check if we should encode the stream */
	if (cfg_get_var_bool(dw_cfg, "encode"))
	{
		dw_encode = TRUE;
		dw_fragment_index = 0;

		/* Get one fragment size and convert to the respective file size */
		dw_fragment_size = cfg_get_var_int(dw_cfg, "fragment-size");
		if (dw_fragment_size <= 0)
			dw_fragment_size = DW_DEFAULT_FRAGMENT_SIZE;
		dw_fragment_size *= (dw_freq * dw_channels);
		if (dw_fmt != AFMT_U8 && dw_fmt != AFMT_S8)
			dw_fragment_size *= 2;

		/* Start encoder thread */
		dw_encoder_tid = 0;
		dw_encoder_stop = FALSE;
		dw_head_fragment = 0;
		if (pthread_create(&dw_encoder_tid, NULL, dw_encoder_thread, NULL))
			dw_encode = FALSE;
	}
	
	/* Prepare the file */
	if (!dw_prepare_file())
		return FALSE;
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
		dw_finish_file();
	if (dw_encoder_tid != 0)
	{
		dw_encoder_stop = TRUE;
		pthread_join(dw_encoder_tid, NULL);
		dw_encoder_tid = 0;
	}
} /* End of 'dw_end' function */

/* Play stream */
void dw_play( void *buf, int size )
{
	if (dw_fd == NULL)
		return;
	
	file_write(buf, 1, size, dw_fd);
	dw_file_size += size;
	if (dw_encode && dw_file_size >= dw_fragment_size)
	{
		dw_finish_file();
		dw_fragment_index ++;
		dw_prepare_file();
	}
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

/* Encoder thread function */
static void *dw_encoder_thread( void *arg )
{
	for ( ;; )
	{
		/* Do encoding */
		while ((dw_head_fragment < dw_fragment_index) ||
				(dw_head_fragment == dw_fragment_index && dw_encoder_stop))
		{
			char cmd[MAX_FILE_NAME], file_name[MAX_FILE_NAME], 
				 full_name[MAX_FILE_NAME];
			char *cmd_format;
			int cmd_len;

			/* Construct file name */
			snprintf(file_name, sizeof(file_name), "%s-%03d", dw_file_name,
					dw_head_fragment);
			snprintf(full_name, sizeof(full_name), "%s.wav", file_name);

			/* Format encoder command */
			cmd_format = cfg_get_var(dw_cfg, "encode-command");
			if (cmd_format != NULL && strcmp(cmd_format, ""))
			{
				/* Substitute tokens in the format string */
				for ( cmd_len = 0; *cmd_format; cmd_format ++ )
				{
					/* Common character */
					if ((*cmd_format) != '%')
						cmd[cmd_len ++] = (*cmd_format);
					/* A token */
					else
					{
						char ch;

						/* Get format symbol */
						cmd_format ++;
						ch = (*cmd_format);

						/* Input file name */
						if (ch == 'i')
						{
							strncpy(&cmd[cmd_len], full_name, 
									sizeof(cmd) - cmd_len);
							cmd_len += strlen(full_name);
						}
						/* Output file name (without extension) */
						else if (ch == 'o') 
						{
							strncpy(&cmd[cmd_len], file_name, 
									sizeof(cmd) - cmd_len);
							cmd_len += strlen(file_name);
						}
						/* Some other symbol */
						else
							cmd[cmd_len ++] = ch;
					}
					if (cmd_len >= sizeof(cmd))
					{
						cmd_len = sizeof(cmd) - 1;
						break;
					}
				}
				cmd[cmd_len] = 0;
			}
			/* Use default coder */
			else
			{
				snprintf(cmd, sizeof(cmd), "oggenc -Q \"%s\"", full_name);
			}
			logger_status_msg(dw_log, 1, _("Encoding: %s"), cmd);
			if (system(cmd) < 0)
			{
				logger_error(dw_log, 1, _("Encoder command failed"));
			}
			unlink(full_name);
			dw_head_fragment ++;
		}
		if (dw_encoder_stop)
			break;
		util_wait();
	}
	return NULL;
} /* End of 'dw_encoder_thread' function */

/* Handle 'ok_clicked' message for configuration dialog */
wnd_msg_retcode_t dw_on_configure( wnd_t *wnd )
{
	editbox_t *eb_path, *eb_fragment, *eb_command;
	checkbox_t *cb_encode;
	
	eb_path = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "path"));
	cb_encode = CHECKBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "encode"));
	eb_fragment = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd),
				"fragment_size"));
	eb_command = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), 
				"encode_command"));
	assert(eb_path && eb_fragment && eb_command && cb_encode);
	cfg_set_var(dw_cfg, "path", EDITBOX_TEXT(eb_path));
	cfg_set_var_bool(dw_cfg, "encode", cb_encode->m_checked);
	cfg_set_var(dw_cfg, "fragment-size", EDITBOX_TEXT(eb_fragment));
	cfg_set_var(dw_cfg, "encode-command", EDITBOX_TEXT(eb_command));
	return WND_MSG_RETCODE_OK;
} /* End of 'dw_on_configure' function */

/* Launch configuration dialog */
void dw_configure( wnd_t *parent )
{
	dialog_t *dlg;
	vbox_t *vbox;

	dlg = dialog_new(parent, _("Configure disk writer plugin"));
	filebox_new_with_label(WND_OBJ(dlg->m_vbox), _("P&ath to store: "), 
			"path", cfg_get_var(dw_cfg, "path"), 'a', 50);
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("Encoder"), 0);
	checkbox_new(WND_OBJ(vbox), _("&Enable encoder"),
			"encode", 'e', cfg_get_var_bool(dw_cfg, "encode"));
	editbox_new_with_label(WND_OBJ(vbox), _("F&ragment size: "), 
			"fragment_size", cfg_get_var(dw_cfg, "fragment-size"), 'r', 50);
	editbox_new_with_label(WND_OBJ(vbox), _("Encoder c&ommand: "),
			"encode_command", cfg_get_var(dw_cfg, "encode-command"), 'o', 50);
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", dw_on_configure);
	dialog_arrange_children(dlg);
} /* End of 'dw_configure' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = dw_desc;
	pd->m_author = dw_author;
	pd->m_configure = dw_configure;
	OUTP_DATA(pd)->m_start = dw_start;
	OUTP_DATA(pd)->m_end = dw_end;
	OUTP_DATA(pd)->m_play = dw_play;
	OUTP_DATA(pd)->m_set_channels = dw_set_channels;
	OUTP_DATA(pd)->m_set_freq = dw_set_freq;
	OUTP_DATA(pd)->m_set_fmt = dw_set_fmt;
	OUTP_DATA(pd)->m_flags = OUTP_NO_SOUND;
	dw_pmng = pd->m_pmng;
	dw_cfg = pd->m_cfg;
	dw_root_cfg = pd->m_root_cfg;
	dw_log = pd->m_logger;
} /* End of 'plugin_exchange_data' function */

/* End of 'writer.c' file */

