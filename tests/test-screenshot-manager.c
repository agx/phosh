/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-screenshot-dbus.h"
#include "log.h"
#include "shell.h"

#include "testlib.h"

#include <handy.h>

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

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

  phosh_log_set_log_domains ("phosh-screenshot-manager");

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
test_phosh_screenshot_png (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusScreenshot) proxy = NULL;
  g_autofree char *used_name = NULL;
  g_autofree char *dirname = NULL;
  g_autofree char *filename = NULL;
  g_autofree char *path = NULL;
  gboolean success1, success2;

  dirname = g_build_filename (TEST_OUTPUT_DIR, __func__, NULL);
  filename = g_strdup_printf ("screenshot-%d.png", g_test_rand_int ());
  path = g_build_filename (dirname, filename, NULL);
  g_assert_cmpint (g_mkdir_with_parents (dirname, 0755), ==, 0);

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_screenshot_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                        BUS_NAME,
                                                        OBJECT_PATH,
                                                        NULL,
                                                        &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_SCREENSHOT_PROXY (proxy));

  success1 = phosh_dbus_screenshot_call_screenshot_sync (proxy,
                                                         FALSE,
                                                         TRUE,
                                                         path,
                                                         &success2,
                                                         &used_name,
                                                         NULL,
                                                         &err);

  g_assert_no_error (err);
  g_assert_true (success1);
  g_assert_true (success2);
  g_assert_cmpstr (used_name, ==, path);
  g_test_message ("Screenshot at %s", used_name);
  g_file_test (used_name, G_FILE_TEST_EXISTS);

  success1 = phosh_dbus_screenshot_call_screenshot_sync (proxy,
                                                         FALSE,
                                                         TRUE,
                                                         path,
                                                         &success2,
                                                         &used_name,
                                                         NULL,
                                                         &err);

  g_assert_no_error (err);
  g_assert_true (success1);
  /* 2nd screenshot with identical file name fails */
  g_assert_false (success2);

  g_assert_cmpint (unlink (path), ==, 0);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/dbus/screenshot-manager/png", Fixture, NULL,
              comp_and_shell_setup, test_phosh_screenshot_png, comp_and_shell_teardown);

  return g_test_run ();
}
