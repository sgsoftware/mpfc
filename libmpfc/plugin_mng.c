/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Plugins manager functions implementation.
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fts.h>
#include <gst/gst.h>
#include "types.h"
#include "cfg.h"
#include "command.h"
#include "csp.h"
#include "ep.h"
#include "inp.h"
#include "genp.h"
#include "outp.h"
#include "plugin.h"
#include "pmng.h"
#include "util.h"

/* Build supported media file extensions list */
static bool_t pmng_fill_media_file_exts( pmng_t *pmng )
{
	/* First collect all mimetypes which might correspond to audio */
	GHashTable *all_mimes = g_hash_table_new(g_str_hash, g_str_equal);
    GList* factories = gst_registry_get_feature_list(gst_registry_get(),
			GST_TYPE_ELEMENT_FACTORY);
    for ( GList* iter = g_list_first(factories); iter; iter = g_list_next(iter) )
	{
        GstPluginFeature *feature = GST_PLUGIN_FEATURE(iter->data);
		GstElementFactory *factory = GST_ELEMENT_FACTORY(feature);
        const gchar *klass = gst_element_factory_get_klass(factory);

		if (!strcmp(klass, "Codec/Decoder") ||
		    !strcmp(klass, "Codec/Decoder/Audio") ||
		    !strcmp(klass, "Codec/Demuxer") ||
		    !strcmp(klass, "Codec/Demuxer/Audio") ||
		    !strcmp(klass, "Codec/Parser") ||
		    !strcmp(klass, "Codec/Parser/Audio"))
		{
            for ( const GList *templs = gst_element_factory_get_static_pad_templates(factory);
					templs; templs = templs->next )
			{
                GstStaticPadTemplate *templ = (GstStaticPadTemplate*)templs->data;
				if (!templ)
					continue;
				if (templ->direction != GST_PAD_SINK)
					continue;

				GstCaps *caps = gst_static_pad_template_get_caps(templ);
				if (!caps)
					continue;

				for ( unsigned i = 0; i < gst_caps_get_size(caps); ++i )
				{
					const gchar *mime = gst_structure_get_name(
							gst_caps_get_structure(caps, i));
					g_hash_table_insert(all_mimes, (gpointer)mime, NULL);
				}
				gst_caps_unref(caps);
			}

        }
    }
    g_list_free(factories);

	/* Collect all supported extensions from type finders corresponding to audio formats */
	str_t *media_exts = str_new("");
	GList *tffs = gst_type_find_factory_get_list();
	for ( GList *p = tffs; p; p = p->next )
	{
		GstTypeFindFactory *tff = (GstTypeFindFactory*)(p->data);
		GstCaps *caps = gst_type_find_factory_get_caps(tff);
		if (!caps)
			continue;

		bool_t has_audio = FALSE;
		for ( unsigned i = 0; i < gst_caps_get_size(caps); ++i )
		{
			const gchar *mime = gst_structure_get_name(
					gst_caps_get_structure(caps, i));
			if (g_hash_table_lookup_extended(all_mimes, mime, NULL, NULL))
			{
				has_audio = TRUE;
				break;
			}
		}
		if (!has_audio)
			continue;

		gchar **exts = gst_type_find_factory_get_extensions(tff);
		if (!exts)
			continue;
		for ( ; *exts; ++exts )
		{
			str_cat_cptr(media_exts, *exts);
			str_cat_cptr(media_exts, ";");
		}
	}
	str_cat_cptr(media_exts, ";");
	gst_plugin_feature_list_free(tffs);
	g_hash_table_destroy(all_mimes);

	pmng->m_media_file_exts = strdup(STR_TO_CPTR(media_exts));
	str_free(media_exts);

	logger_message(pmng->m_log, 1, _("Supported media file extensions: %s"),
			pmng->m_media_file_exts);

	/* Replace ';' with 0 and calculate maximal extension length */
	size_t max_len = 0, len = 0;
	for ( char *p = pmng->m_media_file_exts;; ++p, ++len )
	{
		bool_t end = ((*p) == 0);

		if ((*p) == ';' || end)
		{
			(*p) = 0;
			if (len > max_len)
				max_len = len;
			len = 0;

			if (end)
				break;
		}
	}
	pmng->m_media_ext_max_len = max_len;

	return TRUE;
} /* End of 'pmng_fill_media_file_exts' function */

