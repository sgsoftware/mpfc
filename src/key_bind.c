/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : key_bind.c
 * PURPOSE     : SG MPFC. Key bindings management functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.09.2003
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
	kbind_add(KBIND_TIME_FW, 1, '>');
	kbind_add(KBIND_TIME_FW, 2, 'l', 't');
	kbind_add(KBIND_TIME_BW, 1, '<');
	kbind_add(KBIND_TIME_BW, 2, 'h', 't');
	kbind_add(KBIND_TIME_MOVE, 2, 'g', 't');
	kbind_add(KBIND_VOL_FW, 1, '+');
	kbind_add(KBIND_VOL_FW, 2, 'l', 'v');
	kbind_add(KBIND_VOL_BW, 1, '-');
	kbind_add(KBIND_VOL_BW, 2, 'h', 'v');
	kbind_add(KBIND_VOL_MOVE, 2, 'g', 'v');
	kbind_add(KBIND_BAL_FW, 1, ']');
	kbind_add(KBIND_BAL_FW, 2, 'l', 'b');
	kbind_add(KBIND_BAL_BW, 1, '[');
	kbind_add(KBIND_BAL_BW, 2, 'h', 'b');
	kbind_add(KBIND_BAL_MOVE, 2, 'g', 'b');
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
	kbind_add(KBIND_UNDO, 1, 'U');
	kbind_add(KBIND_REDO, 1, 'D');
	kbind_add(KBIND_RELOAD_INFO, 1, 'I');
	kbind_add(KBIND_SET_PLAY_BOUNDS, 2, 'P', 's');
	kbind_add(KBIND_CLEAR_PLAY_BOUNDS, 2, 'P', 'c');
	kbind_add(KBIND_PLAY_BOUNDS, 2, 'P', '\n');
	kbind_add(KBIND_EXEC, 1, '!');
	kbind_add(KBIND_MARKA, 2, 'm', 'a');
	kbind_add(KBIND_MARKB, 2, 'm', 'b');
	kbind_add(KBIND_MARKC, 2, 'm', 'c');
	kbind_add(KBIND_MARKD, 2, 'm', 'd');
	kbind_add(KBIND_MARKE, 2, 'm', 'e');
	kbind_add(KBIND_MARKF, 2, 'm', 'f');
	kbind_add(KBIND_MARKG, 2, 'm', 'g');
	kbind_add(KBIND_MARKH, 2, 'm', 'h');
	kbind_add(KBIND_MARKI, 2, 'm', 'i');
	kbind_add(KBIND_MARKJ, 2, 'm', 'j');
	kbind_add(KBIND_MARKK, 2, 'm', 'k');
	kbind_add(KBIND_MARKL, 2, 'm', 'l');
	kbind_add(KBIND_MARKM, 2, 'm', 'm');
	kbind_add(KBIND_MARKN, 2, 'm', 'n');
	kbind_add(KBIND_MARKO, 2, 'm', 'o');
	kbind_add(KBIND_MARKP, 2, 'm', 'p');
	kbind_add(KBIND_MARKQ, 2, 'm', 'q');
	kbind_add(KBIND_MARKR, 2, 'm', 'r');
	kbind_add(KBIND_MARKS, 2, 'm', 's');
	kbind_add(KBIND_MARKT, 2, 'm', 't');
	kbind_add(KBIND_MARKU, 2, 'm', 'u');
	kbind_add(KBIND_MARKV, 2, 'm', 'v');
	kbind_add(KBIND_MARKW, 2, 'm', 'w');
	kbind_add(KBIND_MARKX, 2, 'm', 'x');
	kbind_add(KBIND_MARKY, 2, 'm', 'y');
	kbind_add(KBIND_MARKZ, 2, 'm', 'z');
	kbind_add(KBIND_GOA, 2, '`', 'a');
	kbind_add(KBIND_GOB, 2, '`', 'b');
	kbind_add(KBIND_GOC, 2, '`', 'c');
	kbind_add(KBIND_GOD, 2, '`', 'd');
	kbind_add(KBIND_GOE, 2, '`', 'e');
	kbind_add(KBIND_GOF, 2, '`', 'f');
	kbind_add(KBIND_GOG, 2, '`', 'g');
	kbind_add(KBIND_GOH, 2, '`', 'h');
	kbind_add(KBIND_GOI, 2, '`', 'i');
	kbind_add(KBIND_GOJ, 2, '`', 'j');
	kbind_add(KBIND_GOK, 2, '`', 'k');
	kbind_add(KBIND_GOL, 2, '`', 'l');
	kbind_add(KBIND_GOM, 2, '`', 'm');
	kbind_add(KBIND_GON, 2, '`', 'n');
	kbind_add(KBIND_GOO, 2, '`', 'o');
	kbind_add(KBIND_GOP, 2, '`', 'p');
	kbind_add(KBIND_GOQ, 2, '`', 'q');
	kbind_add(KBIND_GOR, 2, '`', 'r');
	kbind_add(KBIND_GOS, 2, '`', 's');
	kbind_add(KBIND_GOT, 2, '`', 't');
	kbind_add(KBIND_GOU, 2, '`', 'u');
	kbind_add(KBIND_GOV, 2, '`', 'v');
	kbind_add(KBIND_GOW, 2, '`', 'w');
	kbind_add(KBIND_GOX, 2, '`', 'x');
	kbind_add(KBIND_GOY, 2, '`', 'y');
	kbind_add(KBIND_GOZ, 2, '`', 'z');
	kbind_add(KBIND_GOBACK, 2, '`', '`');

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
			bool_t found = TRUE;
			
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
	else if (!strcmp(name, "kbind_undo"))
		return KBIND_UNDO;
	else if (!strcmp(name, "kbind_redo"))
		return KBIND_REDO;
	else if (!strcmp(name, "kbind_reload_info"))
		return KBIND_RELOAD_INFO;
	else if (!strcmp(name, "kbind_set_play_bounds"))
		return KBIND_SET_PLAY_BOUNDS;
	else if (!strcmp(name, "kbind_clear_play_bounds"))
		return KBIND_CLEAR_PLAY_BOUNDS;
	else if (!strcmp(name, "kbind_play_bounds"))
		return KBIND_PLAY_BOUNDS;
	else if (!strcmp(name, "kbind_exec"))
		return KBIND_EXEC;
	else if (!strcmp(name, "kbind_marka"))
		return KBIND_MARKA;
	else if (!strcmp(name, "kbind_markb"))
		return KBIND_MARKB;
	else if (!strcmp(name, "kbind_markc"))
		return KBIND_MARKC;
	else if (!strcmp(name, "kbind_markd"))
		return KBIND_MARKD;
	else if (!strcmp(name, "kbind_marke"))
		return KBIND_MARKE;
	else if (!strcmp(name, "kbind_markf"))
		return KBIND_MARKF;
	else if (!strcmp(name, "kbind_markg"))
		return KBIND_MARKG;
	else if (!strcmp(name, "kbind_markh"))
		return KBIND_MARKH;
	else if (!strcmp(name, "kbind_marki"))
		return KBIND_MARKI;
	else if (!strcmp(name, "kbind_markj"))
		return KBIND_MARKJ;
	else if (!strcmp(name, "kbind_markk"))
		return KBIND_MARKK;
	else if (!strcmp(name, "kbind_markl"))
		return KBIND_MARKL;
	else if (!strcmp(name, "kbind_markm"))
		return KBIND_MARKM;
	else if (!strcmp(name, "kbind_markn"))
		return KBIND_MARKN;
	else if (!strcmp(name, "kbind_marko"))
		return KBIND_MARKO;
	else if (!strcmp(name, "kbind_markp"))
		return KBIND_MARKP;
	else if (!strcmp(name, "kbind_markq"))
		return KBIND_MARKQ;
	else if (!strcmp(name, "kbind_markr"))
		return KBIND_MARKR;
	else if (!strcmp(name, "kbind_marks"))
		return KBIND_MARKS;
	else if (!strcmp(name, "kbind_markt"))
		return KBIND_MARKT;
	else if (!strcmp(name, "kbind_marku"))
		return KBIND_MARKU;
	else if (!strcmp(name, "kbind_markv"))
		return KBIND_MARKV;
	else if (!strcmp(name, "kbind_markw"))
		return KBIND_MARKW;
	else if (!strcmp(name, "kbind_markx"))
		return KBIND_MARKX;
	else if (!strcmp(name, "kbind_marky"))
		return KBIND_MARKY;
	else if (!strcmp(name, "kbind_markz"))
		return KBIND_MARKZ;
	else if (!strcmp(name, "kbind_goa"))
		return KBIND_GOA;
	else if (!strcmp(name, "kbind_gob"))
		return KBIND_GOB;
	else if (!strcmp(name, "kbind_goc"))
		return KBIND_GOC;
	else if (!strcmp(name, "kbind_god"))
		return KBIND_GOD;
	else if (!strcmp(name, "kbind_goe"))
		return KBIND_GOE;
	else if (!strcmp(name, "kbind_gof"))
		return KBIND_GOF;
	else if (!strcmp(name, "kbind_gog"))
		return KBIND_GOG;
	else if (!strcmp(name, "kbind_goh"))
		return KBIND_GOH;
	else if (!strcmp(name, "kbind_goi"))
		return KBIND_GOI;
	else if (!strcmp(name, "kbind_goj"))
		return KBIND_GOJ;
	else if (!strcmp(name, "kbind_gok"))
		return KBIND_GOK;
	else if (!strcmp(name, "kbind_gol"))
		return KBIND_GOL;
	else if (!strcmp(name, "kbind_gom"))
		return KBIND_GOM;
	else if (!strcmp(name, "kbind_gon"))
		return KBIND_GON;
	else if (!strcmp(name, "kbind_goo"))
		return KBIND_GOO;
	else if (!strcmp(name, "kbind_gop"))
		return KBIND_GOP;
	else if (!strcmp(name, "kbind_goq"))
		return KBIND_GOQ;
	else if (!strcmp(name, "kbind_gor"))
		return KBIND_GOR;
	else if (!strcmp(name, "kbind_gos"))
		return KBIND_GOS;
	else if (!strcmp(name, "kbind_got"))
		return KBIND_GOT;
	else if (!strcmp(name, "kbind_gou"))
		return KBIND_GOU;
	else if (!strcmp(name, "kbind_gov"))
		return KBIND_GOV;
	else if (!strcmp(name, "kbind_gow"))
		return KBIND_GOW;
	else if (!strcmp(name, "kbind_gox"))
		return KBIND_GOX;
	else if (!strcmp(name, "kbind_goy"))
		return KBIND_GOY;
	else if (!strcmp(name, "kbind_goz"))
		return KBIND_GOZ;
	else if (!strcmp(name, "kbind_goback"))
		return KBIND_GOBACK;
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

