/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : song_info.h
 * PURPOSE     : SG MPFC. Song information type declaration.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.08.2003
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

#ifndef __SG_MPFC_SONG_INFO_H__
#define __SG_MPFC_SONG_INFO_H__

#include "types.h"

/* Song information type */
typedef struct
{
	char m_artist[256];
	char m_name[256];
	char m_album[256];
	char m_year[256];
	char m_comments[256];
	char m_track[256];
	byte m_genre;
	union
	{
		byte m_data;
		char m_text[80];
	} m_genre_data;
	bool m_loaded;
	bool m_only_own;
	char m_own_data[1024];
	bool m_not_own_present;
} song_info_t;

#endif

/* End of 'song_info.h' file */

