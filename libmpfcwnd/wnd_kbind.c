/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_kbind.c
 * PURPOSE     : MPFC Window Library. Key bindings management 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 4.10.2004
 * NOTE        : Module prefix 'wnd_kbind'.
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

#include <ctype.h>
#include <string.h>
#include "types.h"
#include "wnd.h"
#include "wnd_kbind.h"

/* Keys symbolic names */
static struct
{
	char *m_str;
	wnd_key_t m_key;
} wnd_kbind_names[] = { 
		{ "space", ' ' },
		{ "enter", '\n' },
		{ "return", '\n' },
		{ "tab", '\t' },
		{ "escape", KEY_ESCAPE },
		{ "down", KEY_DOWN },
		{ "up", KEY_UP },
		{ "left", KEY_LEFT },
		{ "right", KEY_RIGHT },
		{ "home", KEY_HOME },
		{ "backspace", KEY_BACKSPACE },
		{ "f0", KEY_F0 },
		{ "dl", KEY_DL },
		{ "il", KEY_IL },
		{ "dc", KEY_DC },
		{ "del", KEY_DC },
		{ "delete", KEY_DC },
		{ "ic", KEY_IC },
		{ "insert", KEY_IC },
		{ "ins", KEY_IC },
		{ "eic", KEY_EIC },
		{ "clear", KEY_CLEAR },
		{ "eos", KEY_EOS },
		{ "eol", KEY_EOL },
		{ "sf", KEY_SF },
		{ "sr", KEY_SR },
		{ "npage", KEY_NPAGE },
		{ "pagedown", KEY_NPAGE },
		{ "ppage", KEY_PPAGE },
		{ "pageup", KEY_PPAGE },
		{ "stab", KEY_STAB },
		{ "ctab", KEY_CTAB },
		{ "catab", KEY_CATAB },
		{ "print", KEY_PRINT },
		{ "ll", KEY_LL },
		{ "a1", KEY_A1 },
		{ "a3", KEY_A3 },
		{ "b2", KEY_B2 },
		{ "c1", KEY_C1 },
		{ "c3", KEY_C3 },
		{ "btab", KEY_BTAB },
		{ "beg", KEY_BEG },
		{ "cancel", KEY_CANCEL },
		{ "close", KEY_CLOSE },
		{ "command", KEY_COMMAND },
		{ "copy", KEY_COPY },
		{ "create", KEY_CREATE },
		{ "end", KEY_END },
		{ "exit", KEY_EXIT },
		{ "find", KEY_FIND },
		{ "help", KEY_HELP },
		{ "mark", KEY_MARK },
		{ "message", KEY_MESSAGE },
		{ "move", KEY_MOVE },
		{ "next", KEY_NEXT },
		{ "open", KEY_OPEN },
		{ "options", KEY_OPTIONS },
		{ "previous", KEY_PREVIOUS },
		{ "redo", KEY_REDO },
		{ "reference", KEY_REFERENCE },
		{ "refresh", KEY_REFRESH },
		{ "replace", KEY_REPLACE },
		{ "restart", KEY_RESTART },
		{ "resume", KEY_RESUME },
		{ "save", KEY_SAVE },
		{ "sbeg", KEY_SBEG },
		{ "scancel", KEY_SCANCEL },
		{ "scommand", KEY_SCOMMAND },
		{ "scopy", KEY_SCOPY },
		{ "screate", KEY_SCREATE },
		{ "sdc", KEY_SDC },
		{ "delete", KEY_SDC },
		{ "sdl", KEY_SDL },
		{ "select", KEY_SELECT },
		{ "send", KEY_SEND },
		{ "seol", KEY_SEOL },
		{ "sexit", KEY_SEXIT },
		{ "sfind", KEY_SFIND },
		{ "shelp", KEY_SHELP },
		{ "shome", KEY_SHOME },
		{ "sic", KEY_SIC },
		{ "sinsert", KEY_SIC },
		{ "sleft", KEY_SLEFT },
		{ "smessage", KEY_SMESSAGE },
		{ "smove", KEY_SMOVE },
		{ "snext", KEY_SNEXT },
		{ "soptions", KEY_SOPTIONS },
		{ "sprevious", KEY_SPREVIOUS },
		{ "sprint", KEY_SPRINT },
		{ "sredo", KEY_SREDO },
		{ "sreplace", KEY_SREPLACE },
		{ "sright", KEY_SRIGHT },
		{ "srsume", KEY_SRSUME },
		{ "ssave", KEY_SSAVE },
		{ "ssuspend", KEY_SSUSPEND },
		{ "sundo", KEY_SUNDO },
		{ "suspend", KEY_SUSPEND },
		{ "undo", KEY_UNDO } };
static int wnd_kbind_num_names = sizeof(wnd_kbind_names) /
	sizeof(*wnd_kbind_names);

/* Initialize kbind module */
wnd_kbind_data_t *wnd_kbind_init( wnd_global_data_t *global )
{
	wnd_kbind_data_t *kb;

	/* Allocate memory */
	kb = (wnd_kbind_data_t *)malloc(sizeof(*kb));
	if (kb == NULL)
		return NULL;
	memset(kb, 0, sizeof(*kb));
	return kb;
} /* End of 'wnd_kbind_init' function */

/* Free kbind module */
void wnd_kbind_free( wnd_kbind_data_t *kb )
{
	if (kb == NULL)
		return;
	free(kb);
} /* End of 'wnd_kbind_free' function */

