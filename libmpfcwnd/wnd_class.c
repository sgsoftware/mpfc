/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : wnd_class.c
 * PURPOSE     : MPFC Window Library. Window classes handling 
 *               functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 18.10.2004
 * NOTE        : Module prefix 'wnd_class'.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cfg.h"
#include "wnd.h"

/* Create a new window class */
wnd_class_t *wnd_class_new( wnd_global_data_t *global, char *name,
		wnd_class_t *parent, wnd_class_msg_get_info_t get_info_func,
		wnd_class_free_handlers_t free_handlers_func,
		cfg_set_default_values_t set_def_styles )
{
	wnd_class_t *klass, *prev_klass = NULL;
	int i;

	assert(global);

	/* Search classes list for this class */
	for ( klass = global->m_wnd_classes; klass != NULL; klass = klass->m_next )
	{
		if (!strcmp(klass->m_name, name))
			return klass;
		prev_klass = klass;
	}

	/* Allocate memory for class */
	klass = (wnd_class_t *)malloc(sizeof(*klass));
	if (klass == NULL)
		return NULL;

	/* Fill class data */
	klass->m_name = strdup(name);
	klass->m_parent = parent;
	klass->m_get_info = get_info_func;
	klass->m_free_handlers = free_handlers_func;
	klass->m_cfg_list = cfg_new_list(global->m_classes_cfg, name, 
			set_def_styles, CFG_NODE_RUNTIME | CFG_NODE_MEDIUM_LIST, 0);
	klass->m_next = NULL;

	/* Insert class to the classes table */
	if (prev_klass == NULL)
		global->m_wnd_classes = klass;
	else
		prev_klass->m_next = klass;
	return klass;
} /* End of 'wnd_class_new' function */

/* Free window class */
void wnd_class_free( wnd_class_t *klass )
{
	if (klass == NULL)
		return;
	if (klass->m_name != NULL)
		free(klass->m_name);
	free(klass);
} /* End of 'wnd_class_free' function */

/* Call 'get_msg_info' function */
wnd_msg_handler_t **wnd_class_get_msg_info( wnd_t *wnd, char *msg_name,
		wnd_class_msg_callback_t *callback )
{
	wnd_class_t *klass;
	for ( klass = wnd->m_class; klass != NULL; klass = klass->m_parent )
	{
		if (klass->m_get_info == NULL)
			continue;

		wnd_msg_handler_t **h = klass->m_get_info(wnd, msg_name,
				callback);
		if (h != NULL)
			return h;
	}
	return NULL;
} /* End of 'wnd_class_get_msg_info' function */

/* Call 'free_handlers' function */
void wnd_class_free_handlers( wnd_t *wnd )
{
	wnd_class_t *klass;
	for ( klass = wnd->m_class; klass != NULL; klass = klass->m_parent )
	{
		if (klass->m_free_handlers != NULL)
			klass->m_free_handlers(wnd);
	}
} /* End of 'wnd_class_free_handlers' function */

/* End of 'wnd_class.c' file */

