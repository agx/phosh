/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Zander Brown <zbrown@gnome.org>
 *          Gotam Gorabh <gautamy672@gmail.com>
 */

#include "search-client.h"
#include "search-result-meta.h"
#include "search-source.h"
#include "phosh-searchd.h"

/**
 * PhoshSearchClient:
 *
 * Client for interfacing with the Phosh search service
 *
 * The #PhoshSearchClient class provides an interface to interact with the
 * Phosh search service over D-Bus. It allows client to query for results
 * from different search sources.
 */

enum State {
  CREATED,
  STARTING,
  READY,
};

enum {
  SOURCE_RESULTS_CHANGED,
  QUERY_FINISHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


typedef struct _PhoshSearchClientPrivate PhoshSearchClientPrivate;
struct _PhoshSearchClientPrivate {
  PhoshDBusSearch *server;

  enum State       state;

  GRegex          *highlight;
  GRegex          *splitter;

  GCancellable    *cancellable;
};

static void async_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSearchClient, phosh_search_client, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (PhoshSearchClient)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_iface_init))


static void
phosh_search_client_finalize (GObject *object)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (object);
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);

  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->server);
  g_clear_object (&priv->cancellable);

  g_clear_pointer (&priv->highlight, g_regex_unref);
  g_clear_pointer (&priv->splitter, g_regex_unref);

  G_OBJECT_CLASS (phosh_search_client_parent_class)->finalize (object);
}


static void
phosh_search_client_class_init (PhoshSearchClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_search_client_finalize;

  signals[SOURCE_RESULTS_CHANGED] = g_signal_new ("source-results-changed",
                                                  G_TYPE_FROM_CLASS (klass),
                                                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                  0,
                                                  NULL, NULL, NULL,
                                                  G_TYPE_NONE,
                                                  2,
                                                  G_TYPE_STRING,
                                                  G_TYPE_PTR_ARRAY);

  signals[QUERY_FINISHED] = g_signal_new ("query-finished",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST,
                                          0,
                                          NULL, NULL, NULL,
                                          G_TYPE_NONE,
                                          0);
}


static void
on_source_results_changed (PhoshDBusSearch   *server,
                           const char        *source_id,
                           GVariant          *variant,
                           PhoshSearchClient *self)
{
  GVariantIter iter;
  GVariant *item;
  GPtrArray *results = NULL;

  results = g_ptr_array_new_with_free_func ((GDestroyNotify) phosh_search_result_meta_unref);

  g_variant_iter_init (&iter, variant);
  while ((item = g_variant_iter_next_value (&iter))) {
    g_autoptr (PhoshSearchResultMeta) result = NULL;

    result = phosh_search_result_meta_deserialise (item);

    g_ptr_array_add (results, phosh_search_result_meta_ref (result));

    g_clear_pointer (&item, g_variant_unref);
  }

  g_signal_emit (self,
                 signals[SOURCE_RESULTS_CHANGED],
                 g_quark_from_string (source_id),
                 source_id,
                 results);
}


static void
on_query_finished (PhoshDBusSearch *server, gpointer user_data)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (user_data);

  g_signal_emit (self, signals[QUERY_FINISHED], 0);
}


struct InitData {
  PhoshSearchClient *self;
  GTask             *task;
};

static void
got_search (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  g_autofree struct InitData *data = user_data;
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (data->self);

  priv->server = phosh_dbus_search_proxy_new_finish (result, &error);

  priv->state = READY;

  if (!priv->server) {
    g_task_return_error (data->task, error);
    return;
  }

  g_signal_connect (priv->server, "source-results-changed", G_CALLBACK (on_source_results_changed), data->self);
  g_signal_connect (priv->server, "query-finished", G_CALLBACK (on_query_finished), data->self);

  g_task_return_boolean (data->task, TRUE);
}