/* Initialize plugins */
pmng_t *pmng_init( cfg_node_t *list, logger_t *log, wnd_t *wnd_root )
{
	pmng_t *pmng;

	/* Allocate memory */
	pmng = (pmng_t *)malloc(sizeof(pmng_t));
	if (pmng == NULL)
		return NULL;
	memset(pmng, 0, sizeof(pmng_t));
	pmng->m_cfg_list = list;
	pmng->m_log = log;
	pmng->m_root_wnd = wnd_root;
	
	/* Load plugins */
	if (!pmng_load_plugins(pmng))
	{
		pmng_free(pmng);
		return NULL;
	}

	/* Fill m_media_file_exts */
	pmng_fill_media_file_exts(pmng);

	/* Autostart general plugins */
	pmng_autostart_general(pmng);
	return pmng;
} /* End of 'pmng_init' function */

/* Unitialize plugins */
void pmng_free( pmng_t *pmng )
{
	int i;

	if (pmng == NULL)
		return;

	if (pmng->m_media_file_exts)
		free(pmng->m_media_file_exts);

	for ( i = 0; i < pmng->m_num_plugins; i ++ )
		plugin_free(pmng->m_plugins[i]);
	free(pmng->m_plugins);
	free(pmng);
} /* End of 'pmng_free' function */

/* Add a plugin */
void pmng_add_plugin( pmng_t *pmng, plugin_t *p )
{
	if (pmng == NULL)
		return;
	pmng->m_plugins = (plugin_t **)realloc(pmng->m_plugins,
			sizeof(plugin_t *) * (pmng->m_num_plugins + 1));
	assert(pmng->m_plugins);
	pmng->m_plugins[pmng->m_num_plugins ++] = p;
} /* End of 'pmng_add_plugin' function */

/* Execute a command with a list of parameters */
void pmng_player_command_obj( pmng_t *pmng, char *cmd, 
		cmd_params_list_t *params )
{
	wnd_msg_send(pmng->m_player_wnd, "command", 
			player_msg_command_new(cmd, params));
} /* End of 'pmng_player_command_obj' function */

/* Send command message to player */
void pmng_player_command( pmng_t *pmng, char *cmd, char *params_fmt, ... )
{
	va_list ap;

	va_start(ap, params_fmt);
	pmng_player_command_obj(pmng, cmd, cmd_create_params_va(params_fmt, ap));
	va_end(ap);
} /* End of 'pmng_player_command' function */

/* Autostart general plugins */
void pmng_autostart_general( pmng_t *pmng )
{
	pmng_iterator_t iter;
	if (pmng == NULL)
		return;
	for ( iter = pmng_start_iteration(pmng, PLUGIN_TYPE_GENERAL);; )
	{
		general_plugin_t *p = GENERAL_PLUGIN(pmng_iterate(&iter));
		if (p == NULL)
			break;

		/* Check 'autostart' option for this plugin */
		if (cfg_get_var_bool(PLUGIN(p)->m_cfg, "autostart"))
			genp_start(p);
	}
} /* End of 'pmng_autostart_general' function */

/* Search for input plugin supporting given format */
bool_t pmng_search_format( pmng_t *pmng, char *filename, char *format )
{
	if (pmng == NULL || (!(*filename) && !(*format)))
		return FALSE;

	logger_debug(pmng->m_log, "pmng_search_format(%s, %s)", filename, format);

	for ( char *ext = pmng_first_media_ext(pmng); ext; 
			ext = pmng_next_media_ext(ext) )
	{
		if (!strcasecmp(ext, format))
			return TRUE;
	}
	return FALSE;
} /* End of 'pmng_search_format' function */

/* Apply effect plugins */
int pmng_apply_effects( pmng_t *pmng, byte *data, int len, int fmt, 
		int freq, int channels )
{
	int l = len;
	pmng_iterator_t iter;

	if (pmng == NULL)
		return 0;

	iter = pmng_start_iteration(pmng, PLUGIN_TYPE_EFFECT);
	for ( ;; )
	{
		plugin_t *ep;
		
		/* Apply effect plugin if it is enabled */
		ep = pmng_iterate(&iter);
		if (ep == NULL)
			break;
		if (pmng_is_effect_enabled(pmng, ep))
			l = ep_apply(EFFECT_PLUGIN(ep), data, l, fmt, freq, channels);
	}
	return l;
} /* End of 'pmng_apply_effects' function */

