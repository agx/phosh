/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

/*
 * Based on search.js / remoteSearch.js:
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/2d2824b947754abf0ddadd9c1ba9b9f16b0745d3/js/ui/search.js
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/0a7e717e0e125248bace65e170a95ae12e3cdf38/js/ui/remoteSearch.js
 *
 */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "search-provider.h"
#include "search-result-meta.h"
#include "gnome-shell-search-provider.h"

/**
 * PhoshSearchProvider:
 *
 * Provider for handling user-initiated searches
 *
 * The #PhoshSearchProvider class is handle searches initiated by the
 * user. It interfaces with the D-Bus to communicate with the search service,
 * allowing for the retrieval and activation of search results.
 */


typedef struct _PhoshSearchProviderPrivate PhoshSearchProviderPrivate;
struct _PhoshSearchProviderPrivate {
  GAppInfo                 *info;
  PhoshDBusSearchProvider2 *proxy;
  GCancellable             *cancellable;
  GCancellable             *parent_cancellable;
  gulong                    parent_cancellable_handler;
  GDBusProxyFlags           proxy_flags;
  char                     *bus_name;
  char                     *bus_path;
  gboolean                  autostart;
  gboolean                  default_disabled;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearchProvider, phosh_search_provider, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_APP_INFO,
  PROP_BUS_NAME,
  PROP_BUS_PATH,
  PROP_AUTOSTART,
  PROP_DEFAULT_DISABLED,
  PROP_PARENT_CANCELLABLE,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  READY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
got_proxy (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (PhoshSearchProvider) self = PHOSH_SEARCH_PROVIDER (user_data);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->proxy = phosh_dbus_search_provider2_proxy_new_for_bus_finish (res, &error);

  if (!priv->proxy) {
    g_warning ("[%s]: Unable to create proxy: %s", priv->bus_path, error->message);
    return;
  }

  g_debug ("[%s]: Got proxy", priv->bus_path);

  g_signal_emit (self, signals[READY], 0);
}


static void
phosh_search_provider_constructed (GObject *object)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  G_OBJECT_CLASS (phosh_search_provider_parent_class)->constructed (object);

  phosh_dbus_search_provider2_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                 priv->proxy_flags,
                                                 priv->bus_name,
                                                 priv->bus_path,
                                                 priv->cancellable,
                                                 got_proxy,
                                                 g_object_ref (self));
}


static void
phosh_search_provider_finalize (GObject *object)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);

  g_cancellable_disconnect (priv->parent_cancellable,
                            priv->parent_cancellable_handler);

  g_clear_object (&priv->info);
  g_clear_object (&priv->proxy);
  g_clear_object (&priv->parent_cancellable);

  g_clear_pointer (&priv->bus_name, g_free);
  g_clear_pointer (&priv->bus_path, g_free);

  G_OBJECT_CLASS (phosh_search_provider_parent_class)->finalize (object);
}


static void
parent_canceled (GCancellable *cancellable, PhoshSearchProvider *self)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  g_debug ("Provider %s cancelling", priv->bus_name);

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);
  priv->cancellable = g_cancellable_new ();
}


static void
set_parent_cancellable (PhoshSearchProvider *self, GCancellable *parent_cancellable)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  priv->parent_cancellable = parent_cancellable;

  if (priv->parent_cancellable) {
    priv->parent_cancellable_handler = g_cancellable_connect (priv->parent_cancellable,
                                                              G_CALLBACK (parent_canceled),
                                                              self,
                                                              NULL);
  }
}


static void
set_autostart (PhoshSearchProvider *self, gboolean autostart)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  priv->autostart = autostart;
  if (priv->autostart) {
    /* Delay autostart to avoid spawing everything at once */
    priv->proxy_flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION;
  } else {
    /* Don't attempt to autostart */
    priv->proxy_flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
  }
}


