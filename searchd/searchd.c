/*
 * Copyright (C) 2019 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Zander Brown <zbrown@gnome.org>
 *          Guido Günther <agx@sigxcpu.org>
 *          Gotam Gorabh <gautamy672@gmail.com>
 */

/*
 * Based on gnome-shell's original js implementation
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/2d2824b947754abf0ddadd9c1ba9b9f16b0745d3/js/ui/search.js
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/0a7e717e0e125248bace65e170a95ae12e3cdf38/js/ui/remoteSearch.js
 *
 */

#include "phosh-searchd.h"
#include "searchd.h"
#include "search-provider.h"

#include <glib-unix.h>

#include <search-source.h>
#include <search-result-meta.h>

#define GROUP_NAME "Shell Search Provider"
#define SEARCH_PROVIDERS_SCHEMA "org.gnome.desktop.search-providers"

#define LIMIT_RESULTS 5

/**
 * PhoshSearchApplication:
 *
 * The #PhoshSearchApplication class that serves as a service to facilitate search
 * operations within the Phosh desktop environment. It interacts with various search
 * providers to perform queries and return results.
 */


typedef struct _PhoshSearchApplicationPrivate PhoshSearchApplicationPrivate;
struct _PhoshSearchApplicationPrivate {
  PhoshDBusSearch *object;

  GSettings *settings;

  /* element-type: Phosh.SearchSource */
  GList        *sources;
  /* element-type: Phosh.SearchProvider */
  GHashTable   *providers;

  /* key: char * (object path), value: GVariant * (results) */
  GHashTable   *last_results;
  gboolean      doing_subsearch;

  char         *query;
  GStrv         query_parts;

  GCancellable *cancellable;

  gulong        search_timeout;
  int           outstanding_searches;

  GRegex       *splitter;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearchApplication, phosh_search_application, G_TYPE_APPLICATION)


static void
phosh_search_application_finalize (GObject *object)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (object);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  g_clear_object (&priv->object);

  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->cancellable);
  g_clear_object (&priv->settings);
  g_clear_pointer (&priv->last_results, g_hash_table_destroy);

  g_clear_pointer (&priv->query, g_free);
  g_clear_pointer (&priv->query_parts, g_strfreev);

  g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);
  g_clear_pointer (&priv->providers, g_hash_table_destroy);

  g_clear_pointer (&priv->splitter, g_regex_unref);

  G_OBJECT_CLASS (phosh_search_application_parent_class)->finalize (object);
}


static gboolean
phosh_search_application_dbus_register (GApplication     *app,
                                        GDBusConnection  *connection,
                                        const gchar      *object_path,
                                        GError          **error)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (app);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->object),
                                         connection,
                                         object_path,
                                         error)) {
    g_clear_object (&priv->object);
    return FALSE;
  }

  return G_APPLICATION_CLASS (phosh_search_application_parent_class)->dbus_register (app,
                                                                                     connection,
                                                                                     object_path,
                                                                                     error);
}


static void
phosh_search_application_dbus_unregister (GApplication    *app,
                                          GDBusConnection *connection,
                                          const gchar     *object_path)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (app);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  if (priv->object) {
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (priv->object));

    g_clear_object (&priv->object);
  }

  G_APPLICATION_CLASS (phosh_search_application_parent_class)->dbus_unregister (app,
                                                                                connection,
                                                                                object_path);
}


static void
phosh_search_application_class_init (PhoshSearchApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = phosh_search_application_finalize;

  app_class->dbus_register = phosh_search_application_dbus_register;
  app_class->dbus_unregister = phosh_search_application_dbus_unregister;
}


