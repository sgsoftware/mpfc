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

#include "json_helpers.h"

const char *js_get_string( JsonObject *obj, char *key, char *def )
{
	JsonNode *node = json_object_get_member(obj, key);
	if (!node)
		return def;
	if (!JSON_NODE_HOLDS_VALUE(node))
		return def;
	return json_node_get_string(node);
}

int64_t js_get_int( JsonObject *obj, char *key, int64_t def )
{
	JsonNode *node = json_object_get_member(obj, key);
	if (!node)
		return def;
	if (!JSON_NODE_HOLDS_VALUE(node))
		return def;
	return json_node_get_int(node);
}

double js_get_double( JsonObject *obj, char *key, double def )
{
	JsonNode *node = json_object_get_member(obj, key);
	if (!node)
		return def;
	if (!JSON_NODE_HOLDS_VALUE(node))
		return def;
	if (json_node_get_value_type(node) == G_TYPE_INT64)
		return json_node_get_int(node);
	else
		return json_node_get_double(node);
}

JsonObject *js_get_obj( JsonObject *obj, char *key )
{
	JsonNode *node = json_object_get_member(obj, key);
	if (!node)
		return NULL;
	if (!JSON_NODE_HOLDS_OBJECT(node))
		return NULL;
	return json_node_get_object(node);
}

JsonArray *js_get_array( JsonObject *obj, char *key )
{
	JsonNode *node = json_object_get_member(obj, key);
	if (!node)
		return NULL;
	if (!JSON_NODE_HOLDS_ARRAY(node))
		return NULL;
	return json_node_get_array(node);
}

JsonNode *js_make_node( JsonObject *obj )
{
	JsonNode *node = json_node_new(JSON_NODE_OBJECT);
	json_node_take_object(node, obj);
	return node;
}

JsonNode *js_make_array_node( JsonArray *array )
{
	JsonNode *node = json_node_new(JSON_NODE_ARRAY);
	json_node_take_array(node, array);
	return node;
}

char *js_to_string( JsonNode *node, size_t *len )
{
	JsonGenerator *gen = json_generator_new();
	json_generator_set_root(gen, node);
	char *res = json_generator_to_data(gen, len);
	g_object_unref(gen);
	return res;
}

/* End of 'json_helpers.c' file */

