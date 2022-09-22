/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-idle-dbus.h"
#include "log.h"
#include "shell.h"

#include "testlib.h"

#include <handy.h>

#define BUS_NAME "org.gnome.Mutter.IdleMonitor"
#define PATH "/org/gnome/Mutter/IdleMonitor"
#define OBJECT_PATH "/org/gnome/Mutter/IdleMonitor/Core"
#define POP_TIMEOUT 50000000

static guint watch_id;
static GMainLoop *loop;
int fire = 1000; /* fire after 1 second */

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
  fixture->state = phosh_test_compositor_new (TRUE);

  gtk_init (NULL, NULL);
  hdy_init ();

  phosh_log_set_log_domains ("all");

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

  phosh_log_set_log_domains (NULL);
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
watch_fired_cb (PhoshIdleDBusIdleMonitor *proxy,
                guint                     id,
                gpointer                 *data)
{
  *(gboolean*)data = TRUE;
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
test_phosh_idle_watch_fired (Fixture *fixture, gconstpointer unused)
{
  int timeout_id;
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshIdleDBusIdleMonitor) proxy = NULL;
  g_autoptr (PhoshIdleDBusObjectManagerClient) client = NULL;
  g_autoptr (GDBusObject) object = NULL;
  gboolean fired = FALSE;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  client = PHOSH_IDLE_DBUS_OBJECT_MANAGER_CLIENT (
    phosh_idle_dbus_object_manager_client_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
      BUS_NAME,
      PATH,
      NULL,
      &err));
  g_assert_no_error (err);
  g_assert_true (PHOSH_IDLE_DBUS_IS_OBJECT_MANAGER_CLIENT (client));

  object = g_dbus_object_manager_get_object (G_DBUS_OBJECT_MANAGER (client),
                                             OBJECT_PATH);
  g_assert (PHOSH_IDLE_DBUS_IS_OBJECT (object));

  proxy = phosh_idle_dbus_object_get_idle_monitor (PHOSH_IDLE_DBUS_OBJECT (object));
  g_assert (G_IS_DBUS_PROXY (proxy));

  /* Connect the watch */
  g_signal_connect (proxy, "watch-fired", G_CALLBACK (watch_fired_cb), &fired);
  g_assert (phosh_idle_dbus_idle_monitor_call_add_idle_watch_sync (
              proxy, fire, &watch_id, NULL, NULL));
  g_assert (watch_id);

  /* Aborts in case watch doesn't fire */
  timeout_id = g_timeout_add_seconds (fire * 2 / 1000, timeout_cb, NULL);

  /* Run main loop */
  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  /* Remove watch that fired */
  g_assert_true (fired);
  g_source_remove (timeout_id);
  g_assert (phosh_idle_dbus_idle_monitor_call_remove_watch_sync (
              proxy, watch_id, NULL, NULL));

}


static void
test_phosh_idle_watch_unfired (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshIdleDBusIdleMonitor) proxy = NULL;
  g_autoptr (PhoshIdleDBusObjectManagerClient) client = NULL;
  g_autoptr (GDBusObject) object = NULL;
  gboolean fired = FALSE;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  client = PHOSH_IDLE_DBUS_OBJECT_MANAGER_CLIENT (
    phosh_idle_dbus_object_manager_client_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
      BUS_NAME,
      PATH,
      NULL,
      &err));
  g_assert_no_error (err);
  g_assert_true (PHOSH_IDLE_DBUS_IS_OBJECT_MANAGER_CLIENT (client));

  object = g_dbus_object_manager_get_object (G_DBUS_OBJECT_MANAGER (client),
                                             OBJECT_PATH);
  g_assert (PHOSH_IDLE_DBUS_IS_OBJECT (object));

  proxy = phosh_idle_dbus_object_get_idle_monitor (PHOSH_IDLE_DBUS_OBJECT (object));
  g_assert (G_IS_DBUS_PROXY (proxy));

  /* Connect the watch */
  g_assert (phosh_idle_dbus_idle_monitor_call_add_idle_watch_sync (
              proxy, fire, &watch_id, NULL, NULL));
  g_assert (watch_id);

  /* Remove watch that did not yet fire */
  g_assert (phosh_idle_dbus_idle_monitor_call_remove_watch_sync (
              proxy, watch_id, NULL, NULL));
  g_assert_false (fired);
}


int
main (int   argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/dbus/idle-manager/fired", Fixture, NULL,
              comp_and_shell_setup, test_phosh_idle_watch_fired, comp_and_shell_teardown);
  g_test_add ("/phosh/dbus/idle-manager/unfired", Fixture, NULL,
              comp_and_shell_setup, test_phosh_idle_watch_unfired, comp_and_shell_teardown);

  return g_test_run ();
}
