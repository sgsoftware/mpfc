/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : key_bind.c
 * PURPOSE     : SG MPFC. Key bindings management functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 2.08.2003
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
  License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#include <curses.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
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
	kbind_add(KBIND_NEXT, 1, 'z');
	kbind_add(KBIND_PREV, 1, 'b');
	kbind_add(KBIND_TIME_FW, 1, 'l');
	kbind_add(KBIND_TIME_BW, 1, 'h');
	kbind_add(KBIND_TIME_MOVE, 1, 'g');
	kbind_add(KBIND_VOL_FW, 1, '+');
	kbind_add(KBIND_VOL_BW, 1, '-');
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
	kbind_add(KBIND_VAR_MANAGER, 1, 'o');
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
	va_start(ap, len);
	for ( i = 0; i < len; i ++ )
		b->m_seqs[b->m_num_seqs - 1].m_seq[i] = va_arg(ap, int);
	va_end(ap);
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

/* End of 'key_bind.c' file */