static void
phosh_search_client_init_async (GAsyncInitable      *initable,
                                int                  io_priority,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (initable);
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  GTask* task = NULL;
  struct InitData *data;

  switch (priv->state) {
      case CREATED:
        priv->state = STARTING;

        task = g_task_new (initable, cancellable, callback, user_data);

        data = g_new0 (struct InitData, 1);
        data->self = self;
        data->task = task;

        phosh_dbus_search_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                             G_DBUS_PROXY_FLAGS_NONE,
                                             "mobi.phosh.Shell.Search",
                                             "/mobi/phosh/Shell/Search",
                                             cancellable,
                                             got_search,
                                             data);
        break;
      case STARTING:
        g_critical ("Already initialising");
        break;
      case READY:
        g_critical ("Already initialised");
        break;
      default:
        g_assert_not_reached ();
    }
}


static gboolean
phosh_search_client_init_finish (GAsyncInitable *initable, GAsyncResult *result, GError **error)
{
  g_return_val_if_fail (g_task_is_valid (result, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}


static void
async_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = phosh_search_client_init_async;
  iface->init_finish = phosh_search_client_init_finish;
}


static void
phosh_search_client_init (PhoshSearchClient *self)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->highlight = NULL;
  priv->splitter = g_regex_new ("\\s+",
                                G_REGEX_CASELESS | G_REGEX_MULTILINE,
                                0,
                                &error);
  priv->state = CREATED;
  priv->cancellable = g_cancellable_new ();

  if (error)
    g_error ("Bad Regex: %s", error->message);
}


static void
got_query (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GTask) task = user_data;
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (g_task_get_source_object (task));
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  gboolean out_searching;
  gboolean success;

  success = phosh_dbus_search_call_query_finish (priv->server, &out_searching, result, &error);

  if (!success) {
    g_critical ("Unable to send search: %s", error->message);
    g_task_return_error (task, error);
  }

  g_task_return_boolean (task, out_searching);
}


void
phosh_search_client_query (PhoshSearchClient    *self,
                           const char           *query,
                           GAsyncReadyCallback  callback,
                           gpointer             callback_data)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autofree char *regex_terms = NULL;
  g_autofree char *regex = NULL;
  g_autofree char *striped = NULL;
  g_auto (GStrv) parts = NULL;
  g_auto (GStrv) escaped = NULL;
  g_autoptr (GError) error = NULL;
  GTask *task = NULL;
  int len = 0;
  int i = 0;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_client_query);

  phosh_dbus_search_call_query (priv->server, query, priv->cancellable, got_query, task);

  striped = g_strstrip (g_strdup (query));
  parts = g_regex_split (priv->splitter, striped, 0);

  len = parts ? g_strv_length (parts) : 0;

  escaped = g_new0 (char *, len + 1);

  while (parts[i]) {
    escaped[i] = g_regex_escape_string (parts[i], -1);

    i++;
  }
  escaped[len] = NULL;

  regex_terms = g_strjoinv ("|", escaped);
  regex = g_strconcat ("(", regex_terms, ")", NULL);
  priv->highlight = g_regex_new (regex, G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, &error);

  if (error) {
    g_warning ("Unable to prepare highlighter: %s", error->message);
    g_clear_error (&error);
    priv->highlight = NULL;
  }
}


gboolean
phosh_search_client_query_finish (PhoshSearchClient  *self,
                                  GAsyncResult       *res,
                                  GError            **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_client_query);


  return g_task_propagate_boolean (G_TASK (res), error);
}


char *
phosh_search_client_markup_string (PhoshSearchClient *self, const char *string)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  char *marked = NULL;

  if (string == NULL)
    return NULL;

  marked = g_regex_replace (priv->highlight, string, -1, 0, "<b>\\1</b>", 0, &error);

  if (error)
    return g_strdup (string);

  return marked;
}


void
phosh_search_client_new (GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  g_async_initable_new_async (PHOSH_TYPE_SEARCH_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              NULL);
}


