/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : error.h
 * PURPOSE     : SG MPFC. Interface for errors management 
 *               functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 27.07.2003
 * NOTE        : Module prefix 'error'.
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

#ifndef __SG_MPFC_ERROR_H__
#define __SG_MPFC_ERROR_H__

#include "types.h"

/* Error codes */
#define ERROR_NO_ERROR					0
#define ERROR_NO_SUCH_FILE				1
#define ERROR_NO_MEMORY					2
#define ERROR_CURSES					3
#define ERROR_UNKNOWN_FILE_TYPE			4
#define ERROR_INVALID_CMD_LINE_PARAMS	5
#define ERROR_IN_PLUGIN_ERROR			6
#define ERROR_OUT_PLUGIN_ERROR			7
#define ERROR_EFFECT_PLUGIN_ERROR		8

/* Last error description */
extern char error_text[80];

/* Get last error value */
dword error_get_last( void );

/* Set error code */
void error_set_code( dword code );

/* Get error textual description */
char *error_get_text( dword code );

/* Update error text */
void error_update_text( void );

#endif

/* End of 'error.h' file */