static gboolean
launch_source (PhoshDBusSearch       *interface,
               GDBusMethodInvocation *invocation,
               const gchar           *source_id,
               guint                  timestamp,
               gpointer               user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GHashTableIter iter;
  gpointer key, value;

  g_debug ("[LaunchSearch] Launching full search in source '%s' at timestamp %u",
           source_id, timestamp);

  g_hash_table_iter_init (&iter, priv->providers);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    PhoshSearchProvider *current_provider = PHOSH_SEARCH_PROVIDER (value);
    const char *bus_path = phosh_search_provider_get_bus_path (current_provider);

    if (!phosh_search_provider_get_ready (current_provider)) {
      g_warning ("[%s]: not ready", bus_path);
      continue;
    }

    if (g_strcmp0 (source_id, bus_path) == 0) {
      g_debug ("Matched source_id '%s' with provider '%s'", source_id, bus_path);

      if (!priv->query_parts) {
        g_warning ("[LaunchSearch] Cannot launch source '%s' — no search terms provided yet", source_id);
        break;
      }

      phosh_search_provider_launch_search (current_provider,
                                           (const char * const*) priv->query_parts,
                                           timestamp);

      break;
    }
  }

  phosh_dbus_search_complete_launch_source (interface, invocation);

  return TRUE;
}


static gboolean
activate_result (PhoshDBusSearch       *interface,
                 GDBusMethodInvocation *invocation,
                 const gchar           *source_id,
                 const gchar           *result_id,
                 guint                  timestamp,
                 gpointer               user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GHashTableIter iter;
  gpointer key, value;

  g_debug ("[ActivateResult] Activating result '%s' from source '%s' at timestamp %u",
           result_id, source_id, timestamp);

  g_hash_table_iter_init (&iter, priv->providers);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    PhoshSearchProvider *current_provider = PHOSH_SEARCH_PROVIDER (value);
    const char *bus_path = phosh_search_provider_get_bus_path (current_provider);

    if (!phosh_search_provider_get_ready (current_provider)) {
      g_warning ("[%s]: not ready", bus_path);
      continue;
    }

    if (g_strcmp0 (source_id, bus_path) == 0) {
      g_debug ("Matched source_id '%s' with provider '%s'", source_id, bus_path);

      if (!priv->query_parts) {
        g_warning ("[ActivateResult] Cannot activate result '%s' — no search terms available", result_id);
        break;
      }

      phosh_search_provider_activate_result (current_provider,
                                             result_id,
                                             (const char * const*) priv->query_parts,
                                             timestamp);

      break;
    }
  }

  phosh_dbus_search_complete_activate_result (interface, invocation);

  return TRUE;
}


static gboolean
get_sources (PhoshDBusSearch *interface, GDBusMethodInvocation *invocation, gpointer user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GVariantBuilder builder;
  g_autoptr (GVariant) result = NULL;
  g_autoptr (GList) list = NULL;

  list = priv->sources;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ssu)"));

  while (list) {
    g_variant_builder_add_value (&builder, phosh_search_source_serialise (list->data));

    list = g_list_next (list);
  }

  result = g_variant_builder_end (&builder);

  phosh_dbus_search_complete_get_sources (interface, invocation, g_variant_ref (result));

  return TRUE;
}


static gboolean
get_last_results (PhoshDBusSearch *interface, GDBusMethodInvocation *invocation, gpointer user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GVariantBuilder builder;
  GHashTableIter iter;
  gpointer key, value;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{saa{sv}}"));

  g_hash_table_iter_init (&iter, priv->last_results);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    const char *source_id = key;
    GVariant *results = value;
    GVariantIter result_iter;
    GVariantBuilder results_array;
    GVariant *result;

    g_variant_builder_init (&results_array, G_VARIANT_TYPE ("aa{sv}"));
    g_variant_iter_init (&result_iter, results);

    while (g_variant_iter_next (&result_iter, "@a{sv}", &result))
      g_variant_builder_add_value (&results_array, result);

    g_variant_builder_add (&builder, "{saa{sv}}", source_id, &results_array);
  }

  phosh_dbus_search_complete_get_last_results (interface,
                                               invocation,
                                               g_variant_builder_end (&builder));

  return TRUE;
}


