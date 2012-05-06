/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Basic types declaration.
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

#ifndef __SG_MPFC_TYPES_H__
#define __SG_MPFC_TYPES_H__

#include <assert.h>
#include <libintl.h>
#include <stdint.h>
#include "mpfc-config.h"

/* Some useful types */
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;

/* Define boolean type and values for it */
typedef byte bool_t;
#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

/* Stuff needed for GNU gettext */
#define _(str) gettext (str)
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)

/* The maximal file name length */
#define MAX_FILE_NAME 256

#endif

/* End of 'types.h' file */

