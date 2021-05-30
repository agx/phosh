/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"

#include "notify-dbus.h"
#include "log.h"
#include "shell.h"
#include "notifications/notify-manager.h"

#include "testlib.h"

#include <handy.h>

#define BUS_NAME "org.freedesktop.Notifications"
#define OBJECT_PATH "/org/freedesktop/Notifications"

#define POP_TIMEOUT 50000000


typedef struct _Fixture {
  GThread                  *comp_and_shell;
  GAsyncQueue              *queue;
  PhoshTestCompositorState *state;
} Fixture;


static gboolean
stop_shell (gpointer unused)
{
  g_debug ("Stopping shell");
  gtk_main_quit ();

  return G_SOURCE_REMOVE;
}


static gpointer
comp_and_shell_thread (gpointer data)
{
  PhoshShell *shell;
  GLogLevelFlags flags;
  Fixture *fixture = (Fixture *)data;

  /* compositor setup in thread since this invokes gdk already */
  fixture->state = phosh_test_compositor_new ();

  gtk_init (NULL, NULL);
  hdy_init ();

  phosh_log_set_log_domains ("phosh-notify-manager");

  /* Drop warnings from the fatal log mask since there's plenty
   * when running without recommended DBus services */
  flags = g_log_set_always_fatal (0);
  g_log_set_always_fatal (flags & ~G_LOG_LEVEL_WARNING);

  shell = phosh_shell_get_default ();
  g_assert_true (PHOSH_IS_SHELL (shell));

  g_assert_false (phosh_shell_is_startup_finished (shell));

  /* Process events to startup shell */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  g_assert_true (phosh_shell_is_startup_finished (shell));

  g_async_queue_push (fixture->queue, (gpointer)TRUE);

  gtk_main ();

  g_assert_finalize_object (shell);
  phosh_test_compositor_free (fixture->state);

  /* Process events to tear down compositor */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  return NULL;
}


static void
comp_and_shell_setup (Fixture *fixture, gconstpointer unused)
{
  /* Run shell in a thread so we can sync call to the DBus interfaces */
  fixture->queue = g_async_queue_new ();
  fixture->comp_and_shell = g_thread_new ("comp-and-shell-thread", comp_and_shell_thread, fixture);
}


static void
comp_and_shell_teardown (Fixture *fixture, gconstpointer unused)
{
  gdk_threads_add_idle (stop_shell, NULL);
  g_thread_join (fixture->comp_and_shell);
  g_async_queue_unref (fixture->queue);
}


static void
test_phosh_notify_manager_caps (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshNotifyDBusNotifications) proxy = NULL;
  g_auto (GStrv) caps = NULL;
  gboolean success;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_notify_dbus_notifications_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                  G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                  BUS_NAME,
                                                                  OBJECT_PATH,
                                                                  NULL,
                                                                  &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_NOTIFY_DBUS_IS_NOTIFICATIONS_PROXY (proxy));

  success = phosh_notify_dbus_notifications_call_get_capabilities_sync (proxy,
                                                                        &caps,
                                                                        NULL,
                                                                        &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_assert_cmpint (g_strv_length (caps), ==, 4);
  g_assert_true (g_strv_contains ((const gchar * const *)caps, "body"));
  g_assert_true (g_strv_contains ((const gchar * const *)caps, "body-markup"));
  g_assert_true (g_strv_contains ((const gchar * const *)caps, "actions"));
  g_assert_true (g_strv_contains ((const gchar * const *)caps, "icon-static"));
}


static void
test_phosh_notify_manager_server_info (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshNotifyDBusNotifications) proxy = NULL;
  g_auto (GStrv) caps = NULL;
  g_autofree char *name = NULL;
  g_autofree char *vendor = NULL;
  g_autofree char *version = NULL;
  g_autofree char *spec_ver = NULL;
  gboolean success;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_notify_dbus_notifications_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                  G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                  BUS_NAME,
                                                                  OBJECT_PATH,
                                                                  NULL,
                                                                  &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_NOTIFY_DBUS_IS_NOTIFICATIONS_PROXY (proxy));

  success = phosh_notify_dbus_notifications_call_get_server_information_sync (proxy,
                                                                              &name,
                                                                              &vendor,
                                                                              &version,
                                                                              &spec_ver,
                                                                              NULL,
                                                                              &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_assert_cmpstr ("Phosh Notify Daemon", ==, name);
  g_assert_cmpstr ("Phosh", ==, vendor);
  g_assert_cmpstr (PHOSH_VERSION, ==, version);
  g_assert_cmpstr ("1.2", ==, spec_ver);
}


static void
on_new_notofication (gboolean *data)
{
  *data = TRUE;
}


static void
test_phosh_notify_manager_server_notify (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshNotifyDBusNotifications) proxy = NULL;
  g_auto (GStrv) caps = NULL;
  g_autofree char *name = NULL;
  g_autofree char *vendor = NULL;
  g_autofree char *version = NULL;
  g_autofree char *spec_ver = NULL;
  PhoshNotifyManager *nm = NULL;
  gboolean success, notified = FALSE;
  guint id;
  const gchar *const * actions = (const char*[]){ NULL };
  GVariant *hints = g_variant_new ("a{sv}",  NULL);

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_notify_dbus_notifications_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                  G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                  BUS_NAME,
                                                                  OBJECT_PATH,
                                                                  NULL,
                                                                  &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_NOTIFY_DBUS_IS_NOTIFICATIONS_PROXY (proxy));

  /* phosh runs in another thread without locking, so careful */
  nm = phosh_notify_manager_get_default ();
  g_signal_connect_swapped (nm, "new-notification", G_CALLBACK (on_new_notofication), &notified);

  success = phosh_notify_dbus_notifications_call_notify_sync (proxy,
                                                              "com.example.notify",
                                                              -1,
                                                              "",
                                                              "",
                                                              "",
                                                              actions,
                                                              hints,
                                                              3,
                                                              &id,
                                                              NULL,
                                                              &err);
  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpint (id, ==, 1);
  g_assert_true (notified);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/dbus/notify-manager/caps", Fixture, NULL,
              comp_and_shell_setup, test_phosh_notify_manager_caps, comp_and_shell_teardown);
  g_test_add ("/phosh/dbus/notify-manager/server_info", Fixture, NULL,
              comp_and_shell_setup, test_phosh_notify_manager_server_info, comp_and_shell_teardown);
  g_test_add ("/phosh/dbus/notify-manager/notify", Fixture, NULL,
              comp_and_shell_setup, test_phosh_notify_manager_server_notify, comp_and_shell_teardown);

  return g_test_run ();
}