static void
got_metas (GObject *source, GAsyncResult *res, gpointer user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) result = NULL;
  GVariantBuilder builder;
  GPtrArray *metas;
  char *bus_path;

  metas = phosh_search_provider_get_result_meta_finish (PHOSH_SEARCH_PROVIDER (source),
                                                        res,
                                                        &error);

  g_object_get (source, "bus-path", &bus_path, NULL);

  if (error) {
    g_critical ("Failed to load results %s", error->message);
    return;
  }

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

  for (int i = 0; i < metas->len; i++) {
    g_variant_builder_add_value (&builder,
                                 phosh_search_result_meta_serialise (g_ptr_array_index (metas, i)));
  }

  result = g_variant_builder_end (&builder);

  phosh_dbus_search_emit_source_results_changed (priv->object, bus_path, g_variant_ref (result));

  g_hash_table_insert (priv->last_results, bus_path, g_variant_ref (result));
}


struct GotResultsData {
  gboolean initial;
  PhoshSearchApplication *self;
};


static void
got_results (GObject *source, GAsyncResult *res, gpointer user_data)
{
  PhoshSearchApplicationPrivate *priv;
  struct GotResultsData *data = user_data;
  g_autoptr (GError) error = NULL;
  GStrv results = NULL;
  g_autoptr (GPtrArray) sub_res = NULL;
  g_autofree char *bus_path = NULL;
  GStrv sub_res_strv = NULL;

  priv = phosh_search_application_get_instance_private (data->self);

  g_object_get (source, "bus-path", &bus_path, NULL);

  if (data->initial) {
    results = phosh_search_provider_get_initial_finish (PHOSH_SEARCH_PROVIDER (source),
                                                        res,
                                                        &error);
  } else {
    results = phosh_search_provider_get_subsearch_finish (PHOSH_SEARCH_PROVIDER (source),
                                                          res,
                                                          &error);
  }

  if (error) {
    g_warning ("[%s]: %s", bus_path, error->message);
  } else if (results) {
    sub_res = phosh_search_provider_limit_results (results, LIMIT_RESULTS);

    sub_res_strv = g_new (char *, sub_res->len + 1);

    for (int i = 0; i < sub_res->len; i++)
      sub_res_strv[i] = g_ptr_array_index (sub_res, i);

    sub_res_strv[sub_res->len] = NULL;

    phosh_search_provider_get_result_meta (PHOSH_SEARCH_PROVIDER (source),
                                           sub_res_strv,
                                           got_metas,
                                           data->self);
  }

  priv->outstanding_searches--;

  /* If all searches are done, emit the signal */
  if (priv->outstanding_searches == 0) {
    g_debug ("Query finished: All outstanding searches completed.\n");
    phosh_dbus_search_emit_query_finished (priv->object);
  }

  g_object_unref (data->self);
  g_free (data);
}


static void
search (PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, priv->providers);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    PhoshSearchProvider *provider = PHOSH_SEARCH_PROVIDER (value);
    char *bus_path;
    struct GotResultsData *data;

    g_object_get (provider, "bus-path", &bus_path, NULL);

    if (!phosh_search_provider_get_ready (provider)) {
      g_warning ("[%s]: not ready", bus_path);
      continue;
    }

    data = g_new (struct GotResultsData, 1);
    data->self = g_object_ref (self);

    /* Increment counter for each provider that will be queried */
    priv->outstanding_searches++;

    if (priv->doing_subsearch && g_hash_table_contains (priv->last_results, bus_path)) {
      data->initial = FALSE;
      phosh_search_provider_get_subsearch (provider,
                                           g_hash_table_lookup (priv->last_results, bus_path),
                                           (const char * const*) priv->query_parts,
                                           got_results,
                                           data);
    } else {
      data->initial = TRUE;
      phosh_search_provider_get_initial (provider,
                                         (const char * const*) priv->query_parts,
                                         got_results,
                                         data);
    }
  }

  g_hash_table_remove_all (priv->last_results);

  if (priv->search_timeout != 0) {
    g_source_remove (priv->search_timeout);
    priv->search_timeout = 0;
  }
}


