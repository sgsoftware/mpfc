/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : main.c
 * PURPOSE     : SG MPFC. Main program module.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 24.03.2004
 * NOTE        : None.
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include "error.h"
#include "player.h"

/* Main function */
int main( int argc, char *argv[] )
{
	/* Initialize random numbers generator */
	srand(time(NULL));
	
	/* Initialize gettext */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	
	/* Initialize player */
	if (!player_init(argc, argv))
	{
		fprintf(stderr, _("Initialization error : %s\n"), 
				error_get_text(error_get_last()));
		return 0;
	}

	/* Run player */
	player_run();

	/* Unitialize player and exit */
	player_deinit();
	return 0;
} /* End of 'main' function */

/* End of 'main.c' file */

