/******************************************************************
 * Copyright (C) 2003 - 2013 by SG Software.
 *
 * SG MPFC. Interface for main player functions.
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

#ifndef __SG_MPFC_PLAYER_H__
#define __SG_MPFC_PLAYER_H__

#include "types.h"
#include "cfg.h"
#include "command.h"
#include "logger.h"
#include "logger_view.h"
#include "main_types.h"
#include "plist.h"
#include "pmng.h"
#include "undo.h"
#include "wnd.h"
#include "wnd_dialog.h"
#include "wnd_editbox.h"

/* History lists stuff */
#define PLAYER_HIST_LIST_ADD  		0
#define PLAYER_HIST_LIST_ADD_OBJ 	1
#define PLAYER_HIST_LIST_SAVE		2
#define PLAYER_HIST_LIST_VAR_NAME	3
#define PLAYER_HIST_LIST_VAR_VAL	4
#define PLAYER_HIST_LIST_SEARCH		5
#define PLAYER_HIST_LIST_EXEC		6
#define PLAYER_HIST_FB_PATTERN		7
#define PLAYER_HIST_FB_CD			8
#define PLAYER_NUM_HIST_LISTS 		9

/* Sliders parameters */
#define PLAYER_SLIDER_TIME_Y 2
#define PLAYER_SLIDER_TIME_X 0
#define PLAYER_SLIDER_TIME_W (WND_WIDTH(player_wnd) - 24)
#define PLAYER_SLIDER_VOL_Y  2
#define PLAYER_SLIDER_VOL_X  (WND_WIDTH(player_wnd) - 22)
#define PLAYER_SLIDER_VOL_W  20

/* Player window user messages IDs */
#define PLAYER_MSG_INFO			0
#define PLAYER_MSG_NEXT_FOCUS	1

/* Max number of enqueued songs */
#define PLAYER_MAX_ENQUEUED 	20

/* Player window type */
typedef struct
{
	/* Window part */
	wnd_t m_wnd;

	/* Message handlers */
	wnd_msg_handler_t *m_on_command;
} player_wnd_t;
#define PLAYER_WND(wnd)	((player_wnd_t *)wnd)

/***
 * Global variables
 ***/

/* Undo list */
extern undo_list_t *player_ul;

/* Play list */
extern plist_t *player_plist;

/* Player context */
extern player_context_t *player_context;

/* Do we story undo information now? */
extern bool_t player_store_undo;

/* Edit boxes history lists */
extern editbox_history_t *player_hist_lists[PLAYER_NUM_HIST_LISTS];

/* Plugins manager */
extern pmng_t *player_pmng;

/* User configuration file name */
extern char player_cfg_file[MAX_FILE_NAME];

/* Previous file browser session directory name */
extern char player_fb_dir[MAX_FILE_NAME];

/* Root window */
extern wnd_t *wnd_root;

/* Play list window */
extern wnd_t *player_wnd;

/* Configuration list */
extern cfg_node_t *cfg_list;

/* Logger */
extern logger_t *player_log;
extern logger_view_t *player_logview;

/* enqueued songs - max of PLAYER_MAX_ENQUEUED */
extern int queued_songs[PLAYER_MAX_ENQUEUED];
extern int num_queued_songs;

/***
 * Initialization/deinitialization functions
 ***/

/* Initialize player */
bool_t player_init( int argc, char *argv[] );

/* Unitialize player */
void player_deinit( void );

/* Root window destructor */
void player_root_destructor( wnd_t *wnd );

/* Initialize the player window */
player_wnd_t *player_wnd_new( wnd_t *parent );

/* Run player */
bool_t player_run( void );

/* Initialize configuration */
bool_t player_init_cfg( void );

/* Parse program command line */
bool_t player_parse_cmd_line( int argc, char *argv[] );

/* Save configuration */
void player_save_cfg( void );

/***
 * Message handlers
 ***/

/* Display player function */
wnd_msg_retcode_t player_on_display( wnd_t *wnd );

/* Display slider */
void player_display_slider( wnd_t *wnd, int x, int y, int width, 
	   double pos, double range );

/* Handle new log message */
void player_on_log_msg( logger_t *log, void *data, 
		struct logger_message_t *msg );

/* Play list window closing handler */
wnd_msg_retcode_t player_on_close( wnd_t *wnd );

/* Handle action */
wnd_msg_retcode_t player_on_action( wnd_t *wnd, char *action, int repval );