static gboolean
search_timeout (gpointer user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  priv->search_timeout = 0;

  search (self);

  /* Edge case: if no providers are ready/active, emit immediately */
  if (priv->outstanding_searches == 0)
    phosh_dbus_search_emit_query_finished (priv->object);

  return G_SOURCE_REMOVE;
}


static gboolean
query (PhoshDBusSearch       *interface,
       GDBusMethodInvocation *invocation,
       const char            *query,
       gpointer               user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  g_autofree char *striped = NULL;
  g_auto (GStrv) parts = NULL;
  int len = 0;

  striped = g_strstrip (g_strdup (query));
  parts = g_regex_split (priv->splitter, striped, 0);

  len = parts ? g_strv_length (parts) : 0;
  if (priv->query_parts && g_strv_equal ((const char *const *) priv->query_parts,
                                         (const char *const *) parts)) {
    phosh_dbus_search_complete_query (interface, invocation, FALSE);
    phosh_dbus_search_emit_query_finished (interface);

    return TRUE;
  }

  g_cancellable_cancel (priv->cancellable);
  /* Avoid reusing cancellable by creating a new one */
  g_clear_object (&priv->cancellable);
  priv->cancellable = g_cancellable_new ();

  if (len == 0) {
    g_clear_pointer (&priv->query, g_free);
    g_clear_pointer (&priv->query_parts, g_strfreev);

    priv->doing_subsearch = FALSE;

    phosh_dbus_search_complete_query (interface, invocation, FALSE);
    phosh_dbus_search_emit_query_finished (interface);

    return TRUE;
  }

  if (priv->query != NULL)
    priv->doing_subsearch = g_str_has_prefix (query, priv->query);
  else
    priv->doing_subsearch = FALSE;

  g_clear_pointer (&priv->query_parts, g_strfreev);
  priv->query_parts = g_strdupv (parts);
  priv->query = g_strdup (query);

  if (priv->search_timeout == 0)
    priv->search_timeout = g_timeout_add (150, search_timeout, self);

  phosh_dbus_search_complete_query (interface, invocation, TRUE);

  return TRUE;
}


/* Sort algorithm taken straight from remoteSearch.js, comments and all */
static int
sort_sources (gconstpointer a, gconstpointer b, gpointer user_data)
{
  GAppInfo *app_a = NULL;
  GAppInfo *app_b = NULL;
  const char *app_id_a = NULL;
  const char *app_id_b = NULL;
  GStrv order = user_data;
  int idx_a = -1;
  int idx_b = -1;
  int i = 0;

  g_return_val_if_fail (a != NULL, -1);
  g_return_val_if_fail (b != NULL, -1);

  app_a = phosh_search_source_get_app_info ((PhoshSearchSource *) a);
  app_b = phosh_search_source_get_app_info ((PhoshSearchSource *) b);

  g_return_val_if_fail (G_IS_APP_INFO (app_a), -1);
  g_return_val_if_fail (G_IS_APP_INFO (app_b), -1);

  app_id_a = g_app_info_get_id (app_a);
  app_id_b = g_app_info_get_id (app_b);

  while ((order[i])) {
    if (idx_a == -1 && g_strcmp0 (order[i], app_id_a) == 0)
      idx_a = i;

    if (idx_b == -1 && g_strcmp0 (order[i], app_id_b) == 0)
      idx_b = i;

    if (idx_a != -1 && idx_b != -1)
      break;

    i++;
  }

  /* if no provider is found in the order, use alphabetical order */
  if ((idx_a == -1) && (idx_b == -1))
    return g_utf8_collate (g_app_info_get_name (app_a), g_app_info_get_name (app_b));

  /* if providerA isn't found, it's sorted after providerB */
  if (idx_a == -1)
    return 1;

  /* if providerB isn't found, it's sorted after providerA */
  if (idx_b == -1)
    return -1;

  /* finally, if both providers are found, return their order in the list */
  return (idx_a - idx_b);
}


