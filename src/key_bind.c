/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : key_bind.c
 * PURPOSE     : SG MPFC. Key bindings management functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 8.08.2003
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

#include <curses.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "cfg.h"
#include "key_bind.h"

/* Bindings array */
kbind_single_t kbind_bindings[KBIND_NUM_ACTIONS];

/* Key sequence buffer */
int kbind_buf[KBIND_MAX_SEQ_LEN];
int kbind_buf_len;

/* Initialize bindings */
void kbind_init( void )
{
	int i;
	
	/* Initialize key buffer */
	kbind_buf_len = 0;

	/* Initialize bindings */
	for ( i = 0; i < KBIND_NUM_ACTIONS; i ++ )
	{
		kbind_bindings[i].m_seqs = NULL;
		kbind_bindings[i].m_num_seqs = 0;
	}
	kbind_add(KBIND_QUIT, 1, 'q');
	kbind_add(KBIND_QUIT, 1, 'Q');
	kbind_add(KBIND_MOVE_DOWN, 1, 'j');
	kbind_add(KBIND_MOVE_DOWN, 1, KEY_DOWN);
	kbind_add(KBIND_MOVE_UP, 1, 'k');
	kbind_add(KBIND_MOVE_UP, 1, KEY_UP);
	kbind_add(KBIND_SCREEN_UP, 1, 'u');
	kbind_add(KBIND_SCREEN_UP, 1, KEY_PPAGE);
	kbind_add(KBIND_SCREEN_DOWN, 1, 'd');
	kbind_add(KBIND_SCREEN_DOWN, 1, KEY_NPAGE);
	kbind_add(KBIND_MOVE, 1, 'G');
	kbind_add(KBIND_START_PLAY, 1, '\n');
	kbind_add(KBIND_PLAY, 1, 'x');
	kbind_add(KBIND_PAUSE, 1, 'c');
	kbind_add(KBIND_STOP, 1, 'v');
	kbind_add(KBIND_NEXT, 1, 'b');
	kbind_add(KBIND_PREV, 1, 'z');
	kbind_add(KBIND_TIME_FW, 2, 'l', 't');
	kbind_add(KBIND_TIME_BW, 2, 'h', 't');
	kbind_add(KBIND_TIME_MOVE, 2, 'g', 't');
	kbind_add(KBIND_VOL_FW, 1, '+');
	kbind_add(KBIND_VOL_FW, 2, 'l', 'v');
	kbind_add(KBIND_VOL_BW, 1, '-');
	kbind_add(KBIND_VOL_BW, 2, 'h', 'v');
	kbind_add(KBIND_VOL_MOVE, 2, 'g', 'v');
	kbind_add(KBIND_INFO, 1, 'i');
	kbind_add(KBIND_ADD, 1, 'a');
	kbind_add(KBIND_REM, 1, 'r');
	kbind_add(KBIND_SAVE, 1, 's');
	kbind_add(KBIND_SORT, 1, 'S');
	kbind_add(KBIND_VISUAL, 1, 'V');
	kbind_add(KBIND_CENTRIZE, 1, 'C');
	kbind_add(KBIND_SEARCH, 1, '/');
	kbind_add(KBIND_NEXT_MATCH, 1, 'n');
	kbind_add(KBIND_PREV_MATCH, 1, 'N');
	kbind_add(KBIND_HELP, 1, '?');
	kbind_add(KBIND_ADD_OBJ, 1, 'A');
	kbind_add(KBIND_EQUALIZER, 1, 'e');
	kbind_add(KBIND_SHUFFLE, 1, 'R');
	kbind_add(KBIND_LOOP, 1, 'L');
	kbind_add(KBIND_VAR_MANAGER, 1, 'O');
	kbind_add(KBIND_VAR_MINI_MANAGER, 1, 'o');
	kbind_add(KBIND_DIG_0, 1, '0');
	kbind_add(KBIND_DIG_1, 1, '1');
	kbind_add(KBIND_DIG_2, 1, '2');
	kbind_add(KBIND_DIG_3, 1, '3');
	kbind_add(KBIND_DIG_4, 1, '4');
	kbind_add(KBIND_DIG_5, 1, '5');
	kbind_add(KBIND_DIG_6, 1, '6');
	kbind_add(KBIND_DIG_7, 1, '7');
	kbind_add(KBIND_DIG_8, 1, '8');
	kbind_add(KBIND_DIG_9, 1, '9');
	kbind_add(KBIND_PLIST_DOWN, 1, 'J');
	kbind_add(KBIND_PLIST_UP, 1, 'K');
	kbind_add(KBIND_PLIST_MOVE, 1, 'M');

	/* Read bindings from configuration */
	kbind_read_from_cfg();
} /* End of 'kbind_init' function */

