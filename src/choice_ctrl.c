/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : choice_ctrl.c
 * PURPOSE     : SG MPFC. Choice control functions 
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 23.04.2003
 * NOTE        : Module prefix 'choice'.
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

#include <stdlib.h>
#include "types.h"
#include "choice_ctrl.h"
#include "error.h"
#include "window.h"

/* Create a new choice control */
choice_ctrl_t *choice_new( wnd_t *parent, int x, int y, int w, int h,
	   						char *prompt, char *choices )
{
	choice_ctrl_t *ch;

	/* Allocate memory */
	ch = (choice_ctrl_t *)malloc(sizeof(choice_ctrl_t));
	if (ch == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize choice control fields */
	if (!choice_init(ch, parent, x, y, w, h, prompt, choices))
	{
		free(ch);
		return NULL;
	}

	return ch;
} /* End of 'choice_new' function */

/* Initialize choice control */
bool_t choice_init( choice_ctrl_t *ch, wnd_t *parent, int x, int y, int w,
	   				int h, char *prompt, char *choices )
{
	/* Create window object */
	if (!wnd_init(&ch->m_wnd, parent, x, y, w, h))
	{
		return FALSE;
	}

	/* Register handlers */
	wnd_register_handler(ch, WND_MSG_DISPLAY, choice_display);
	wnd_register_handler(ch, WND_MSG_KEYDOWN, choice_handle_key);

	/* Set choice control specific fields */
	strcpy(ch->m_prompt, prompt);
	strcpy(ch->m_choices, choices);
	ch->m_choice = 0;
	WND_OBJ(ch)->m_flags |= (WND_INITIALIZED);
	return TRUE;
} /* End of 'choice_init' function */

/* Destroy choice control */
void choice_destroy( wnd_t *wnd )
{
	wnd_destroy_func(wnd);
} /* End of 'choice_destroy' function */

/* Choice control display function */
void choice_display( wnd_t *wnd, dword data )
{
	choice_ctrl_t *ch = (choice_ctrl_t *)wnd;

	/* Print prompt */
	wnd_move(wnd, 0, 0);
	wnd_printf(wnd, "%s\n", ch->m_prompt);
} /* End of 'choice_display' function */

/* Choice control key handler */
void choice_handle_key( wnd_t *wnd, dword data )
{
	choice_ctrl_t *ch = (choice_ctrl_t *)wnd;
	int key = (int)data;
	
	/* Escape - exit */
	if (key == 27)
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
	/* If we have key from choices list - save it and exit */
	else
	{
		int i, n = strlen(ch->m_choices);
		
		for ( i = 0; i < n; i ++ )
			if (key == ch->m_choices[i])
			{
				ch->m_choice = key;
				wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
			}
	}
} /* End of 'choice_handle_key' function */

/* End of 'choice_ctrl.c' file */

