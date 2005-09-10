/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Audio CD input plugin functions implementation.
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

#include <stdio.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include "types.h"
#include "audiocd.h"
#include "cddb.h"
#include "inp.h"
#include "pmng.h"
#include "song.h"
#include "song_info.h"
#include "util.h"
#include "wnd.h"
#include "wnd_combobox.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

/* Get track number from filename */
#define acd_fname2trk(name) (strncmp(name, "/track", 6) ? -1 : \
		atoi(&(name)[6]) - 1)

/* Calculate track length */
#define acd_get_trk_len(trk) \
	((acd_tracks_info[trk].m_end.minute * 60 + acd_tracks_info[trk].m_end.second) - \
	((acd_tracks_info[trk].m_start.minute * 60 + \
	  acd_tracks_info[trk].m_start.second)))

/* Get address in frames */
#define ACD_LBA(msf)	(((msf).minute * 60 + (msf).second) * 75 + (msf).frame)

/* Plugins manager */
pmng_t *acd_pmng = NULL;

/* Tracks information array */
struct acd_trk_info_t acd_tracks_info[ACD_MAX_TRACKS];
int acd_num_tracks = 0;
static int acd_cur_track = -1;
static bool_t acd_info_read = FALSE;

/* Current time */
static int acd_time = 0;

/* Whether running first time? */
static bool_t acd_first_time = TRUE;

/* The next track */
static char acd_next_song[MAX_FILE_NAME] = "";

/* Audio device */
static int audio_fd = -1;

/* Logger */
logger_t *acd_log = NULL;

/* Plugin description */
static char *acd_desc = "AudioCD playback plugin";

/* Configuration list */
cfg_node_t *acd_cfg = NULL;

/* Plugin author */
static char *acd_author = "Sergey E. Galanov <sgsoftware@mail.ru>";

/* Prepare cdrom for ioctls */
static int acd_prepare_cd( void )
{
	int fd;
	char *dev;

	/* Get device name */
	dev = cfg_get_var(acd_cfg, "device");
	if (dev == NULL)
		dev = "/dev/cdrom";
	
	/* Open device */
	fd = open(dev, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return fd;

	/* Check drive */
	if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) != CDS_DISC_OK)
	{
		close(fd);
		return -1;
	}
	return fd;
} /* End of 'acd_prepare_cd' function */

/* Start play function */
bool_t acd_start( char *filename )
{
	int fd, track;
	struct cdrom_msf msf;
	struct cdrom_subchnl info;
	bool_t playing = FALSE;
	int mixer_fd;
	int format = AFMT_S16_LE, ch = 2, rate = 44100;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return FALSE;

	/* Load tracks information */
	acd_load_tracks(fd);

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return FALSE;
	acd_cur_track = track;

	/* Start playing */
	msf.cdmsf_min0 = acd_tracks_info[track].m_start.minute;
	msf.cdmsf_sec0 = acd_tracks_info[track].m_start.second;
	msf.cdmsf_frame0 = acd_tracks_info[track].m_start.frame;
	msf.cdmsf_min1 = acd_tracks_info[track].m_end.minute;
	msf.cdmsf_sec1 = acd_tracks_info[track].m_end.second;
	msf.cdmsf_frame1 = acd_tracks_info[track].m_end.frame;
	if (ioctl(fd, CDROMPLAYMSF, &msf) < 0)
	{
		close(fd);
		return FALSE;
	}
	acd_time = 0;
	acd_info_read = FALSE;
	util_strncpy(acd_next_song, "", sizeof(acd_next_song));

	/* Close device */
	close(fd);

	/* Set recording source as cd */
	mixer_fd = open("/dev/mixer", O_WRONLY);
	if (mixer_fd >= 0)
	{
		int mask = SOUND_MASK_CD;
		ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &mask);
		close(mixer_fd);
	}

	/* Open audio device (for reading audio data) */
	audio_fd = open("/dev/dsp", O_RDONLY);
	if (audio_fd >= 0)
	{
		ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format);
		ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &ch);
		ioctl(audio_fd, SNDCTL_DSP_SPEED, &rate);
	}
	
	/* Open audio device */
	return TRUE;
} /* End of 'acd_start' function */

/* End playing function */
void acd_end( void )
{
	int fd;

	/* Open device */
	acd_time = 0;
	acd_info_read = FALSE;
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Stop playing */
	ioctl(fd, CDROMPAUSE, 0);

	/* Close device */
	close(fd);
	cddb_free();

	if (audio_fd >= 0)
	{
		close(audio_fd);
		audio_fd = -1;
	}
} /* End of 'acd_end' function */

