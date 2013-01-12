/******************************************************************
 * Copyright (C) 2003 - 2013 by SG Software.
 *
 * SG MPFC. Load and save song info.
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

#ifndef __SG_MPFC_METADATA_IO_H__
#define __SG_MPFC_METADATA_IO_H__

#include "types.h"
#include "main_types.h"
#include "song_info.h"

/* Get song information function */
song_info_t *md_get_info( const char *file_name, const char *full_uri, song_time_t *len );
	
/* Save song information function */
bool_t md_save_info( const char *file_name, song_info_t *info );
	
#endif

/* End of 'metadata_io.h' file */