static void
reload_providers (PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  const char *const *data_dirs = g_get_system_data_dirs ();
  const char *data_dir = NULL;
  g_autolist (PhoshSearchSource) sources = NULL;
  /* This skip the normal sorting */
  g_autoptr (PhoshSearchSource) settings = NULL;
  g_auto (GStrv) enabled = NULL;
  g_auto (GStrv) disabled = NULL;
  g_auto (GStrv) sort_order = NULL;
  GList *list;
  int i = 0;

  if (g_settings_get_boolean (priv->settings, "disable-external"))
    g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);

  enabled = g_settings_get_strv (priv->settings, "enabled");
  disabled = g_settings_get_strv (priv->settings, "disabled");
  sort_order = g_settings_get_strv (priv->settings, "sort-order");

  while ((data_dir = data_dirs[i])) {
    g_autofree char *dir = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GDir) contents = NULL;
    const char* name = NULL;

    i++;

    dir = g_build_filename (data_dir, "gnome-shell", "search-providers", NULL);
    contents = g_dir_open (dir, 0, &error);

    if (error) {
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_warning ("Can't look for in %s: %s", dir, error->message);

      g_clear_error (&error);
      continue;
    }

    while ((name = g_dir_read_name (contents))) {
      g_autofree char *provider = NULL;
      g_autofree char *bus_path = NULL;
      g_autofree char *bus_name = NULL;
      g_autofree char *desktop_id = NULL;
      g_autoptr (GKeyFile) data = NULL;
      g_autoptr (PhoshSearchProvider) provider_object = NULL;
      g_autoptr (PhoshSearchSource) source = NULL;
      g_autoptr (GAppInfo) info = NULL;
      int version = 0;
      gboolean autostart = TRUE;
      gboolean autostart_tmp = FALSE;
      gboolean default_disabled = FALSE;
      gboolean default_disabled_tmp = FALSE;

      provider = g_build_filename (dir, name, NULL);
      data = g_key_file_new ();

      g_key_file_load_from_file (data, provider, G_KEY_FILE_NONE, &error);

      if (error) {
        g_warning ("Can't read %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!g_key_file_has_group (data, GROUP_NAME)) {
        g_warning ("%s doesn't define a search provider", provider);
        continue;
      }


      version = g_key_file_get_integer (data, GROUP_NAME, "Version", &error);

      if (error) {
        g_warning ("Failed to fetch provider version %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (version < 2) {
        g_warning ("Provider %s implements version %i but we only support version 2 and up", provider, version);
        continue;
      }


      desktop_id = g_key_file_get_string (data, GROUP_NAME, "DesktopId", &error);
      if (error) {
        g_warning ("Failed to fetch provider desktop id %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }
      if (!desktop_id) {
        g_warning ("Provider %s doesn't specify a desktop id", provider);
        continue;
      }


      bus_name = g_key_file_get_string (data, GROUP_NAME, "BusName", &error);
      if (error) {
        g_warning ("Failed to fetch provider bus name %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }
      if (!bus_name) {
        g_warning ("Provider %s doesn't specify a bus name", provider);
        continue;
      }


      bus_path = g_key_file_get_string (data, GROUP_NAME, "ObjectPath", &error);
      if (error) {
        g_warning ("Failed to fetch provider bus path %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }
      if (!bus_path) {
        g_warning ("Provider %s doesn't specify a bus path", provider);
        continue;
      }

      if (g_hash_table_contains (priv->providers, bus_path)) {
        g_debug ("We already have a provider for %s, ignoring %s", bus_path, provider);
        continue;
      }

      autostart_tmp = g_key_file_get_boolean (data, GROUP_NAME, "AutoStart", &error);

      if (G_LIKELY (error))
        g_clear_error (&error);
      else
        autostart = autostart_tmp;

      default_disabled_tmp = g_key_file_get_boolean (data, GROUP_NAME, "DefaultDisabled", &error);
      if (G_LIKELY (error))
        g_clear_error (&error);
      else
        default_disabled = default_disabled_tmp;

      if (!default_disabled) {
        if (g_strv_contains ((const char * const*) disabled, desktop_id)) {
          g_debug ("Provider %s has been disabled", provider);
          continue;
        }
      } else {
        if (!g_strv_contains ((const char * const*) enabled, desktop_id)) {
          g_debug ("Provider %s hasn't been enabled", provider);
          continue;
        }
      }

      provider_object = phosh_search_provider_new (desktop_id,
                                                   priv->cancellable,
                                                   bus_path,
                                                   bus_name,
                                                   autostart,
                                                   default_disabled);

      if (G_UNLIKELY (g_str_equal (desktop_id, "org.gnome.Settings.desktop"))) {
        info = G_APP_INFO (g_desktop_app_info_new (desktop_id));
        settings = phosh_search_source_new (bus_path, info);
      } else {
        info = G_APP_INFO (g_desktop_app_info_new (desktop_id));
        source = phosh_search_source_new (bus_path, info);
        sources = g_list_prepend (sources, phosh_search_source_ref (source));
      }

      g_hash_table_insert (priv->providers, g_strdup (bus_path), g_object_ref (provider_object));
    }
  }

  sources = g_list_sort_with_data (sources, sort_sources, sort_order);

  if (settings)
    sources = g_list_prepend (sources, phosh_search_source_ref (settings));

  list = sources;
  i = 0;

  while (list) {
    phosh_search_source_set_position (list->data, i);

    i++;
    list = g_list_next (list);
  }

  g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);
  priv->sources = g_list_copy_deep (sources, (GCopyFunc) phosh_search_source_ref, NULL);
}


static void
reload_providers_apps_changed (GAppInfoMonitor *monitor, PhoshSearchApplication *self)
{
  reload_providers (self);
}


static void
phosh_search_application_init (PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->doing_subsearch = FALSE;
  priv->search_timeout = 0;
  priv->outstanding_searches = 0;

  priv->splitter = g_regex_new ("\\s+", G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, &error);
  if (error)
    g_error ("Bad Regex: %s", error->message);

  priv->last_results = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify) g_variant_unref);
  priv->providers = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           (GDestroyNotify) g_object_unref);

  priv->cancellable = g_cancellable_new ();

  priv->settings = g_settings_new (SEARCH_PROVIDERS_SCHEMA);
  g_object_connect (priv->settings,
                    "swapped-object-signal::changed::disabled", reload_providers, self,
                    "swapped-object-signal::changed::enabled", reload_providers, self,
                    "swapped-object-signal::changed::disable-external", reload_providers, self,
                    "swapped-object-signal::changed::sort-order", reload_providers, self,
                    NULL);

  g_signal_connect (g_app_info_monitor_get (),
                    "changed",
                    G_CALLBACK (reload_providers_apps_changed), self);

  priv->object = phosh_dbus_search_skeleton_new ();
  g_object_connect (priv->object,
                    "object-signal::handle-launch-source", launch_source, self,
                    "object-signal::handle-activate-result", activate_result, self,
                    "object-signal::handle-get-sources", get_sources, self,
                    "object-signal::handle-query", query, self,
                    "object-signal::handle-get-last-results", get_last_results, self,
                    NULL);

  reload_providers (self);

  g_application_hold (G_APPLICATION (self));
}


static gboolean
on_shutdown_signal (gpointer user_data)
{
  GApplication *app = G_APPLICATION (user_data);

  g_debug ("Exiting gracefully.");

  g_application_release (app);

  return G_SOURCE_REMOVE;
}


int
main (int argc, char **argv)
{
  g_autoptr (GApplication) app = NULL;

  app = g_object_new (PHOSH_TYPE_SEARCH_APPLICATION,
                      "application-id", "mobi.phosh.Shell.Search",
                      "flags", G_APPLICATION_IS_SERVICE,
                      NULL);
  g_unix_signal_add (SIGTERM, on_shutdown_signal, app);
  g_unix_signal_add (SIGINT, on_shutdown_signal, app);

  return g_application_run (app, argc, argv);
}
