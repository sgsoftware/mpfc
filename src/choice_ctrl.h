/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : choice_ctrl.h
 * PURPOSE     : SG MPFC. Interface for choice control functions.
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

#ifndef __SG_MPFC_CHOICE_CTRL_H__
#define __SG_MPFC_CHOICE_CTRL_H__

#include "types.h"
#include "window.h"

/* Check that choice is really a choice macro */
#define CHOICE_VALID(ch) (ch >= ' ')

/* Choice control type */
typedef struct
{
	/* Common window object */
	wnd_t m_wnd;

	/* Choice prompt */
	char m_prompt[256];

	/* Choices */
	char m_choices[256];

	/* Choice made */
	char m_choice;
} choice_ctrl_t;

/* Create a new choice control */
choice_ctrl_t *choice_new( wnd_t *parent, int x, int y, int w, int h,
	   						char *prompt, char *choices );

/* Initialize choice control */
bool choice_init( choice_ctrl_t *ch, wnd_t *parent, int x, int y, int w,
	   				int h, char *prompt, char *choices );

/* Destroy choice control */
void choice_destroy( wnd_t *ch );

/* Choice control display function */
void choice_display( wnd_t *obj, dword data );

/* Choice control key handler */
void choice_handle_key( wnd_t *obj, dword data );

#endif

/* End of 'choice_ctrl.h' file */