/* Handle left-button click */
wnd_msg_retcode_t player_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle middle-button click */
wnd_msg_retcode_t player_on_mouse_mdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle left-button double click */
wnd_msg_retcode_t player_on_mouse_ldouble( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle user message */
wnd_msg_retcode_t player_on_user( wnd_t *wnd, int id, void *data );

/* Signal handler */
void player_handle_signal( int signum );

/***
 * Playing-related functions
 ***/

/* Play song */
void player_play( int song, song_time_t start_time );

/* Seek song */
void player_seek( song_time_t val, bool_t rel );

/* Skip some songs */
int player_skip_songs( int num, bool_t play );

/* Go to next next track */
void player_next_track( void );

/* Start track */
void player_set_track( int track );

/* Set volume */
void player_set_vol( double vol, bool_t rel );

/* Update volume */
void player_update_vol( void );

/* End play song thread */
void player_end_play( bool_t rem_cur_song );

/* Stop timer thread */
void player_stop_timer( void );

/* Timer thread function */
void *player_timer_func( void *arg );

/* Translate projected song time to real time */
song_time_t player_translate_time( song_t *s, song_time_t t, bool_t virtual2real );

/* Player thread function */
void *player_thread( void *arg );

/***
 * Dialogs launching functions
 ***/

/* Launch song adding dialog box */
void player_add_dialog( void );

/* Launch save play list dialog */
void player_save_dialog( void );

/* Launch remove song(s) dialog */
void player_rem_dialog( void );

/* Launch an external command execution dialog */
void player_exec_dialog( void );

/* Launch sort play list dialog */
void player_sort_dialog( void );

/* Launch song info dialog */
void player_info_dialog( void );

/* Fill info dialog with values */
bool_t player_info_dialog_fill( dialog_t *dlg, bool_t first_call );

/* Set gray-non-modified flag for info dialog edit box */
void player_info_dlg_change_eb_gray( dialog_t *dlg, char *id, bool_t gray );

/* Launch info reload dialog */
void player_info_reload_dialog( void );

/* Launch search play list dialog */
void player_search_dialog( void );

/* Launch advanced search dialog */
void player_advanced_search_dialog( void );

/* Launch variables manager */
void player_var_manager( void );

/* Launch test management dialog */
void player_test_dialog( void );

/* Launch plugin manager dialog */
void player_pmng_dialog( void );

/* Synchronize plugin manager dialog items with current item */
void player_pmng_dialog_sync( dialog_t *dlg );

/***
 * Dialog message handlers
 ***/

/* Handle 'ok_clicked' for add songs dialog */
wnd_msg_retcode_t player_on_add( wnd_t *wnd );

/* Handle 'ok_clicked' for save dialog */
wnd_msg_retcode_t player_on_save( wnd_t *wnd );

/* Handle 'ok_clicked' for execution dialog */
wnd_msg_retcode_t player_on_exec( wnd_t *wnd );

/* Handle 'ok_clicked' for sort dialog */
wnd_msg_retcode_t player_on_sort( wnd_t *wnd );

/* Handle 'ok_clicked' for info dialog */
wnd_msg_retcode_t player_on_info( wnd_t *wnd );

/* Save the info dialog contents */
void player_save_info_dialog( dialog_t *dlg );

/* Destructor for info dialog */
void player_on_info_close( wnd_t *wnd );

/* Handle 'clicked' for info dialog reload button */
wnd_msg_retcode_t player_on_info_dlg_reload( wnd_t *wnd );

/* Handle 'clicked' for info dialog write-in-all checkbox */
wnd_msg_retcode_t player_on_info_cb_clicked( wnd_t *wnd );

/* Handle 'ok_clicked' for search dialog */
wnd_msg_retcode_t player_on_search( wnd_t *wnd );

/* Handle 'ok_clicked' for advanced search dialog */
wnd_msg_retcode_t player_on_adv_search( wnd_t *wnd );

/* Handle 'ok_clicked' for info reload dialog */
wnd_msg_retcode_t player_on_info_reload( wnd_t *wnd );

/* Handle 'ok_clicked' for variables manager */
wnd_msg_retcode_t player_on_var( wnd_t *wnd );

/* Handle 'clicked' for variables manager view value button */
wnd_msg_retcode_t player_on_var_view( wnd_t *wnd );

/* Handle 'ok_clicked' for repeat value dialog box */
wnd_msg_retcode_t player_repval_on_ok( wnd_t *wnd );

/* Handle 'clicked' for stop test button */
wnd_msg_retcode_t player_on_test_stop( wnd_t *wnd );

/* Handle 'ok_clicked' for test dialog box */
wnd_msg_retcode_t player_on_test( wnd_t *wnd );

/* Handle 'close' message for logger view */
wnd_msg_retcode_t player_logview_on_close( wnd_t *wnd );

/* Handle 'changed' message for plugins manager list boxes */
wnd_msg_retcode_t player_pmng_dialog_on_list_change( wnd_t *wnd, int index );

/* Handle 'clicked' message for plugins manager configure buttons */
wnd_msg_retcode_t player_pmng_dialog_on_configure( wnd_t *wnd );

/* Handle 'clicked' message for plugins manager start/stop general 
 * plugin button */
wnd_msg_retcode_t player_pmng_dialog_on_start_stop_general( wnd_t *wnd );

/* Handle 'clicked' message for plugins manager reload button */
wnd_msg_retcode_t player_pmng_dialog_on_reload( wnd_t *wnd );

/* Destructor for plugins manager */
void player_pmng_dialog_destructor( wnd_t *wnd );

/***
 * Player window class functions
 ***/

/* Initialize class */
wnd_class_t *player_wnd_class_init( wnd_global_data_t *global );

/* Get message information */
wnd_msg_handler_t **player_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback );

/* Free message handlers */
void player_free_handlers( wnd_t *wnd );

/* Set player class default styles */
void player_class_set_default_styles( cfg_node_t *list );

/***
 * Miscellaneous functions
 ***/

/* Set mark */
void player_set_mark( char m );

/* Go to mark */
void player_goto_mark( char m );

/* Go back in play list */
void player_go_back( void );

/* Return to the last time */
void player_time_back( void );

/* Set a new search string */
void player_set_search_string( char *str );

/* Save current song and time */
void player_save_time( void );

/* High-level start play */
void player_start_play( int song, song_time_t start_time );

/* High-level pause/resume */
void player_pause_resume( void );

/* High-level stop play */
void player_stop( void );

/* Queue the selected song */
void player_queue_song( void );

#endif

/* End of 'player.h' file */