/* Uninitialize bindings */
void kbind_free( void )
{
	int i; 

	for ( i = 0; i < KBIND_NUM_ACTIONS; i ++ )
		if (kbind_bindings[i].m_seqs != NULL)
			free(kbind_bindings[i].m_seqs);
} /* End of 'kbind_free' function */

/* Add binding */
void kbind_add( int action, int len, ... )
{
	va_list ap;
	int seq[KBIND_MAX_SEQ_LEN];
	int i;

	if (len > KBIND_MAX_SEQ_LEN)
		len = KBIND_MAX_SEQ_LEN;

	/* Initialize binding */
	va_start(ap, len);
	for ( i = 0; i < len; i ++ )
		seq[i] = va_arg(ap, int);
	va_end(ap);
	kbind_add_seq(action, len, seq);
} /* End of 'kbind_add' function */

/* Store key in buffer */
void kbind_key2buf( int key )
{
	int res;
	
	if (kbind_buf_len == KBIND_MAX_SEQ_LEN)
		kbind_buf_len = 0;

	kbind_buf[kbind_buf_len ++] = key;
	switch (res = kbind_check_buf())
	{
	case KBIND_NOT_EXISTING:
		if (kbind_buf_len > 1)
		{
			kbind_buf_len = 0;
			kbind_key2buf(key);
		}
		else
			kbind_buf_len = 0;
		break;
	case KBIND_START:
		break;
	default:
		player_handle_action(res);
		break;
	}
} /* End of 'kbind_key2buf' function */

/* Check if buffer contains complete sequence */
int kbind_check_buf( void )
{
	int i, j, k;

	for ( i = 0; i < KBIND_NUM_ACTIONS; i ++ )
	{
		for ( j = 0; j < kbind_bindings[i].m_num_seqs; j ++ )
		{
			bool found = TRUE;
			
			for ( k = 0; k < kbind_buf_len; k ++ )
				if (kbind_buf[k] != kbind_bindings[i].m_seqs[j].m_seq[k])
				{
					found = FALSE;
					break;
				}
			if (found)
				return (kbind_buf_len == kbind_bindings[i].m_seqs[j].m_len) ?
					i : KBIND_START;
		}
	}
	return KBIND_NOT_EXISTING;
} /* End of 'kbind_check_buf' function */

/* Read key bindings from configuration */
void kbind_read_from_cfg( void )
{
	int i;

	/* Read all variables starting with 'kbind_' */
	for ( i = 0; i < cfg_list->m_num_vars; i ++ )
	{
		int action = kbind_var2act(cfg_list->m_vars[i].m_name);
		if (action >= 0)
			kbind_set_var(action, cfg_list->m_vars[i].m_val);
	}
} /* End of 'kbind_read_from_cfg' function */

