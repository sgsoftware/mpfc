/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : outp.c
 * PURPOSE     : SG Konsamp. Output plugin management functions
 *               implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.03.2003
 * NOTE        : Module prefix 'outp'.
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

#include <dlfcn.h>
#include <stdlib.h>
#include "types.h"
#include "error.h"
#include "inp.h"
#include "outp.h"
#include "song.h"

/* Initialize output plugin */
out_plugin_t *outp_init( char *name )
{
	out_plugin_t *p;
	void (*fl)( outp_func_list_t * );

	/* Try to allocate memory for plugin object */
	p = (out_plugin_t *)malloc(sizeof(out_plugin_t));
	if (p == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Load respective library */
	p->m_lib_handler = dlopen(name, RTLD_LAZY);
	if (p->m_lib_handler == NULL)
	{
		error_set_code(ERROR_OUT_PLUGIN_ERROR);
		free(p);
		return NULL;
	}

	/* Initialize plugin */
	fl = dlsym(p->m_lib_handler, "outp_get_func_list");
	if (fl == NULL)
	{
		error_set_code(ERROR_OUT_PLUGIN_ERROR);
		outp_free(p);
		return NULL;
	}
	fl(&p->m_fl);

	return p;
} /* End of 'outp_init' function */

/* Free output plugin object */
void outp_free( out_plugin_t *p )
{
	if (p != NULL)
	{
		dlclose(p->m_lib_handler);
		free(p);
	}
} /* End of 'outp_free' function */

/* End of 'outp.c' file */