/* Search for plugin with a specified name */
plugin_t *pmng_search_by_name( pmng_t *pmng, char *name, 
		plugin_type_t type_mask )
{
	pmng_iterator_t iter;

	if (pmng == NULL)
		return NULL;

	/* Iterate through plugins */
	iter = pmng_start_iteration(pmng, type_mask);
	for ( ;; )
	{
		plugin_t *p = pmng_iterate(&iter);
		if (p == NULL)
			return NULL;
		else if (!strcmp(name, p->m_name))
			return p;
	}
	return NULL;
} /* End of 'pmng_search_by_name' function */

/* Plugin glob handler */
static void pmng_glob_handler( pmng_t *pmng, plugin_type_t type, const char *path )
{
	plugin_t *p = NULL;

	/* Filter files */
	if (strcmp(util_extension((char*)path), "so"))
		return;
	char *name = (char*)path;

	/* Check if this plugin is not loaded already */
	if (pmng_is_loaded(pmng, name, type))
		return;

	/* Initialize plugin */
	if (type == PLUGIN_TYPE_INPUT)
		p = inp_init(name, pmng);
	else if (type == PLUGIN_TYPE_OUTPUT)
		p = outp_init(name, pmng);
	else if (type == PLUGIN_TYPE_EFFECT)
		p = ep_init(name, pmng);
	else if (type == PLUGIN_TYPE_CHARSET)
		p = csp_init(name, pmng);
	else if (type == PLUGIN_TYPE_GENERAL)
		p = genp_init(name, pmng);
	else if (type == PLUGIN_TYPE_PLIST)
		p = plp_init(name, pmng);
	if (p == NULL)
		return;

	/* Add plugin to the list */
	pmng_add_plugin(pmng, p);

	/* Extra work for output plugins */
	if (type == PLUGIN_TYPE_OUTPUT)
	{
		char *out_plugin;
		out_plugin_t *op = OUTPUT_PLUGIN(p);

		if (pmng->m_cur_out == NULL)
			pmng->m_cur_out = op;
		out_plugin = cfg_get_var(pmng->m_cfg_list, "output-plugin");
		if (out_plugin != NULL && !strcmp(PLUGIN(op)->m_name, out_plugin))
			pmng->m_cur_out = op;
	}
	return;
} /* End of 'pmng_glob_handler' function */

/* Load plugins */
bool_t pmng_load_plugins( pmng_t *pmng )
{
	char path[MAX_FILE_NAME];
	struct
	{
		plugin_type_t m_type;
		char *m_dir;
	} types[] = { { PLUGIN_TYPE_INPUT, "input" }, 
				{ PLUGIN_TYPE_OUTPUT, "output" },
				{ PLUGIN_TYPE_EFFECT, "effect" },
				{ PLUGIN_TYPE_CHARSET, "charset" },
				{ PLUGIN_TYPE_PLIST, "plist" },
				{ PLUGIN_TYPE_GENERAL, "general" } };
	int num_types, i;
	if (pmng == NULL)
		return FALSE;

	/* Load plugins of all types */
	num_types = sizeof(types) / sizeof(types[0]);
	for ( i = 0; i < num_types; i ++ )
	{
		snprintf(path, sizeof(path), "%s/%s", 
				cfg_get_var(pmng->m_cfg_list, "lib-dir"), types[i].m_dir);

		/* Walk file tree */
		char *names[2];
		names[0] = path;
		names[1] = NULL;
		FTS *fts = fts_open(names, FTS_NOCHDIR, NULL);
		if (!fts)
			continue;
		for ( ;; )
		{
			FTSENT *ent = fts_read(fts);
			if (!ent)
				break;
			if (ent->fts_info != FTS_F && ent->fts_info != FTS_SL)
				continue;

			pmng_glob_handler(pmng, types[i].m_type, ent->fts_path);
		}
		fts_close(fts);
	}
	return TRUE;
} /* End of 'pmng_load_plugins' function */

/* Check if specified plugin is already loaded */
bool_t pmng_is_loaded( pmng_t *pmng, char *name, plugin_type_t type )
{
	char *sn;
	pmng_iterator_t iter;

	if (pmng == NULL)
		return FALSE;
	
	/* Get plugin short name */
	sn = util_short_name(name);
	
	/* Search */
	iter = pmng_start_iteration(pmng, type);
	for ( ;; )
	{
		plugin_t *p = pmng_iterate(&iter);
		if (p == NULL)
			break;
		if (!strcmp(sn, p->m_name))
			return TRUE;
	}
	return FALSE;
} /* End of 'pmng_is_loaded' function */