/* Convert variable name to action ID */
int kbind_var2act( char *name )
{
	if (!strcmp(name, "kbind_quit"))
		return KBIND_QUIT;
	else if (!strcmp(name, "kbind_move_down"))
		return KBIND_MOVE_DOWN;
	else if (!strcmp(name, "kbind_move_up"))
		return KBIND_MOVE_UP;
	else if (!strcmp(name, "kbind_screen_down"))
		return KBIND_SCREEN_DOWN;
	else if (!strcmp(name, "kbind_screen_up"))
		return KBIND_SCREEN_UP;
	else if (!strcmp(name, "kbind_move"))
		return KBIND_MOVE;
	else if (!strcmp(name, "kbind_start_play"))
		return KBIND_START_PLAY;
	else if (!strcmp(name, "kbind_play"))
		return KBIND_PLAY;
	else if (!strcmp(name, "kbind_pause"))
		return KBIND_PAUSE;
	else if (!strcmp(name, "kbind_stop"))
		return KBIND_STOP;
	else if (!strcmp(name, "kbind_next"))
		return KBIND_NEXT;
	else if (!strcmp(name, "kbind_prev"))
		return KBIND_PREV;
	else if (!strcmp(name, "kbind_time_fw"))
		return KBIND_TIME_FW;
	else if (!strcmp(name, "kbind_time_bw"))
		return KBIND_TIME_BW;
	else if (!strcmp(name, "kbind_time_move"))
		return KBIND_TIME_MOVE;
	else if (!strcmp(name, "kbind_vol_fw"))
		return KBIND_VOL_FW;
	else if (!strcmp(name, "kbind_vol_bw"))
		return KBIND_VOL_BW;
	else if (!strcmp(name, "kbind_vol_move"))
		return KBIND_VOL_MOVE;
	else if (!strcmp(name, "kbind_bal_fw"))
		return KBIND_BAL_FW;
	else if (!strcmp(name, "kbind_bal_bw"))
		return KBIND_BAL_BW;
	else if (!strcmp(name, "kbind_bal_move"))
		return KBIND_BAL_MOVE;
	else if (!strcmp(name, "kbind_info"))
		return KBIND_INFO;
	else if (!strcmp(name, "kbind_add"))
		return KBIND_ADD;
	else if (!strcmp(name, "kbind_rem"))
		return KBIND_REM;
	else if (!strcmp(name, "kbind_save"))
		return KBIND_SAVE;
	else if (!strcmp(name, "kbind_sort"))
		return KBIND_SORT;
	else if (!strcmp(name, "kbind_visual"))
		return KBIND_VISUAL;
	else if (!strcmp(name, "kbind_centrize"))
		return KBIND_CENTRIZE;
	else if (!strcmp(name, "kbind_search"))
		return KBIND_SEARCH;
	else if (!strcmp(name, "kbind_next_match"))
		return KBIND_NEXT_MATCH;
	else if (!strcmp(name, "kbind_prev_match"))
		return KBIND_PREV_MATCH;
	else if (!strcmp(name, "kbind_help"))
		return KBIND_HELP;
	else if (!strcmp(name, "kbind_add_obj"))
		return KBIND_ADD_OBJ;
	else if (!strcmp(name, "kbind_equalizer"))
		return KBIND_EQUALIZER;
	else if (!strcmp(name, "kbind_shuffle"))
		return KBIND_SHUFFLE;
	else if (!strcmp(name, "kbind_loop"))
		return KBIND_LOOP;
	else if (!strcmp(name, "kbind_var_manager"))
		return KBIND_VAR_MANAGER;
	else if (!strcmp(name, "kbind_var_mini_manager"))
		return KBIND_VAR_MINI_MANAGER;
	else if (!strcmp(name, "kbind_plist_down"))
		return KBIND_PLIST_DOWN;
	else if (!strcmp(name, "kbind_plist_up"))
		return KBIND_PLIST_UP;
	else if (!strcmp(name, "kbind_plist_move"))
		return KBIND_PLIST_MOVE;
	return -1;
} /* End of 'kbind_var2act' function */

