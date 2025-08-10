/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#include "testlib-search-provider-app.h"
#include "testlib-search-provider.h"

struct _PhoshSearchProviderMock
{
  GObject parent;

  PhoshShellSearchProvider2 *skeleton;
};

G_DEFINE_TYPE (PhoshSearchProviderMock, phosh_search_provider_mock, G_TYPE_OBJECT)


static gboolean
handle_get_initial_result_set (PhoshSearchProviderMock  *self,
                               GDBusMethodInvocation    *invocation,
                               char                    **terms)
{
  /* Results that match "sy" search term */
  const char *results[] = {
    "system-monitor",
    "system-settings",
    "synaptic",
    "symbolic",
    "synergy",
    NULL
  };

  g_debug ("Get Initial Result Set Invoked");

  if (terms) {
    for (int i = 0; terms[i]; i++)
      g_debug ("Initial search term: %s\n", terms[i]);
  }

  phosh_shell_search_provider2_complete_get_initial_result_set (self->skeleton,
                                                                invocation,
                                                                results);
  return TRUE;
}


static gboolean
handle_get_subsearch_result_set (PhoshSearchProviderMock  *self,
                                 GDBusMethodInvocation    *invocation,
                                 char                    **previous_results,
                                 char                    **terms)
{
  /* Results refined to "sys" search term */
  const char *results[] = {
    "system-monitor",
    "system-settings",
    NULL
  };

  g_debug ("Get Subsearch Result Set Invoked");

  if (previous_results) {
    for (int i = 0; previous_results[i]; i++) {
      g_debug ("Previous result: %s\n", previous_results[i]);
    }
  }

  if (terms) {
    for (int i = 0; terms[i]; i++)
      g_debug ("Subsearch term: %s\n", terms[i]);
  }

  phosh_shell_search_provider2_complete_get_subsearch_result_set (self->skeleton,
                                                                  invocation,
                                                                  results);
  return TRUE;
}


static gboolean
handle_get_result_metas (PhoshSearchProviderMock  *self,
                         GDBusMethodInvocation    *invocation,
                         char                    **results)
{
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

  g_debug ("Get Result Metas Invoked");

  for (int i = 0; results[i]; i++) {
    const char *id = "test.app";
    const char *name = "Test App";
    const char *description = "This is a test application.";
    g_autoptr(GIcon) icon = g_themed_icon_new ("utilities-terminal");

    g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (&builder, "{sv}",
                           "id", g_variant_new_string (id));
    g_variant_builder_add (&builder, "{sv}",
                           "name", g_variant_new_string (name));
    g_variant_builder_add (&builder, "{sv}",
                           "icon", g_icon_serialize (icon));
    g_variant_builder_add (&builder, "{sv}",
                           "description", g_variant_new_string (description));
    g_variant_builder_close (&builder);
  }

  if (results) {
    g_debug ("Returning result metas for: \n");
    for (int i = 0; results[i]; i++)
      g_debug ("Result : %s\n", results[i]);
  }

  phosh_shell_search_provider2_complete_get_result_metas (self->skeleton,
                                                          invocation,
                                                          g_variant_builder_end (&builder));
  return TRUE;
}


static gboolean
handle_activate_result (PhoshSearchProviderMock  *self,
                        GDBusMethodInvocation    *invocation,
                        char                     *identifier,
                        char                    **results,
                        guint                     timestamp)
{
  return TRUE;
}


static gboolean
handle_launch_search (PhoshSearchProviderMock  *self,
                      GDBusMethodInvocation    *invocation,
                      char                    **terms,
                      guint                     timestamp)
{
  return TRUE;
}


static void
phosh_search_provider_mock_init (PhoshSearchProviderMock *self)
{
  self->skeleton = phosh_shell_search_provider2_skeleton_new ();

  g_signal_connect_swapped (self->skeleton, "handle-get-initial-result-set",
                            G_CALLBACK (handle_get_initial_result_set), self);
  g_signal_connect_swapped (self->skeleton, "handle-get-subsearch-result-set",
                            G_CALLBACK (handle_get_subsearch_result_set), self);
  g_signal_connect_swapped (self->skeleton, "handle-get-result-metas",
                            G_CALLBACK (handle_get_result_metas), self);
  g_signal_connect_swapped (self->skeleton, "handle-activate-result",
                            G_CALLBACK (handle_activate_result), self);
  g_signal_connect_swapped (self->skeleton, "handle-launch-search",
                            G_CALLBACK (handle_launch_search), self);
}


gboolean
phosh_search_provider_mock_dbus_register (PhoshSearchProviderMock  *self,
                                          GDBusConnection          *connection,
                                          const char               *object_path,
                                          GError                  **error)
{
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (self->skeleton);

  return g_dbus_interface_skeleton_export (skeleton, connection, object_path, error);
}


void
phosh_search_provider_mock_dbus_unregister (PhoshSearchProviderMock *self,
                                            GDBusConnection         *connection,
                                            const char              *object_path)
{
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (self->skeleton);

  if (g_dbus_interface_skeleton_has_connection (skeleton, connection))
      g_dbus_interface_skeleton_unexport_from_connection (skeleton, connection);
}


static void
phosh_search_provider_mock_dispose (GObject *object)
{
  PhoshSearchProviderMock *self = PHOSH_SEARCH_PROVIDER_MOCK (object);

  g_clear_object (&self->skeleton);

  G_OBJECT_CLASS (phosh_search_provider_mock_parent_class)->dispose (object);
}


static void
phosh_search_provider_mock_class_init (PhoshSearchProviderMockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_search_provider_mock_dispose;
}


PhoshSearchProviderMock *
phosh_search_provider_mock_new (void)
{
  return g_object_new (PHOSH_TYPE_SEARCH_PROVIDER_MOCK, NULL);
}