PhoshSearchClient *
phosh_search_client_new_finish (GObject *source, GAsyncResult *result, GError **error)
{
  return PHOSH_SEARCH_CLIENT (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                                           result,
                                                           error));
}


static void
got_last_results (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GTask) task = user_data;
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (g_task_get_source_object (task));
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) results = NULL;
  gboolean success;

  success = phosh_dbus_search_call_get_last_results_finish (priv->server,
                                                            &results,
                                                            result,
                                                            &error);

  if (!success) {
    g_critical ("Unable to get last results: %s", error->message);
    g_task_return_error (task, error);
    return;
  }

  g_task_return_pointer (task, g_variant_ref (results), (GDestroyNotify) g_variant_unref);
}


void
phosh_search_client_get_last_results (PhoshSearchClient *self,
                                      GAsyncReadyCallback callback,
                                      gpointer callback_data)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  GTask *task;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_client_get_last_results);

  phosh_dbus_search_call_get_last_results (priv->server,
                                           priv->cancellable,
                                           got_last_results,
                                           task);
}


GVariant *
phosh_search_client_get_last_results_finish (PhoshSearchClient *self,
                                             GAsyncResult *res,
                                             GError **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_client_get_last_results);

  return g_task_propagate_pointer (G_TASK (res), error);
}


static void
got_called_activate_result (GObject      *source_object,
                            GAsyncResult *async_result,
                            gpointer      user_data)
{
  g_autoptr (GTask) task = user_data;
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (g_task_get_source_object (task));
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = phosh_dbus_search_call_activate_result_finish (priv->server, async_result, &error);

  if (!success) {
    g_critical ("%s: Could not activate result for this request.\n", error->message);
    g_task_return_error (task, error);
  }

  g_task_return_boolean (task, success);
}


void
phosh_search_client_activate_result (PhoshSearchClient   *self,
                                     const char          *source_id,
                                     const char          *result_id,
                                     GAsyncReadyCallback  callback,
                                     gpointer             callback_data)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  GDateTime *dt = g_date_time_new_now_utc ();
  GTask *task = NULL;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_client_activate_result);

  phosh_dbus_search_call_activate_result (priv->server,
                                          source_id,
                                          result_id,
                                          (int) g_date_time_get_seconds (dt),
                                          priv->cancellable,
                                          got_called_activate_result,
                                          task);
}


gboolean
phosh_search_client_activate_result_finish (PhoshSearchClient  *self,
                                            GAsyncResult       *res,
                                            GError            **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_client_activate_result);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static void
got_called_launch_source (GObject      *source_object,
                          GAsyncResult *async_result,
                          gpointer      user_data)
{
  g_autoptr (GTask) task = user_data;
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (g_task_get_source_object (task));
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = phosh_dbus_search_call_launch_source_finish (priv->server, async_result, &error);

  if (!success) {
    g_critical ("%s: Could not launch search for this request.\n", error->message);
    g_task_return_error (task, error);
  }

  g_task_return_boolean (task, success);

}


void
phosh_search_client_launch_source (PhoshSearchClient   *self,
                                   const char          *source_id,
                                   GAsyncReadyCallback  callback,
                                   gpointer             callback_data)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  GDateTime *dt = g_date_time_new_now_utc ();
  GTask *task = NULL;

  task = g_task_new (self, priv->cancellable, callback, callback_data);
  g_task_set_source_tag (task, phosh_search_client_launch_source);

  phosh_dbus_search_call_launch_source (priv->server,
                                        source_id,
                                        (int) g_date_time_get_seconds (dt),
                                        priv->cancellable,
                                        got_called_launch_source,
                                        task);
}


gboolean
phosh_search_client_launch_source_finish (PhoshSearchClient  *self,
                                          GAsyncResult       *res,
                                          GError            **error)
{
  g_assert_true (g_task_is_valid (res, self));
  g_assert_true (g_task_get_source_tag (G_TASK (res)) == phosh_search_client_launch_source);

  return g_task_propagate_boolean (G_TASK (res), error);
}