/* Set bindings using variable value */
void kbind_set_var( int action, char *val )
{
	int i, j;
	char str[80];
	
	/* Clear bindings for this action */
	kbind_clear_bindings(action);

	/* Extract sequences */
	for ( i = 0, j = 0; val[i]; i ++ )
	{
		if (val[i] == ';' && !(i > 0 && val[i - 1] == '\\'))
		{
			str[j] = 0;
			kbind_parse_cfg_str(action, str);
			j = 0;
		}
		else
			str[j ++] = val[i];
	}
	str[j] = 0;
	kbind_parse_cfg_str(action, str);
} /* End of 'kbind_set_var' function */

/* Parse configuration variable value with single key binding */
void kbind_parse_cfg_str( int action, char *str )
{
	int i, j;
	int seq[KBIND_MAX_SEQ_LEN];
	char name[10];

	if (!strlen(str))
		return;

	for ( i = 0, j = 0; str[i]; i ++ )
	{
		int k;

		/* Special key */
		if (str[i] == '<')
		{
			for ( k = 0, i ++; str[i] && str[i] != '>'; i ++, k ++ )
				name[k] = str[i];
			name[k] = 0;
			if (!strcasecmp(name, "Up"))
				seq[j ++] = KEY_UP;
			else if (!strcasecmp(name, "Down"))
				seq[j ++] = KEY_DOWN;
			else if (!strcasecmp(name, "Right"))
				seq[j ++] = KEY_RIGHT;
			else if (!strcasecmp(name, "Left"))
				seq[j ++] = KEY_LEFT;
			else if (!strcasecmp(name, "PageUp"))
				seq[j ++] = KEY_PPAGE;
			else if (!strcasecmp(name, "PageDn"))
				seq[j ++] = KEY_NPAGE;
			else if (!strcasecmp(name, "Home"))
				seq[j ++] = KEY_HOME;
			else if (!strcasecmp(name, "End"))
				seq[j ++] = KEY_END;
			else if (!strcasecmp(name, "BS"))
				seq[j ++] = KEY_BACKSPACE;
			else if (!strcasecmp(name, "Del"))
				seq[j ++] = KEY_DC;
			else if (!strcasecmp(name, "Ret"))
				seq[j ++] = '\n';
			else if (!strcasecmp(name, "Tab"))
				seq[j ++] = '\t';
		}
		/* Escaped symbol */
		else if (str[i] == '\\' && str[i + 1])
			seq[j ++] = str[++i];
		else
			seq[j ++] = str[i];

		if (j == KBIND_MAX_SEQ_LEN)
			break;
	}
	seq[j] = 0;

	kbind_add_seq(action, j, seq);
} /* End of 'kbind_parse_cfg_str' function */
	
/* Clear key bindings for specified action */
void kbind_clear_bindings( int action )
{
	kbind_single_t *b;
	
	b = &kbind_bindings[action];
	if (b->m_seqs != NULL)
	{
		free(b->m_seqs);
		b->m_seqs = NULL;
	}
	b->m_num_seqs = 0;
} /* End of 'kbind_clear_bindings' function */
	
/* Add sequence */
void kbind_add_seq( int action, int len, int *seq )
{
	kbind_single_t *b;
	int i;

	if (action >= KBIND_NUM_ACTIONS)
		return;

	/* Allocate memory for binding */
	b = &kbind_bindings[action];
	b->m_num_seqs ++;
	if (b->m_seqs == NULL)
		b->m_seqs = (kbind_seq_t *)malloc(sizeof(kbind_seq_t) * b->m_num_seqs);
	else
		b->m_seqs = (kbind_seq_t *)realloc(b->m_seqs,
					  sizeof(kbind_seq_t) * b->m_num_seqs);
	if (len > KBIND_MAX_SEQ_LEN)
		len = KBIND_MAX_SEQ_LEN;

	/* Initialize binding */
	b->m_seqs[b->m_num_seqs - 1].m_len = len;
	memcpy(b->m_seqs[b->m_num_seqs - 1].m_seq, seq, len * sizeof(int));
} /* End of 'kbind_add_seq' function */
	
/* End of 'key_bind.c' file */