/* Search plugin for content-type */
in_plugin_t *pmng_search_content_type( pmng_t *pmng, char *content )
{
	pmng_iterator_t iter;

	if (content == NULL || pmng == NULL)
		return NULL;

	iter = pmng_start_iteration(pmng, PLUGIN_TYPE_INPUT);
	for ( ;; )
	{
		char types[256], type[80];
		int j, k = 0;
		in_plugin_t *inp = INPUT_PLUGIN(pmng_iterate(&iter));
		if (inp == NULL)
			break;

		inp_get_formats(inp, NULL, types);
		if (types == NULL)
			continue;
		for ( j = 0;; type[k ++] = types[j ++] )
		{
			if (types[j] == 0 || types[j] == ';')
			{
				type[k] = 0;
				if (!strcasecmp(type, content))
					return inp;
				k = 0;
			}
			if (!types[j])
				break;
		}
	}
	return NULL;
} /* End of 'pmng_search_content_type' function */

/* Find charset plugin which supports specified set */
cs_plugin_t *pmng_find_charset( pmng_t *pmng, char *name, int *index )
{
	pmng_iterator_t iter;
	
	if (pmng == NULL)
		return NULL;

	iter = pmng_start_iteration(pmng, PLUGIN_TYPE_CHARSET);
	for ( ;; )
	{
		int num, j;
		cs_plugin_t *csp = CHARSET_PLUGIN(pmng_iterate(&iter));
		if (csp == NULL)
			break;

		num = csp_get_num_sets(csp);
		for ( j = 0; j < num; j ++ )
		{
			char *sn;

			if ((sn = csp_get_cs_name(csp, j)) != NULL)
			{
				if (!strcmp(sn, name))
				{
					*index = j;
					return csp;
				}
			}
		}
	}
	return NULL;
} /* End of 'pmng_find_charset' function */

/* Get configuration variables list */
cfg_node_t *pmng_get_cfg( pmng_t *pmng )
{
	if (pmng == NULL)
		return NULL;
	else
		return pmng->m_cfg_list;
} /* End of 'pmng_get_cfg' function */

/* Get logger */
logger_t *pmng_get_logger( pmng_t *pmng )
{
	if (pmng != NULL)
		return pmng->m_log;
	else
		return NULL;
} /* End of 'pmng_get_log' function */

/* Create a plugin name */
char *pmng_create_plugin_name( char *filename )
{
	char *sn = util_short_name(filename), *name, *ext;

	if (strncmp(sn, "lib", 3))
		return strdup(sn);
	sn += 3;
	name = strdup(sn);
	ext = util_extension(name);
	if (ext != NULL)
	{
		ext --;
		*ext = 0;
	}
	return name;
} /* End of 'pmng_create_plugin_name' function */

/* Stop all general plugins */
void pmng_stop_general_plugins( pmng_t *pmng )
{
	pmng_iterator_t iter;
	iter = pmng_start_iteration(pmng, PLUGIN_TYPE_GENERAL);
	for ( ;; )
	{
		general_plugin_t *p = GENERAL_PLUGIN(pmng_iterate(&iter));
		if (p == NULL)
			break;
		if (genp_is_started(p))
			genp_end(p);
	}
} /* End of 'pmng_stop_general_plugins' function */

/* Start iteration */
pmng_iterator_t pmng_start_iteration( pmng_t *pmng, plugin_type_t type_mask )
{
	pmng_iterator_t iter;
	iter.m_pmng = pmng;
	iter.m_index = 0;
	iter.m_type_mask = type_mask;
	return iter;
} /* End of 'pmng_start_iteration' function */

/* Make an iteration */
plugin_t *pmng_iterate( pmng_iterator_t *iter )
{
	pmng_t *pmng = iter->m_pmng;

	/* Iterate */
	while (iter->m_index < pmng->m_num_plugins &&
			!(pmng->m_plugins[iter->m_index]->m_type & iter->m_type_mask))
		iter->m_index ++;

	/* Return plugin */
	return (iter->m_index >= pmng->m_num_plugins ? NULL : 
			pmng->m_plugins[iter->m_index ++]);
} /* End of 'pmng_iterate' function */