/* Get length */
static int acd_get_len( char *filename )
{
	int track;
	
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return 0;
	return acd_tracks_info[track].m_len;
} /* End of 'acd_get_len' function */

/* Get stream function */
int acd_get_stream( void *buf, int size )
{
	int fd;
	struct cdrom_subchnl info;

	/* Check if we are playing now */
	if ((fd = acd_prepare_cd()) < 0)
		return 0;
	info.cdsc_format = CDROM_MSF;
	if (ioctl(fd, CDROMSUBCHNL, &info) < 0)
	{
		close(fd);
		return 0;
	}
	close(fd);
	if (info.cdsc_audiostatus == CDROM_AUDIO_COMPLETED ||
			info.cdsc_audiostatus == CDROM_AUDIO_ERROR)
		return 0;
	acd_time = (ACD_LBA(info.cdsc_absaddr.msf) - 
		ACD_LBA(acd_tracks_info[acd_cur_track].m_start)) / 75;
	if (ACD_LBA(info.cdsc_absaddr.msf) >=
			ACD_LBA(acd_tracks_info[acd_cur_track].m_end) - 20)
		return 0;

	/* Read audio data */
	if (audio_fd >= 0)
	{
		int ret_size = read(audio_fd, buf, size);
		if (ret_size > 0)
			size = ret_size;
	}
	return size;
} /* End of 'acd_get_stream' function */

/* Pause */
void acd_pause( void )
{
	int fd;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Pause */
	if (ioctl(fd, CDROMPAUSE, 0) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_pause' function */

/* Resume */
void acd_resume( void )
{
	int fd;

	/* Open device */
	if ((fd = acd_prepare_cd()) < 0)
		return;

	/* Pause */
	if (ioctl(fd, CDROMRESUME, 0) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_resume' function */

/* Seek */
void acd_seek( int shift )
{
	int fd;
	struct cdrom_subchnl info;
	struct cdrom_msf msf;

	if (acd_cur_track < 0 || acd_cur_track >= acd_num_tracks)
		return;

	/* Start playing from new position */
	if ((fd = acd_prepare_cd()) < 0)
		return;
	shift = acd_tracks_info[acd_cur_track].m_start.minute * 60 + 
		acd_tracks_info[acd_cur_track].m_start.second + shift;
	msf.cdmsf_min0 = shift / 60;
	msf.cdmsf_sec0 = shift % 60;
	msf.cdmsf_frame0 = 0;
	msf.cdmsf_min1 = acd_tracks_info[acd_cur_track].m_end.minute;
	msf.cdmsf_sec1 = acd_tracks_info[acd_cur_track].m_end.second;
	msf.cdmsf_frame1 = acd_tracks_info[acd_cur_track].m_end.frame;
	if (ioctl(fd, CDROMPLAYMSF, &msf) < 0)
	{
		close(fd);
		return;
	}

	/* Close device */
	close(fd);
} /* End of 'acd_seek' function */

/* Get audio parameters */
void acd_get_audio_params( int *ch, int *freq, dword *fmt, int *bitrate )
{
	*ch = 2;
	*freq = 44100;
	*fmt = 0;
	*bitrate = 0;
} /* End of 'acd_get_audio_params' function */

/* Get current time */
int acd_get_cur_time( void ) 
{
	return acd_time;
} /* End of 'acd_get_cur_time' function */

/* Get song info */
song_info_t *acd_get_info( char *filename, int *len )
{
	int track;

	/* Get length */
	*len = acd_get_len(filename);

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return NULL;

	/* Read whole disc info */
	if (!cddb_read())
		return si_new();

	/* Save info for specified track */
	return cddb_get_trk_info(track);
} /* End of 'acd_get_info' function */

/* Save song info */
bool_t acd_save_info( char *filename, song_info_t *info )
{
	int track;

	/* Get track number from filename */
	track = acd_fname2trk(filename);
	if (track < 0 || track >= acd_num_tracks || 
			track > acd_tracks_info[acd_num_tracks - 1].m_number)
		return;

	/* Save info */
	return cddb_save_trk_info(track, info);
} /* End of 'acd_save_info' function */

/* Set next song name */
void acd_set_next_song( char *name )
{
	if (name == NULL)
		strcpy(acd_next_song, "");
	else
		util_strncpy(acd_next_song, name, sizeof(acd_next_song));
} /* End of 'acd_set_next_song' function */

/* Open directory */
void *acd_opendir( char *name )
{
	acd_dir_data_t *data;
	int fd;

	/* Load tracks */
	fd = acd_prepare_cd();
	if (fd < 0)
		return NULL;
	acd_load_tracks(fd);
	close(fd);
	if (acd_num_tracks == 0)
		return NULL;
	
	/* Create directory data */
	data = (acd_dir_data_t *)malloc(sizeof(*data));
	data->m_next_track = 0;
	return data;
} /* End of 'acd_opendir' function */

/* Close directory */
void acd_closedir( void *dir )
{
	assert(dir);
	free(dir);
} /* End of 'acd_closedir' function */

/* Read directory entry */
char *acd_readdir( void *dir )
{
	acd_dir_data_t *data = (acd_dir_data_t *)dir;

	assert(dir);

	/* Finish */
	if (data->m_next_track >= acd_num_tracks)
		return NULL;

	/* Return next track name */
	return acd_tracks_info[data->m_next_track ++].m_name;
} /* End of 'acd_readdir' function */

/* Get file parameters */
int acd_stat( char *name, struct stat *sb )
{
	memset(sb, 0, sizeof(*sb));
	if (!strcmp(name, "/"))
	{
		sb->st_mode = S_IFDIR;
		return 0;
	}
	else if (!strncmp(name, "/track", 6))
	{
		int track = (name[6] - '0') * 10 + (name[7] - '0');
		if (track >= 1 && track <= acd_num_tracks)
		{
			sb->st_mode = S_IFREG;
			return 0;
		}
	}
	return ENOENT;
} /* End of 'acd_stat' function */

/* Get plugin mixer type */
plugin_mixer_type_t acd_get_mixer_type( void )
{
	return PLUGIN_MIXER_CD;
} /* End of 'acd_get_mixer_type' function */

/* Handle 'ok_clicked' message for configuration dialog */
wnd_msg_retcode_t acd_on_configure( wnd_t *wnd )
{
	editbox_t *device_eb, *host_eb, *cat_eb, *email_eb;
	
	device_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "device"));
	host_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "host"));
	cat_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "category"));
	email_eb = EDITBOX_OBJ(dialog_find_item(DIALOG_OBJ(wnd), "email"));
	assert(device_eb && host_eb && cat_eb && email_eb);
	cfg_set_var(acd_cfg, "device", EDITBOX_TEXT(device_eb));
	cfg_set_var(acd_cfg, "cddb-host", EDITBOX_TEXT(host_eb));
	cfg_set_var(acd_cfg, "cddb-email", EDITBOX_TEXT(email_eb));
	cfg_set_var(acd_cfg, "cddb-category", EDITBOX_TEXT(cat_eb));
	return WND_MSG_RETCODE_OK;
} /* End of 'acd_on_configure' function */

