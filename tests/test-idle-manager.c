/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-idle-dbus.h"

#define BUS_NAME "org.gnome.Mutter.IdleMonitor"
#define PATH "/org/gnome/Mutter/IdleMonitor"
#define OBJECT_PATH "/org/gnome/Mutter/IdleMonitor/Core"

static guint watch_id;
static GMainLoop *loop;
gint fire = 1000; /* fire after 1 second */

static void
watch_fired_cb (PhoshIdleDbusOrgGnomeMutterIdleMonitor *proxy,
                guint                id,
                gpointer            *data)
{
  g_assert (loop);
  g_assert_cmpint (watch_id, ==, id);
  g_main_loop_quit (loop);
}


static gboolean
timeout_cb  (gpointer data)
{
  g_error ("Watch did not fire");
  return FALSE;
}


static void
test_phosh_idle_add_watch(void)
{
  gint timeout_id;
  GError *err = NULL;
  PhoshIdleDbusOrgGnomeMutterIdleMonitor *proxy;
  PhoshIdleDbusObjectManagerClient *client;
  GDBusObject *object;

  if (!g_test_slow()) {
    g_test_skip ("Skipping thorough test");
    return;
  }

  client = PHOSH_IDLE_DBUS_OBJECT_MANAGER_CLIENT (
    phosh_idle_dbus_object_manager_client_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
      BUS_NAME,
      PATH,
      NULL,
      &err));
  g_assert (client);

  object = g_dbus_object_manager_get_object (G_DBUS_OBJECT_MANAGER (client),
                                             OBJECT_PATH);
  g_assert (object);

  proxy = phosh_idle_dbus_object_get_org_gnome_mutter_idle_monitor (PHOSH_IDLE_DBUS_OBJECT (object));
  g_assert (proxy);

  g_signal_connect_object (proxy, "watch-fired", G_CALLBACK (watch_fired_cb), NULL, 0);
  g_assert (phosh_idle_dbus_org_gnome_mutter_idle_monitor_call_add_idle_watch_sync (
              proxy, fire, &watch_id, NULL, NULL));
  g_assert (watch_id);

  timeout_id = g_timeout_add_seconds (fire * 2 / 1000, timeout_cb, NULL);
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  /* Remove watch that fired */
  g_source_remove (timeout_id);
  g_assert (phosh_idle_dbus_org_gnome_mutter_idle_monitor_call_remove_watch_sync (
              proxy, watch_id, NULL, NULL));
}


static void
test_phosh_idle_remove_watch(void)
{
  GError *err = NULL;
  PhoshIdleDbusOrgGnomeMutterIdleMonitor *proxy;
  PhoshIdleDbusObjectManagerClient *client;
  GDBusObject *object;

  if (!g_test_slow()) {
    g_test_skip ("Skipping thorough test");
    return;
  }

  client = PHOSH_IDLE_DBUS_OBJECT_MANAGER_CLIENT (
    phosh_idle_dbus_object_manager_client_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
      BUS_NAME,
      PATH,
      NULL,
      &err));
  g_assert (client);

  object = g_dbus_object_manager_get_object (G_DBUS_OBJECT_MANAGER (client),
                                             OBJECT_PATH);
  g_assert (object);

  proxy = phosh_idle_dbus_object_get_org_gnome_mutter_idle_monitor (PHOSH_IDLE_DBUS_OBJECT (object));
  g_assert (proxy);

  g_assert (phosh_idle_dbus_org_gnome_mutter_idle_monitor_call_add_idle_watch_sync (
              proxy, fire, &watch_id, NULL, NULL));
  g_assert (watch_id);
  /* Remove watch that did not yet fire */
  g_assert (phosh_idle_dbus_org_gnome_mutter_idle_monitor_call_remove_watch_sync (
              proxy, watch_id, NULL, NULL));
}


gint
main (gint argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/idle/idle", test_phosh_idle_add_watch);
  g_test_add_func("/phosh/idle/remove", test_phosh_idle_remove_watch);
  return g_test_run();
}