/* Move key to kbind buffer and send action message if need */
void wnd_kbind_key2buf( wnd_t *wnd, wnd_key_t key )
{
	int res;
	wnd_kbind_data_t *kb;
	char *action;
	assert(wnd);
	assert(kb = WND_KBIND_DATA(wnd));
	
	if (kb->m_buf_ptr == WND_KBIND_BUF_SIZE)
		kb->m_buf_ptr = 0;

	kb->m_buf[kb->m_buf_ptr ++] = key;
	switch (res = wnd_kbind_check_buf(kb, wnd, &action))
	{
	case WND_KBIND_NOT_EXISTING:
		if (kb->m_buf_ptr > 1)
		{
			kb->m_buf_ptr = 0;
			wnd_kbind_key2buf(wnd, key);
		}
		else
			kb->m_buf_ptr = 0;
		break;
	case WND_KBIND_START:
		break;
	default:
		wnd_msg_send(wnd, "action", wnd_msg_action_new(action));
		break;
	}
} /* End of 'wnd_kbind_key2buf' function */

/* Check if buffer contains complete sequence */
int wnd_kbind_check_buf( wnd_kbind_data_t *kb, wnd_t *wnd, char **action )
{
	cfg_node_t *node;
	wnd_class_t *klass;
	int res;

	assert(kb);
	assert(wnd);

	/* Search for this kbind in all the related lists */
	node = wnd->m_cfg_list;
	res = wnd_kbind_check_buf_in_node(kb, wnd, node, action);
	if (res != WND_KBIND_NOT_EXISTING)
		return res;
	for ( klass = wnd->m_class; klass != NULL; klass = klass->m_parent )
	{
		node = klass->m_cfg_list;
		res = wnd_kbind_check_buf_in_node(kb, wnd, node, action);
		if (res != WND_KBIND_NOT_EXISTING)
			return res;
	}
	node = WND_ROOT_CFG(wnd); 
	return wnd_kbind_check_buf_in_node(kb, wnd, node, action);
} /* End of 'wnd_kbind_check_buf' function */

/* Check buffer for sequence in a specified configuration list */
int wnd_kbind_check_buf_in_node( wnd_kbind_data_t *kb, wnd_t *wnd, 
		cfg_node_t *node, char **action )
{
	cfg_node_t *list;
	cfg_list_iterator_t iter;

	/* Search kbinds list for a matching item */
	list = cfg_search_node(node, "kbind");
	if (list == NULL)
		return WND_KBIND_NOT_EXISTING;
	iter = cfg_list_begin_iteration(list);
	for ( ;; )
	{
		bool_t not_matches;
		char *val;
		int i;

		/* Get variable */
		cfg_node_t *var = cfg_list_iterate(&iter);
		if (var == NULL)
			break;
		if (!(CFG_NODE_IS_VAR(var)))
			continue;
		val = CFG_VAR(var)->m_value;

		/* Compare with value in the buffer */
		for ( ;; )
		{
			not_matches = FALSE;
			for ( i = 0; i < kb->m_buf_ptr; i ++ )
			{
				wnd_key_t key = wnd_kbind_value_next_key(&val);
				if (key != kb->m_buf[i] || key == 0)
				{
					not_matches = TRUE;
					if ((*val) == ';')
						break;
					while (val[0] != 0 && !(val[0] != '\\' && val[1] == ';'))
						val ++;
					if (val[0] != 0)
						val ++;
					break;
				}
			}
			if (not_matches)
			{
				/* Test next list item */
				if ((*val) == ';')
				{
					while ((*val) == ';')
						val ++;
					continue;
				}
				else
					break;
			}

			/* Matches */
			(*action) = var->m_name;
			return (((*val) == 0 || (*val) == ';') ? WND_KBIND_FOUND : 
					WND_KBIND_START);
		}
	}
	return WND_KBIND_NOT_EXISTING;
} /* End of 'wnd_kbind_check_buf_in_node' function */

/* Get next key from kbind string value */
wnd_key_t wnd_kbind_value_next_key( char **val )
{
	wnd_key_t ret = 0;
	char *value = *val;

	/* Backslashed symbol */
	if ((*value) == '\\')
	{
		value ++;
		ret = (*value);
		if (ret != 0)
			value ++;
	}
	/* Special symbol in angle brackets */
	else if ((*value) == '<')
	{
		int i;

		/* One key */
		value ++;
		for ( i = 0; i < wnd_kbind_num_names; i ++ )
		{
			if (!strncasecmp(value, wnd_kbind_names[i].m_str, 
						strlen(wnd_kbind_names[i].m_str)))
				ret = wnd_kbind_names[i].m_key;
		}

		/* Control or Alt combination or function key */
		if (i == wnd_kbind_num_names)
		{
			if (!strncasecmp(value, "Ctrl-", 5))
			{
				value = strchr(value, '-');
				value ++;
				ret = KEY_CTRL_AT + toupper(*value) - '@';
			}
			else if (!strncasecmp(value, "Alt-", 4))
			{
				value = strchr(value, '-');
				value ++;
				ret = WND_KEY_WITH_ALT(*value);
			}
			else if ((value[0] == 'f' || value[0] == 'F') &&
					isdigit(value[1]))
			{
				int i = 1;
				int index = 0;
				while (isdigit(value[i]))
				{
					index *= 10;
					index += (value[i] - '0');
					i ++;
				}
				ret = KEY_F(index);
			}
		}
		value = strchr(value, '>');
		if (value != NULL)
			value ++;
	}
	/* Common symbol */
	else
	{
		ret = (*value);
		if (ret == ';')
			ret = 0;
		if (ret != 0)
			value ++;
	}
	(*val) = value;
	return ret;
} /* End of 'wnd_kbind_value_next_key' function */

/* End of 'wnd_kbind.c' file */

