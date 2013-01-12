/******************************************************************
 * Copyright (C) 2013 by SG Software.
 *
 * SG MPFC. Convenience functions for JSON handling.
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

#ifndef __SG_MPFC_JSON_HELPERS_H__
#define __SG_MPFC_JSON_HELPERS_H__

#include <json-glib/json-glib.h>

/* Object member accessors */
const char *js_get_string( JsonObject *obj, char *key, char *def );
int64_t js_get_int( JsonObject *obj, char *key, int64_t def );
double js_get_double( JsonObject *obj, char *key, double def );
JsonObject *js_get_obj( JsonObject *obj, char *key );
JsonArray *js_get_array( JsonObject *obj, char *key );

JsonNode *js_make_node( JsonObject *obj );
JsonNode *js_make_array_node( JsonArray *array );

char *js_to_string( JsonNode *node, size_t *len );

#endif

/* End of 'json_helpers.h' file */

