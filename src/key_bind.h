/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : key_bind.h
 * PURPOSE     : SG MPFC. Interface for key bindings management
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.09.2003
 * NOTE        : Module prefix 'kbind'.
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

#ifndef __SG_MPFC_KEY_BIND_H__
#define __SG_MPFC_KEY_BIND_H__

#include "types.h"

/* Key actions */
#define KBIND_QUIT 				0
#define KBIND_MOVE_DOWN			1
#define KBIND_MOVE_UP			2
#define KBIND_SCREEN_DOWN		3
#define KBIND_SCREEN_UP			4
#define KBIND_MOVE				5
#define KBIND_START_PLAY		6
#define KBIND_PLAY				7
#define KBIND_PAUSE				8
#define KBIND_STOP				9
#define KBIND_NEXT				10
#define KBIND_PREV				11
#define KBIND_TIME_FW			12
#define KBIND_TIME_BW			13
#define KBIND_TIME_MOVE			14
#define KBIND_VOL_FW			15
#define KBIND_VOL_BW			16
#define KBIND_VOL_MOVE			17
#define KBIND_BAL_FW			18
#define KBIND_BAL_BW			19
#define KBIND_BAL_MOVE			20
#define KBIND_INFO				21
#define KBIND_ADD				22
#define KBIND_REM				23
#define KBIND_SAVE				24
#define KBIND_SORT				25
#define KBIND_VISUAL			26
#define KBIND_CENTRIZE			27
#define KBIND_SEARCH			28
#define KBIND_NEXT_MATCH		29
#define KBIND_PREV_MATCH		30
#define KBIND_HELP				31
#define KBIND_ADD_OBJ			32
#define KBIND_EQUALIZER			33
#define KBIND_SHUFFLE			34
#define KBIND_LOOP				35
#define KBIND_VAR_MANAGER		36
#define KBIND_DIG_0				37
#define KBIND_DIG_1				38
#define KBIND_DIG_2				39
#define KBIND_DIG_3				40
#define KBIND_DIG_4				41
#define KBIND_DIG_5				42
#define KBIND_DIG_6				43
#define KBIND_DIG_7				44
#define KBIND_DIG_8				45
#define KBIND_DIG_9				46
#define KBIND_PLIST_DOWN		47
#define KBIND_PLIST_UP			48
#define KBIND_PLIST_MOVE		49
#define KBIND_VAR_MINI_MANAGER	50
#define KBIND_UNDO				51
#define KBIND_REDO				52
#define KBIND_RELOAD_INFO		53
#define KBIND_SET_PLAY_BOUNDS	54
#define KBIND_CLEAR_PLAY_BOUNDS	55
#define KBIND_PLAY_BOUNDS		56
#define KBIND_NUM_ACTIONS		57

/* The maximal key sequence length */
#define KBIND_MAX_SEQ_LEN 10

/* Results for kbind_check_buf function */
#define KBIND_NOT_EXISTING -1
#define KBIND_START -2

/* Keys sequence type */
typedef struct tag_kbind_seq_t
{
	int m_seq[KBIND_MAX_SEQ_LEN];
	int m_len;
} kbind_seq_t;

/* Binding for single key */
typedef struct tag_kbind_single_t
{
	kbind_seq_t *m_seqs;
	int m_num_seqs;
} kbind_single_t;

/* Initialize bindings */
void kbind_init( void );

/* Uninitialize bindings */
void kbind_free( void );

/* Add binding */
void kbind_add( int action, int len, ... );

/* Store key in buffer */
void kbind_key2buf( int key );

/* Check if buffer contains complete sequence */
int kbind_check_buf( void );

/* Read key bindings from configuration */
void kbind_read_from_cfg( void );

/* Convert variable name to action ID */
int kbind_var2act( char *name );

/* Set bindings using variable value */
void kbind_set_var( int action, char *val );

/* Parse configuration variable value with single key binding */
void kbind_parse_cfg_str( int action, char *str );

/* Clear key bindings for specified action */
void kbind_clear_bindings( int action );

/* Add sequence */
void kbind_add_seq( int action, int len, int *seq );
	
#endif

/* End of 'key_bind.h' file */

