/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Main program module.
 * $Id$
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
#include <gst/gst.h>
#include "error.h"
#include "player.h"
#include "wnd.h"
#include "util.h"

/* Main function */
int main( int argc, char *argv[] )
{
	/* Initialize random numbers generator */
	srand(time(NULL));
	
	/* Initialize gettext */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gst_init(&argc, &argv);
	
	/* Initialize player */
	if (!player_init(argc, argv))
	{
		wnd_deinit(wnd_root);
		player_deinit();
		fprintf(stderr, _("A fatal error occured during player initialization."
					" See log for details\n"));
		return 1;
	}

	/* Check that we have an utf8 locale */
	if (!util_check_utf8_mode())
	{
		logger_fatal(player_log, 0, _("Your locale is not UTF-8! Text handling will work incorrectly"));
	}

	/* Run player */
	player_run();

	/* Unitialize player and exit */
	player_deinit();
	return 0;
} /* End of 'main' function */

/* End of 'main.c' file */

