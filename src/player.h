/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : player.h
 * PURPOSE     : SG MPFC. Interface for main player functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 16.08.2003
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

#ifndef __SG_KONSAMP_PLAYER_H__
#define __SG_KONSAMP_PLAYER_H__

#include "types.h"
#include "cfg.h"
#include "plist.h"
#include "undo.h"
#include "window.h"

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
#define PLAYER_NUM_HIST_LISTS 		6

/* Variables manager dialog items IDs */
#define PLAYER_VAR_MNGR_VARS	0
#define PLAYER_VAR_MNGR_VAL		1
#define PLAYER_VAR_MNGR_NEW		2
#define PLAYER_VAR_MNGR_SAVE	3
#define PLAYER_VAR_MNGR_RESTORE	4

/* Equalizer information */
extern bool player_eq_changed;

/* Undo list */
extern undo_list_t *player_ul;

/* Play list */
extern plist_t *player_plist;

/* Do we story undo information now? */
extern bool player_store_undo;

/* Initialize player */
bool player_init( int argc, char *argv[] );

/* Unitialize player */
void player_deinit( void );

/* Run player */
bool player_run( void );

/* Parse program command line */
bool player_parse_cmd_line( int argc, char *argv[] );

/* Handle key function */
void player_handle_key( wnd_t *wnd, dword data );

/* Handle mouse left button click */
void player_handle_mouse_click( wnd_t *wnd, dword data );

/* Display player function */
void player_display( wnd_t *wnd, dword data );

/* User message handling function */
void player_handle_user( wnd_t *wnd, dword data );

/* Key handler function for command repeat value edit box */
void player_repval_handle_key( wnd_t *wnd, dword data );

/* Seek song */
void player_seek( int sec, bool rel );

/* Set volume */
void player_set_vol( int vol, bool rel );

/* Set balance */
void player_set_bal( int bal, bool rel );

/* Play song */
void player_play( int start_time );

/* End play song thread */
void player_end_play( void );

/* Player thread function */
void *player_thread( void *arg );

/* Stop timer thread */
void player_stop_timer( void );

/* Timer thread function */
void *player_timer_func( void *arg );

/* Process add file dialog */
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
void player_search_dialog( void );

/* Process equalizer dialog */
void player_eq_dialog( void );

/* Show help screen */
void player_help( void );

/* Display message handler for help screen */
void player_help_display( wnd_t *wnd, dword data );

/* Handle key message handler for help screen */
void player_help_handle_key( wnd_t *wnd, dword data );

/* Start next track */
void player_next_track( void );

/* Handle non-digit key (place it to buffer) */
void player_handle_non_digit( int key );

/* Execute key action */
void player_exec_key_action( void );

/* Display slider */
void player_display_slider( wnd_t *wnd, int x, int y, int width, 
	   int pos, int range );

/* Skip some songs */
void player_skip_songs( int num );

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
void player_save_cfg_vars( cfg_list_t *list, char *vars );

/* Save configuration list */
void player_save_cfg_list( cfg_list_t *list, char *fname );

/* Update volume */
void player_update_vol( void );

#endif

/* End of 'player.h' file */