/* Launch configuration dialog */
void acd_configure( wnd_t *parent )
{
	dialog_t *dlg;
	combo_t *category;
	vbox_t *vbox;
	int i;

	dlg = dialog_new(parent, _("Configure AudioCD plugin"));
	editbox_new_with_label(WND_OBJ(dlg->m_vbox), _("CD &device: "), 
			"device", cfg_get_var(acd_cfg, "device"), 'd', 50);
	vbox = vbox_new(WND_OBJ(dlg->m_vbox), _("CDDB parameters"), 0);
	editbox_new_with_label(WND_OBJ(vbox), _("&Host: "), 
			"host", cfg_get_var(acd_cfg, "cddb-host"), 'h', 50);
	editbox_new_with_label(WND_OBJ(vbox), _("&Email: "), 
			"email", cfg_get_var(acd_cfg, "cddb-email"), 'e', 50);
	category = combo_new_with_label(WND_OBJ(vbox), _("Disc c&ategory: "),
			"category", "", 'a', 50, cddb_num_cats + 1);
	for ( i = 0; i < cddb_num_cats; i ++ )
		combo_add_item(category, cddb_cats[i]);
	editbox_set_text(EDITBOX_OBJ(category), 
			cfg_get_var(acd_cfg, "cddb-category"));
	wnd_msg_add_handler(WND_OBJ(dlg), "ok_clicked", acd_on_configure);
	dialog_arrange_children(dlg);
} /* End of 'acd_configure' function */

