/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_kbind.c
 * PURPOSE     : MPFC Window Library. Key bindings management 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 29.09.2004
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
	assert(kb);
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
					break;
				}
			}
			if (not_matches)
			{
				/* Test next list item */
				if ((*val) == ';')
				{
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
		value ++;
	}
	/* Special symbol in angle brackets */
	else if ((*value) == '<')
	{
		struct
		{
			char *m_str;
			wnd_key_t m_key;
		} keys[] = { 
			{"Left", KEY_LEFT}, {"Right", KEY_RIGHT},
			{"Up", KEY_UP}, {"Down", KEY_DOWN}, {"Home", KEY_HOME},
			{"End", KEY_END}, {"PageUp", KEY_PPAGE}, {"PageDown", KEY_NPAGE},
			{"Enter", '\n'}, {"Return", '\n'}, {"Escape", KEY_ESCAPE}, 
			{"Backspace", KEY_BACKSPACE}, {"Space", ' '}, {"Tab", '\t'},
			{"Del", KEY_DC}, {"Insert", KEY_IC} };
		int i, count;

		/* One key */
		value ++;
		count = sizeof(keys) / sizeof(keys[0]);
		for ( i = 0; i < count; i ++ )
		{
			if (!strncasecmp(value, keys[i].m_str, strlen(keys[i].m_str)))
				ret = keys[i].m_key;
		}

		/* Control or Alt combination */
		if (i == count)
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
		value ++;
	}
	(*val) = value;
	return ret;
} /* End of 'wnd_kbind_value_next_key' function */

/* End of 'wnd_kbind.c' file */

