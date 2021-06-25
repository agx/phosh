/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-display-dbus.h"
#include "log.h"
#include "shell.h"

#include "testlib.h"

#include <handy.h>

#define BUS_NAME "org.gnome.Mutter.DisplayConfig"
#define OBJECT_PATH "/org/gnome/Mutter/DisplayConfig"

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

  phosh_log_set_log_domains ("phosh-monitor-manager");

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
test_phosh_monitor_manager_current_state (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusDisplayConfig) proxy = NULL;
  g_autoptr (GVariant) monitors = NULL;
  g_autoptr (GVariant) logical = NULL;
  g_autoptr (GVariant) props = NULL;
  GVariantIter iter;
  guint serial;
  gboolean success;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_display_config_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                            BUS_NAME,
                                                            OBJECT_PATH,
                                                            NULL,
                                                            &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_DISPLAY_CONFIG_PROXY (proxy));

  success = phosh_dbus_display_config_call_get_current_state_sync (proxy,
                                                                   &serial,
                                                                   &monitors,
                                                                   &logical,
                                                                   &props,
                                                                   NULL,
                                                                   &err);
  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpint (serial, ==, 2);

  g_variant_iter_init (&iter, monitors);
  g_assert_cmpint (g_variant_iter_n_children (&iter), ==, 1);

  g_variant_iter_init (&iter, logical);
  g_assert_cmpint (g_variant_iter_n_children (&iter), ==, 1);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/dbus/monitor-manager/current_state", Fixture, NULL,
              comp_and_shell_setup, test_phosh_monitor_manager_current_state, comp_and_shell_teardown);

  return g_test_run ();
}