/* Exchange data with main program */
void plugin_exchange_data( plugin_data_t *pd )
{
	pd->m_desc = acd_desc;
	pd->m_author = acd_author;
	pd->m_configure = acd_configure;
	INP_DATA(pd)->m_start = acd_start;
	INP_DATA(pd)->m_end = acd_end;
	INP_DATA(pd)->m_get_stream = acd_get_stream;
	INP_DATA(pd)->m_seek = acd_seek;
	INP_DATA(pd)->m_get_audio_params = acd_get_audio_params;
	INP_DATA(pd)->m_flags = INP_OWN_SOUND | INP_VFS;
	INP_DATA(pd)->m_pause = acd_pause;
	INP_DATA(pd)->m_resume = acd_resume;
	INP_DATA(pd)->m_get_cur_time = acd_get_cur_time;
	INP_DATA(pd)->m_get_info = acd_get_info;
	INP_DATA(pd)->m_save_info = acd_save_info;
	INP_DATA(pd)->m_set_song_title = acd_set_song_title;
	INP_DATA(pd)->m_set_next_song = acd_set_next_song;
	INP_DATA(pd)->m_vfs_opendir = acd_opendir;
	INP_DATA(pd)->m_vfs_closedir = acd_closedir;
	INP_DATA(pd)->m_vfs_readdir = acd_readdir;
	INP_DATA(pd)->m_vfs_stat = acd_stat;
	INP_DATA(pd)->m_get_mixer_type = acd_get_mixer_type;
	acd_pmng = pd->m_pmng;
	acd_log = pd->m_logger;
	acd_cfg = pd->m_cfg;

	INP_DATA(pd)->m_num_spec_funcs = 2;
	INP_DATA(pd)->m_spec_funcs = 
		(inp_spec_func_t *)malloc(sizeof(inp_spec_func_t) * 
								  INP_DATA(pd)->m_num_spec_funcs);
	INP_DATA(pd)->m_spec_funcs[0].m_title = strdup(_("Reload info from CDDB"));
	INP_DATA(pd)->m_spec_funcs[0].m_flags = 0;
	INP_DATA(pd)->m_spec_funcs[0].m_func = cddb_reload;
	INP_DATA(pd)->m_spec_funcs[1].m_title = strdup(_("Submit info to CDDB"));
	INP_DATA(pd)->m_spec_funcs[1].m_flags = INP_SPEC_SAVE_INFO;
	INP_DATA(pd)->m_spec_funcs[1].m_func = cddb_submit;
} /* End of 'plugin_exchange_data' function */

/* Set song title */
str_t *acd_set_song_title( char *filename )
{
	int track = acd_fname2trk(filename);
	str_t *title = str_new("");
	str_printf(title, "Track %02d", track + 1);
	return title;
} /* End of 'acd_set_song_title' function */

/* Load tracks information */
void acd_load_tracks( int fd )
{
	int i;

	/* Read tracks information */
	if (acd_first_time || ioctl(fd, CDROM_MEDIA_CHANGED, 0))
	{
		struct cdrom_tochdr toc;
		struct cdrom_tocentry entry;

		ioctl(fd, CDROMREADTOCHDR, &toc);
		acd_num_tracks = toc.cdth_trk1 - toc.cdth_trk0 + 1;
		entry.cdte_format = CDROM_MSF;
		for ( i = 0; i < acd_num_tracks; i ++ )
		{
			entry.cdte_track = i + toc.cdth_trk0;
			ioctl(fd, CDROMREADTOCENTRY, &entry);
			acd_tracks_info[i].m_start = entry.cdte_addr.msf;
			acd_tracks_info[i].m_data = entry.cdte_ctrl & CDROM_DATA_TRACK;
			acd_tracks_info[i].m_number = i + toc.cdth_trk0;
			snprintf(acd_tracks_info[i].m_name, 
					sizeof(acd_tracks_info[i].m_name), "track%02d", i + 1);
		}
		for ( i = 0; i < acd_num_tracks - 1; i ++ )
		{
			acd_tracks_info[i].m_end = acd_tracks_info[i + 1].m_start;
			acd_tracks_info[i].m_len = acd_get_trk_len(i);
		}
		entry.cdte_track = CDROM_LEADOUT;
		ioctl(fd, CDROMREADTOCENTRY, &entry);
		acd_tracks_info[i].m_end = entry.cdte_addr.msf;
		acd_tracks_info[i].m_len = acd_get_trk_len(i);
		acd_first_time = FALSE;

		/* Free CDDB info */
		cddb_free();
	}
} /* End of 'acd_load_tracks' function */

/* End of 'audiocd.c' file */

