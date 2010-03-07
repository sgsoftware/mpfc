/******************************************************************
 * Copyright (C) 2006 by SG Software.
 *
 * MPFC Cue plugin. Testing module.
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
#include "cue_sheet.h"

/* Main function */
int main( int argc, char *argv[] )
{
	char *cuename;
	int i;
	cue_sheet_t *cs;

	/* Get cue file name */
	if (argc < 2)
	{
		printf("test <cue-name>\n");
		return 1;
	}
	cuename = argv[1];
	
	/* Read sheet */
	cs = cue_sheet_parse(cuename);
	if (cs == NULL)
	{
		printf("Failed to parse cue %s\n", cuename);
		return 1;
	}

	/* Print cue data */
	printf("Filename: %s\n", cs->m_file_name);
	for ( i = 0; i < cs->m_num_tracks; i ++ )
	{
		cue_track_t *track = cs->m_tracks[i];

		printf("Track %d.\nTitle: %s\nPerformer: %s\nIndex 1: %d\n",
				i, track->m_title, track->m_performer,
				track->m_indices[1]);
	}

	cue_sheet_free(cs);
	return 0;
} /* End of 'main' function */

/* End of 'test.c' file */