static void
phosh_search_provider_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      priv->info = g_value_dup_object (value);
      break;
    case PROP_PARENT_CANCELLABLE:
      set_parent_cancellable (self, g_value_dup_object (value));
      break;
    case PROP_BUS_NAME:
      priv->bus_name = g_value_dup_string (value);
      break;
    case PROP_BUS_PATH:
      priv->bus_path = g_value_dup_string (value);
      break;
    case PROP_AUTOSTART:
      set_autostart (self, g_value_get_boolean (value));
      break;
    case PROP_DEFAULT_DISABLED:
      priv->default_disabled = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_provider_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      g_value_set_object (value, priv->info);
      break;
    case PROP_PARENT_CANCELLABLE:
      g_value_set_object (value, priv->parent_cancellable);
      break;
    case PROP_BUS_NAME:
      g_value_set_string (value, priv->bus_name);
      break;
    case PROP_BUS_PATH:
      g_value_set_string (value, priv->bus_path);
      break;
    case PROP_AUTOSTART:
      g_value_set_boolean (value, priv->autostart);
      break;
    case PROP_DEFAULT_DISABLED:
      g_value_set_boolean (value, priv->default_disabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_provider_class_init (PhoshSearchProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_search_provider_constructed;
  object_class->finalize = phosh_search_provider_finalize;
  object_class->set_property = phosh_search_provider_set_property;
  object_class->get_property = phosh_search_provider_get_property;

  pspecs[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "", "",
                         G_TYPE_APP_INFO,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_PARENT_CANCELLABLE] =
    g_param_spec_object ("parent-cancellable", "", "",
                         G_TYPE_CANCELLABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_BUS_NAME] =
    g_param_spec_string ("bus-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_BUS_PATH] =
    g_param_spec_string ("bus-path", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_AUTOSTART] =
    g_param_spec_boolean ("autostart", "", "",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_DEFAULT_DISABLED] =
    g_param_spec_boolean ("default-disabled", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[READY] = g_signal_new ("ready",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 0, NULL, NULL, NULL,
                                 G_TYPE_NONE, 0);
}


static void
phosh_search_provider_init (PhoshSearchProvider *self)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  priv->cancellable = g_cancellable_new ();
  priv->proxy_flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
  priv->autostart = TRUE;
}


PhoshSearchProvider *
phosh_search_provider_new (const char   *desktop_app_id,
                           GCancellable *parent_cancellable,
                           const char   *bus_path,
                           const char   *bus_name,
                           gboolean      autostart,
                           gboolean      default_disabled)
{
  return g_object_new (PHOSH_TYPE_SEARCH_PROVIDER,
                       "app-info", g_desktop_app_info_new (desktop_app_id),
                       "parent-cancellable", parent_cancellable,
                       "bus-path", bus_path,
                       "bus-name", bus_name,
                       "autostart", autostart,
                       "default-disabled", default_disabled,
                       NULL);
}


GPtrArray *
phosh_search_provider_limit_results (GStrv results, int max)
{
  g_autoptr (GPtrArray) normal = NULL;
  g_autoptr (GPtrArray) special = NULL;
  GPtrArray *array;
  int i = 0;

  g_return_val_if_fail (results != NULL, NULL);

  normal = g_ptr_array_new_full (max * 1.5, g_free);
  special = g_ptr_array_new_full (max * 1.5, g_free);
  array = g_ptr_array_new_full (max * 1.5, g_free);

  while (results[i]) {
    if (G_UNLIKELY (g_str_has_prefix (results[i], "special:")))
      g_ptr_array_add (special, g_strdup (results[i]));
    else
      g_ptr_array_add (normal, g_strdup (results[i]));

    if (special->len == max && normal->len == max)
      break;

    i++;
  }

  for (i = 0; i < max && i < normal->len; i++)
    g_ptr_array_add (array, g_ptr_array_steal_index (normal, i));

  for (i = 0; i < max && i < special->len; i++)
    g_ptr_array_add (array, g_ptr_array_steal_index (special, i));

  return array;
}


static void
result_activated (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = phosh_dbus_search_provider2_call_activate_result_finish (PHOSH_DBUS_SEARCH_PROVIDER2 (source),
                                                                     res,
                                                                     &error);

  if (!success)
    g_warning ("Failed to activate result: %s", error->message);
}


void
phosh_search_provider_activate_result (PhoshSearchProvider *self,
                                       const char          *result,
                                       const char *const   *terms,
                                       guint                timestamp)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  phosh_dbus_search_provider2_call_activate_result (priv->proxy,
                                                    result,
                                                    terms,
                                                    timestamp,
                                                    priv->cancellable,
                                                    result_activated,
                                                    NULL);
}


static void
search_launched (GObject *source, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = phosh_dbus_search_provider2_call_launch_search_finish (PHOSH_DBUS_SEARCH_PROVIDER2 (source),
                                                                   res,
                                                                   &error);

  if (!success)
    g_warning ("Failed to launch search: %s", error->message);
}


void
phosh_search_provider_launch_search (PhoshSearchProvider *self,
                                     const char *const   *terms,
                                     guint                timestamp)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  phosh_dbus_search_provider2_call_launch_search (PHOSH_DBUS_SEARCH_PROVIDER2 (priv->proxy),
                                                  terms,
                                                  timestamp,
                                                  priv->cancellable,
                                                  search_launched,
                                                  NULL);
}