/* Check if effect is enabled */
bool_t pmng_is_effect_enabled( pmng_t *pmng, plugin_t *ep )
{
	char name[256];
	snprintf(name, sizeof(name), "enable-effect-%s", ep->m_name);
	return cfg_get_var_bool(pmng->m_cfg_list, name);
} /* End of 'pmng_is_effect_enabled' function */

/* Enable/disable effect */
void pmng_enable_effect( pmng_t *pmng, plugin_t *ep, bool_t enable )
{
	char name[256];
	snprintf(name, sizeof(name), "enable-effect-%s", ep->m_name);
	cfg_set_var_bool(pmng->m_cfg_list, name, enable);
} /* End of 'pmng_enable_effect' function */

/* Install a hook handler */
int pmng_add_hook_handler( pmng_t *pmng, void (*handler)(char*) )
{
	/* Already occupied */
	if (pmng->m_hook_handler)
		return -1;

	pmng->m_hook_handler = handler;
	return (++pmng->m_hook_id);
} /* End of 'pmng_add_hook_handler' function */

/* Uninstall a hook handler */
void pmng_remove_hook_handler( pmng_t *pmng, int handler_id )
{
	if (pmng->m_hook_id == handler_id)
		pmng->m_hook_handler = NULL;
} /* End of 'pmng_remove_hook_handler' function */

/* Call hook functions */
void pmng_hook( pmng_t *pmng, char *hook )
{
	pmng_iterator_t i;

	logger_debug(pmng->m_log, "hook %s", hook);
	for ( i = pmng_start_iteration(pmng, PLUGIN_TYPE_GENERAL);; )
	{
		general_plugin_t *p = GENERAL_PLUGIN(pmng_iterate(&i));
		if (p == NULL)
			break;
		genp_hooks_callback(p, hook);
	}

	/* Call local handler */
	if (pmng->m_hook_handler)
		pmng->m_hook_handler(hook);
} /* End of 'pmng_hook' function */

/* Start media extensions list iteration */
char *pmng_first_media_ext( pmng_t *pmng )
{
	char *res = pmng->m_media_file_exts;
	if (!(*res))
		return NULL; /* Empty list */
	return res;
} /* End of 'pmng_first_media_exts' function */

/* Get next media extension */
char *pmng_next_media_ext( char *iter )
{
	char *next = strchr(iter, 0) + 1;
	if (!(*next))
		return NULL; /* End of the list */
	return next;
} /* End of 'pmng_next_media_exts' function */

/* Check if file is a play list managed by a plugin */
plist_plugin_t *pmng_is_playlist_prefix( pmng_t *pmng, char *name )
{
	if (!pmng)
		return NULL;

	logger_debug(pmng->m_log, "pmng_is_playlist_prefix(%s)", name);

	pmng_iterator_t iter = pmng_start_iteration(pmng, PLUGIN_TYPE_PLIST);
	for ( ;; )
	{
		plist_plugin_t *plp = PLIST_PLUGIN(pmng_iterate(&iter));
		if (!plp)
			break;

		char *prefix = plp_get_prefix(plp);
		if (!prefix)
			continue;
		if (!strncmp(name, prefix, strlen(prefix)))
			return plp;
	}
	return NULL;
}

/* Check if file is a play list managed by a plugin */
plist_plugin_t *pmng_is_playlist_extension( pmng_t *pmng, char *format )
{
	if (!pmng)
		return NULL;

	logger_debug(pmng->m_log, "pmng_is_playlist(%s)", format);

	pmng_iterator_t iter = pmng_start_iteration(pmng, PLUGIN_TYPE_PLIST);
	for ( ;; )
	{
		char formats[128], ext[10];
		int j, k = 0;
		plist_plugin_t *plp = PLIST_PLUGIN(pmng_iterate(&iter));
		if (!plp)
			break;

		plp_get_formats(plp, formats, NULL);
		for ( j = 0;; ext[k ++] = formats[j ++] )
		{
			if (formats[j] == 0 || formats[j] == ';')
			{
				ext[k] = 0;
				if (!strcasecmp(ext, format))
				{
					logger_debug(pmng->m_log, "extension matches");
					return plp;
				}
				k = 0;
			}
			if (!formats[j])
				break;
		}
	}
	return NULL;
} /* End of 'pmng_is_playlist' function */

/* End of 'pmng.c' file */

