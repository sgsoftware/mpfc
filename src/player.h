/******************************************************************
 * Copyright (C) 2003 - 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.h
 * PURPOSE     : SG MPFC. Interface for main player functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.08.2004
 * NOTE        : None.
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
#include "history.h"
#include "plist.h"
#include "pmng.h"
#include "undo.h"
#include "wnd.h"

/* User messages for root window types */
#define PLAYER_MSG_END_TRACK    0

/* Player statuses */
#define PLAYER_STATUS_PLAYING 	0
#define PLAYER_STATUS_PAUSED	1
#define PLAYER_STATUS_STOPPED	2

/* History lists stuff */
#define PLAYER_HIST_LIST_ADD  		0
#define PLAYER_HIST_LIST_ADD_OBJ 	1
#define PLAYER_HIST_LIST_SAVE		2
#define PLAYER_HIST_LIST_VAR_NAME	3
#define PLAYER_HIST_LIST_VAR_VAL	4
#define PLAYER_HIST_LIST_SEARCH		5
#define PLAYER_HIST_LIST_EXEC		6
#define PLAYER_HIST_FB_PATTERN		7
#define PLAYER_NUM_HIST_LISTS 		8

/* Variables manager dialog items IDs */
#define PLAYER_VAR_MNGR_VARS	0
#define PLAYER_VAR_MNGR_VAL		1
#define PLAYER_VAR_MNGR_NEW		2
#define PLAYER_VAR_MNGR_SAVE	3
#define PLAYER_VAR_MNGR_RESTORE	4

/* Info editor dialog items IDs */
#define PLAYER_INFO_NAME     100
#define PLAYER_INFO_ARTIST   101
#define PLAYER_INFO_ALBUM    102
#define PLAYER_INFO_YEAR     103
#define PLAYER_INFO_TRACK    104
#define PLAYER_INFO_COMMENTS 105
#define PLAYER_INFO_GENRE    106
#define PLAYER_INFO_LABEL    107

/* Sliders parameters */
#define PLAYER_SLIDER_TIME_Y 2
#define PLAYER_SLIDER_TIME_X 0
#define PLAYER_SLIDER_TIME_W (WND_WIDTH(player_wnd) - 24)
#define PLAYER_SLIDER_VOL_Y  2
#define PLAYER_SLIDER_VOL_X  (WND_WIDTH(player_wnd) - 22)
#define PLAYER_SLIDER_VOL_W  20
#define PLAYER_SLIDER_BAL_Y  1
#define PLAYER_SLIDER_BAL_X  (WND_WIDTH(player_wnd) - 22)
#define PLAYER_SLIDER_BAL_W  20

/* Equalizer information */
extern bool_t player_eq_changed;

/* Undo list */
extern undo_list_t *player_ul;

/* Play list */
extern plist_t *player_plist;

/* Do we story undo information now? */
extern bool_t player_store_undo;

/* Edit boxes history lists */
extern hist_list_t *player_hist_lists[PLAYER_NUM_HIST_LISTS];

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

/* Initialize player */
bool_t player_init( int argc, char *argv[] );

/* Unitialize player */
void player_deinit( void );

/* Run player */
bool_t player_run( void );

/* Initialize configuration */
bool_t player_init_cfg( void );

/* Read configuration file */
void player_read_rcfile( cfg_node_t *list, char *name );

/* Read one line from the configuration file */
void player_parse_cfg_line( cfg_node_t *list, char *str );

/* Parse program command line */
bool_t player_parse_cmd_line( int argc, char *argv[] );

/* Handle key function */
wnd_msg_retcode_t player_on_keydown( wnd_t *wnd, wnd_key_t key );