static GIcon *
get_result_icon (PhoshSearchProvider *self, GVariantDict *result_meta)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autofree char *icon_data = NULL;
  GIcon *icon = NULL;
  int width = 0;
  int height = 0;
  int row_stride = 0;
  int has_alpha = 0;
  int sample_size = 0;
  int channels = 0;

  if (g_variant_dict_lookup (result_meta, "icon", "v", &variant)) {
    icon = g_icon_deserialize (variant);
  } else if (g_variant_dict_lookup (result_meta, "gicon", "s", &icon_data)) {
    icon = g_icon_new_for_string (icon_data, &error);

    if (error) {
      g_warning ("[%s]: bad icon: %s", priv->bus_path, error->message);
    }
  } else if (g_variant_dict_lookup (result_meta, "icon-data", "iiibiiay",
                                    &width, &height, &row_stride, &has_alpha,
                                    &sample_size, &channels, &icon_data)) {
    icon = G_ICON (gdk_pixbuf_new_from_data ((guchar *) icon_data,
                                             GDK_COLORSPACE_RGB,
                                             has_alpha,
                                             sample_size,
                                             width,
                                             height,
                                             row_stride,
                                             (GdkPixbufDestroyNotify) g_free,
                                             NULL));
  }

  return icon;
}


static void
got_result_meta (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) metas = NULL;
  g_autoptr (GPtrArray) results = NULL;
  g_autoptr (GTask) task = user_data;
  g_autofree char *bus_path = NULL;
  GVariant *val = NULL;
  GVariantIter iter;
  gboolean success;

  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (g_task_get_source_object (task));

  g_object_get (self, "bus-path", &bus_path, NULL);

  success = phosh_dbus_search_provider2_call_get_result_metas_finish (PHOSH_DBUS_SEARCH_PROVIDER2 (source),
                                                                      &metas,
                                                                      res,
                                                                      &error);

  if (!success)
    g_warning ("[%s]: Failed get result meta: %s", bus_path, error->message);

  results = g_ptr_array_new_full (100, (GDestroyNotify) phosh_search_result_meta_unref);

/*
 * Some providers decide to provide NULL instead of an empty array
 * thus we do what JS does and map NULL to an empty array
 */
  if (metas == NULL)
    metas = g_variant_new ("aa{sv}", NULL);

  g_variant_iter_init (&iter, metas);
  while (g_variant_iter_loop (&iter, "@a{sv}", &val)) {
    g_auto (GVariantDict) dict;
    g_autofree char *id = NULL;
    g_autofree char *name = NULL;
    g_autofree char *desc = NULL;
    g_autofree char *clipboard = NULL;
    g_autoptr (GIcon) icon = NULL;
    PhoshSearchResultMeta *meta;

    g_variant_dict_init (&dict, val);

    if (!g_variant_dict_lookup (&dict, "id", "s", &id)) {
      g_warning ("Result is missing a result id");
      continue;
    }

    if (!g_variant_dict_lookup (&dict, "name", "s", &name)) {
      g_warning ("Result %s is missing a name!", id);
      continue;
    }

    g_variant_dict_lookup (&dict, "description", "s", &desc);

    /* Isn't consistent naming great? */
    g_variant_dict_lookup (&dict, "clipboardText", "s", &clipboard);

    icon = get_result_icon (self, &dict);

    meta = phosh_search_result_meta_new (id, name, desc, icon, clipboard);

    g_ptr_array_add (results, meta);
  }

  g_task_return_pointer (task, g_ptr_array_ref (results), (GDestroyNotify) g_ptr_array_unref);
}


