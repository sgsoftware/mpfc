/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : error.c
 * PURPOSE     : SG Konsamp. Errors management functions 
 *               implementation.
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

#include <string.h>
#include "types.h"
#include "error.h"

/* Last error code */
dword error_last = ERROR_NO_ERROR;

/* Last error description */
char error_text[80] = "No error";

/* Get last error value */
dword error_get_last( void )
{
	return error_last;
} /* End of 'error_get_last' function */

/* Set error code */
void error_set_code( dword code )
{
	error_last = code;
	error_update_text();
} /* End of 'error_set_code' function */

/* Get error textual description */
char *error_get_text( dword code )
{
	return error_text;
} /* End of 'error_get_text' function */

/* Update error text */
void error_update_text( void )
{
	switch (error_last)
	{
	case ERROR_NO_ERROR:
		strcpy(error_text, _("No error"));
		break;
	case ERROR_NO_SUCH_FILE:
		strcpy(error_text, _("No such file or directory"));
		break;
	case ERROR_NO_MEMORY:
		strcpy(error_text, _("No enough memory"));
		break;
	case ERROR_CURSES:
		strcpy(error_text, _("Curses library error"));
		break;
	case ERROR_UNKNOWN_FILE_TYPE:
		strcpy(error_text, _("Unknown file type"));
		break;
	case ERROR_INVALID_CMD_LINE_PARAMS:
		strcpy(error_text, _("Invalid command line parameters"));
		break;
	case ERROR_IN_PLUGIN_ERROR:
		strcpy(error_text, _("Input plugin error"));
		break;
	case ERROR_OUT_PLUGIN_ERROR:
		strcpy(error_text, _("Output plugin error"));
		break;
	case ERROR_EFFECT_PLUGIN_ERROR:
		strcpy(error_text, _("Effect plugin error"));
		break;
	}
} /* End of 'error_update_text' function */

/* End of 'error.c' file */

