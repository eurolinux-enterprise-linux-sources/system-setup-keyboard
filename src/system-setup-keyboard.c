/*
 * Copyright © 2009 Adel Gadllah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *
 */

#include <libhal.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#define KBDCONFIG "/etc/sysconfig/keyboard"
#define KEY_OPTIONS "input.xkb.options"

gchar* remove_quotes(gchar *str) {
	gchar *tmp;
	gboolean s_offset = FALSE;

	if(str == NULL)
		return NULL;

	if(str[0] == '"') {
		str++;
		s_offset = TRUE;
	}
	if(str[strlen(str) - 1] == '"') 
		str[strlen(str) - 1] = 0;

	tmp = g_strdup(str);

	if(s_offset)
		str--;
	g_free(str);
	return tmp;	
}

/**
 * Append the given value to the current value of matching key.
 * Memory for the returned string must be freed by the caller.
 *
 * Only merges input.xkb.options ATM
 */
gchar *merge_key(LibHalContext *hal_ctx, gchar *udi, gchar* key, gchar *value) {
	gchar *xkb_opts;
	gchar *merged = NULL;

	/* We only need to merge xkb.options */
	if (strcmp(key, KEY_OPTIONS) != 0)
		return g_strdup(value);
	xkb_opts = libhal_device_get_property_string(hal_ctx, udi,
			KEY_OPTIONS, NULL);
	if (!xkb_opts || strlen(xkb_opts) == 0)
		merged = g_strdup(value);
	else if (!value || strlen(value) == 0)
		merged = g_strdup(xkb_opts);
	else
		merged = g_strdup_printf("%s,%s", xkb_opts, value);

	if (xkb_opts)
		libhal_free_string(xkb_opts);

	return merged;
}

int main() {
	GKeyFile *cfg_file;
	gchar *buffer, *conf;

	gchar *map[] = { "layout", "model", "variant", "options" };
	GHashTable* kbd_models;
	gchar *keytable;
	gchar *property;
	guint n, i;
	gchar **list;
	gchar *key, *udi, *tmp;

	/* connect to HAL */
	LibHalContext *hal_ctx;

	if((udi = getenv("UDI")) == NULL) 
		return 1;

	if((hal_ctx = libhal_ctx_init_direct(NULL)) == NULL) {
		if((hal_ctx = libhal_ctx_new()) == NULL) 
			return 1;

		if(!libhal_ctx_set_dbus_connection(hal_ctx, dbus_bus_get(DBUS_BUS_SYSTEM, NULL))) 
			return 1;

		if(!libhal_ctx_init(hal_ctx, NULL)) 
			return 1;
	}
	

	/* Parse the config file */
	cfg_file = g_key_file_new();
	g_file_get_contents(KBDCONFIG, &buffer, NULL, NULL);
	conf = g_strdup_printf("[kbd]\n%s", buffer);
	g_free(buffer);
	g_key_file_load_from_data(cfg_file, conf, strlen(conf), G_KEY_FILE_NONE, NULL);
	property = remove_quotes(g_key_file_get_value(cfg_file, "kbd", "KEYTABLE", NULL));
	if(property == NULL) 
		return 1;

	/* Generate model database */
	kbd_models = g_hash_table_new(g_str_hash, g_str_equal);
#include "keyboards.h"

	/* Read and apply user config */
	keytable = g_hash_table_lookup(kbd_models, (gpointer) property);
	g_free(property);

	list = g_strsplit(keytable, " ", 4);
	n = g_strv_length(list);
	for ( i = 0; i < sizeof (map) / sizeof (*map); i++) {
		gchar *value;
		/* honor user setting */
		tmp = g_ascii_strup(map[i], -1);
		property = remove_quotes(g_key_file_get_value(cfg_file, "kbd", tmp, NULL));
		g_free(tmp);

		if (property != NULL)
			value = property;
		else if (i < n)
			value = list[i];
		else
			continue;

		key = g_strdup_printf("input.xkb.%s", map[i]);
		value = merge_key(hal_ctx, udi, key, value);
		libhal_device_set_property_string(hal_ctx, udi, key, value, NULL);

		if(property != NULL) 
			g_free(property);
		g_free(value);
		g_free(key);
		
	}

	/* cleanup */
	g_free(conf);
	g_free(list);
	libhal_ctx_free(hal_ctx);
	g_hash_table_destroy(kbd_models);

	return 0;
}