/* Handle left-button click */
wnd_msg_retcode_t player_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle middle-button click */
wnd_msg_retcode_t player_on_mouse_mdown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Handle left-button double click */
wnd_msg_retcode_t player_on_mouse_ldouble( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Display player function */
wnd_msg_retcode_t player_on_display( wnd_t *wnd );

/* Play list window closing handler */
wnd_msg_retcode_t player_on_close( wnd_t *wnd );

/* User message handling function */
void player_handle_user( wnd_t *wnd, dword data );

/* Key handler function for command repeat value edit box */
void player_repval_handle_key( wnd_t *wnd, dword data );

/* Seek song */
void player_seek( int sec, bool_t rel );

/* Set volume */
void player_set_vol( int vol, bool_t rel );

/* Set balance */
void player_set_bal( int bal, bool_t rel );

/* Play song */
void player_play( int song, int start_time );

/* End play song thread */
void player_end_play( bool_t rem_cur_song );

/* Player thread function */
void *player_thread( void *arg );

/* Stop timer thread */
void player_stop_timer( void );

/* Timer thread function */
void *player_timer_func( void *arg );

/* Display song adding dialog box */
void player_add_dialog( void );

/* Process save play list dialog */
void player_save_dialog( void );

/* Process remove song(s) dialog */
void player_rem_dialog( void );

/* Process sort play list dialog */
void player_sort_dialog( void );

/* Process song info dialog */
void player_info_dialog( void );

/* Process search play list dialog */
void player_search_dialog( int criteria );

/* Process equalizer dialog */
void player_eq_dialog( void );

/* Show help screen */
void player_help( void );

/* Display message handler for help screen */
void player_help_display( wnd_t *wnd, dword data );

/* Handle key message handler for help screen */
void player_help_handle_key( wnd_t *wnd, dword data );

/* Go to next next track */
void player_next_track( void );

/* Start track */
void player_set_track( int track );

/* Handle non-digit key (place it to buffer) */
void player_handle_non_digit( int key );

/* Execute key action */
void player_exec_key_action( void );

/* Display slider */
void player_display_slider( wnd_t *wnd, int x, int y, int width, 
	   int pos, int range );

/* Skip some songs */
int player_skip_songs( int num, bool_t play );

/* Launch variables mini-manager */
void player_var_mini_mngr( void );

/* Variables manager dialog notify handler */
void player_var_mngr_notify( wnd_t *wnd, dword data );

/* Launch variables manager */
void player_var_manager( void );

/* Launch add object dialog */
void player_add_obj_dialog( void );

/* Handle action */
void player_handle_action( int action );

/* Save variables to main configuration file */
void player_save_cfg_vars( cfg_node_t *list, char *vars );

/* Save configuration list */
void player_save_cfg_list( cfg_node_t *list, char *fname );

/* Update volume */
void player_update_vol( void );

/* Execute a command */
void player_exec( void );

/* Set mark */
void player_set_mark( char m );

/* Go to mark */
void player_goto_mark( char m );

/* Go back in play list */
void player_go_back( void );

/* Advanced search dialog */
void player_advanced_search_dialog( void );

/* Handle 'title-format' variable setting */
bool_t player_handle_var_title_format( cfg_node_t *var, char *value );

/* Handle 'output-plugin' variable setting */
bool_t player_handle_var_outp( cfg_node_t *var, char *value );

/* Handle 'color-scheme' variable setting */
bool_t player_handle_color_scheme( cfg_node_t *var, char *value );

/* Handle 'kbind-scheme' variable setting */
bool_t player_handle_kbind_scheme( cfg_node_t *var, char *value );

/* Return to the last time */
void player_time_back( void );

/* Message printer */
void player_print_msg( char *format, ... );

/* Info reload dialog */
void player_info_reload_dialog( void );

/* Notify function for info editor */
void player_info_notify( wnd_t *wnd, dword data );

/* Update currently opened info editor dialog */
void player_update_info_dlg( wnd_t *wnd );

/* Save currently opened info editor dialog */
void player_save_info_dlg( wnd_t *wnd );

/* Launch file browser */
void player_file_browser( void );

/* Set a new search string */
void player_set_search_string( char *str );

/* Signal handler */
void player_handle_signal( int signum );

#endif

/* End of 'player.h' file */