void
phosh_search_provider_get_result_meta (PhoshSearchProvider *self,
                                       GStrv                results,
                                       GAsyncReadyCallback  callback,
                                       gpointer             callback_data)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  GTask *task;

  task = g_task_new (self, priv->cancellable, callback, callback_data);

  g_task_set_source_tag (task, phosh_search_provider_get_result_meta);

  phosh_dbus_search_provider2_call_get_result_metas (PHOSH_DBUS_SEARCH_PROVIDER2 (priv->proxy),
                                                     (const char * const*) results,
                                                     priv->cancellable,
                                                     got_result_meta,
                                                     task);
}


GPtrArray *
phosh_search_provider_get_result_meta_finish (PhoshSearchProvider  *self,
                                              GAsyncResult         *res,
                                              GError              **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_provider_get_result_meta);

  return g_task_propagate_pointer (G_TASK (res), error);
}


static void
got_results (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GTask) task = user_data;
  GStrv results = NULL;

  if (g_task_get_source_tag (task) == phosh_search_provider_get_initial) {
    phosh_dbus_search_provider2_call_get_initial_result_set_finish (PHOSH_DBUS_SEARCH_PROVIDER2 (source),
                                                                    &results,
                                                                    res,
                                                                    &error);
  } else {
    phosh_dbus_search_provider2_call_get_subsearch_result_set_finish (PHOSH_DBUS_SEARCH_PROVIDER2 (source),
                                                                      &results,
                                                                      res,
                                                                      &error);
  }

  if (error)
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_pointer (task, results, (GDestroyNotify) g_strfreev);
}


void
phosh_search_provider_get_initial (PhoshSearchProvider *self,
                                   const char *const   *terms,
                                   GAsyncReadyCallback  callback,
                                   gpointer             callback_data)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  GTask *task;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_provider_get_initial);

  phosh_dbus_search_provider2_call_get_initial_result_set (PHOSH_DBUS_SEARCH_PROVIDER2 (priv->proxy),
                                                           terms,
                                                           priv->cancellable,
                                                           got_results,
                                                           task);
}


GStrv
phosh_search_provider_get_initial_finish (PhoshSearchProvider  *self,
                                          GAsyncResult         *res,
                                          GError              **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_provider_get_initial);

  return g_task_propagate_pointer (G_TASK (res), error);
}


void
phosh_search_provider_get_subsearch (PhoshSearchProvider *self,
                                     const char *const   *results,
                                     const char *const   *terms,
                                     GAsyncReadyCallback  callback,
                                     gpointer             callback_data)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  GTask *task;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_provider_get_subsearch);

  phosh_dbus_search_provider2_call_get_subsearch_result_set (PHOSH_DBUS_SEARCH_PROVIDER2 (priv->proxy),
                                                             results,
                                                             terms,
                                                             priv->cancellable,
                                                             got_results,
                                                             task);
}


GStrv
phosh_search_provider_get_subsearch_finish (PhoshSearchProvider  *self,
                                            GAsyncResult         *res,
                                            GError              **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_provider_get_subsearch);

  return g_task_propagate_pointer (G_TASK (res), error);
}


gboolean
phosh_search_provider_get_ready (PhoshSearchProvider  *self)
{
  PhoshSearchProviderPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SEARCH_PROVIDER (self), FALSE);

  priv = phosh_search_provider_get_instance_private (self);

  return priv->proxy != NULL;
}


const char *
phosh_search_provider_get_bus_path (PhoshSearchProvider  *self)
{
  PhoshSearchProviderPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SEARCH_PROVIDER (self), FALSE);

  priv = phosh_search_provider_get_instance_private (self);

  return priv->bus_path;
}
