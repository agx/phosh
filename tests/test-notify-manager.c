/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"

#include "notify-dbus.h"
#include "shell.h"
#include "notifications/notify-manager.h"

#include "testlib-full-shell.h"

#define BUS_NAME "org.freedesktop.Notifications"
#define OBJECT_PATH "/org/freedesktop/Notifications"

#define POP_TIMEOUT 50000000


static void
test_phosh_notify_manager_caps (PhoshTestFullShellFixture *fixture, gconstpointer unused)
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
test_phosh_notify_manager_server_info (PhoshTestFullShellFixture *fixture, gconstpointer unused)
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
on_new_notification (gboolean *data)
{
  *data = TRUE;
}


static void
test_phosh_notify_manager_server_notify (PhoshTestFullShellFixture *fixture, gconstpointer unused)
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
  g_signal_connect_swapped (nm, "new-notification", G_CALLBACK (on_new_notification), &notified);

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
  g_autofree gchar *display = NULL;
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  /* Preserve DISPLAY for wlroots x11 backend */
  cfg = phosh_test_full_shell_fixture_cfg_new (g_getenv ("DISPLAY"), "phosh-notify-manager");

  g_test_add ("/phosh/dbus/notify-manager/caps", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_phosh_notify_manager_caps, phosh_test_full_shell_teardown);
  g_test_add ("/phosh/dbus/notify-manager/server_info", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_phosh_notify_manager_server_info, phosh_test_full_shell_teardown);
  g_test_add ("/phosh/dbus/notify-manager/notify", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_phosh_notify_manager_server_notify, phosh_test_full_shell_teardown);

  return g_test_run ();
}
