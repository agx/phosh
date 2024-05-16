/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "log.h"
#include "shell.h"
#include "background-manager.h"
#include "phosh-wayland.h"
#include "wall-clock.h"

#include <glib.h>
#include <handy.h>


static void
compositor_setup (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_setenv ("WLR_HEADLESS_OUTPUTS", "1", TRUE);

  phosh_test_compositor_setup (fixture, NULL);
}


static void
compositor_setup_dualhead (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_setenv ("WLR_HEADLESS_OUTPUTS", "2", TRUE);

  phosh_test_compositor_setup (fixture, NULL);
}


static void
compositor_teardown (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  phosh_test_compositor_teardown (fixture, NULL);
  g_unsetenv("WLR_HEADLESS_OUTPUTS");
}


static gboolean
on_idle (gpointer data)
{
  gtk_main_quit ();

  *(gboolean*)data = TRUE;

  return G_SOURCE_REMOVE;
}


/**
 * phosh_test_get_shell:
 * @saved_flags: Current logging flags
 *
 * Returns:(transfer full): A new shell object
 */
static PhoshShell *
phosh_test_get_shell (GLogLevelFlags *saved_flags)
{
  PhoshShell *shell;
  GLogLevelFlags flags;

  gtk_init (NULL, NULL);
  hdy_init ();

  phosh_log_set_log_domains ("all");

  /* Drop warnings from the fatal log mask since there's plenty
   * when running without recommended DBus services */
  flags = g_log_set_always_fatal (0);
  g_log_set_always_fatal(flags & ~G_LOG_LEVEL_WARNING);

  shell = phosh_shell_new ();
  g_assert_true (PHOSH_IS_SHELL (shell));

  g_assert_false (phosh_shell_get_locked (shell));
  g_assert_false (phosh_shell_is_startup_finished (shell));
  g_assert_true (PHOSH_IS_MONITOR (phosh_shell_get_primary_monitor (shell)));

  if (saved_flags)
    *saved_flags = flags;

  return shell;
}


/**
 * phosh_test_drain_events:
 *
 * Start a new default main loop and drain events. This can
 * be useful to detect lingering async handlers.
 */
static void
phosh_test_drain_events (void)
{
  GMainLoop *loop;

  /* Process all pending events to spot missing cleanups */
  loop = g_main_loop_new (NULL, FALSE);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  g_main_loop_unref (loop);
}


static void
on_shell_ready (PhoshShell *shell, gboolean *ready)
{
  *ready = TRUE;
}


static void
test_shell_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  PhoshShell *shell;
  PhoshMonitorManager *mm;
  GLogLevelFlags flags;
  gboolean success;
  gboolean ready = FALSE;

  shell = phosh_test_get_shell (&flags);
  phosh_shell_set_default (shell);

  g_signal_connect (shell, "ready", G_CALLBACK (on_shell_ready), &ready);

  mm = phosh_shell_get_monitor_manager (shell);
  g_assert_cmpint (phosh_monitor_manager_get_num_monitors (mm), ==, 1);

  g_idle_add (on_idle, &success);
  gtk_main ();

  /* No warnings allowed from here on */
  g_log_set_always_fatal (flags);

  g_assert_true (ready);
  g_assert_true (success);

  g_debug ("Finalizing shell");
  g_assert_finalize_object (shell);

  phosh_test_drain_events ();
}


static void
test_shell_new_two_outputs (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  PhoshShell *shell;
  PhoshMonitorManager *mm;
  PhoshBackgroundManager *bm;
  g_autoptr (GList) bgs = NULL;
  GLogLevelFlags flags;
  gboolean success;

  shell = phosh_test_get_shell (&flags);
  phosh_shell_set_default (shell);

  mm = phosh_shell_get_monitor_manager (shell);
  g_assert_cmpint (phosh_monitor_manager_get_num_monitors (mm), ==, 2);

  /* Process events to startup shell */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  g_idle_add (on_idle, &success);
  gtk_main ();

  g_log_set_always_fatal (flags);
  g_assert_true (success);

  bm = phosh_shell_get_background_manager (shell);
  bgs = phosh_background_manager_get_backgrounds (bm);
  g_assert_cmpint (g_list_length (bgs), ==, 2);

  g_assert_finalize_object (shell);

  phosh_test_drain_events ();
}


int
main (int argc, char *argv[])
{
  g_autoptr (PhoshWallClock) wall_clock = phosh_wall_clock_new ();

  g_test_init (&argc, &argv, NULL);
  phosh_wall_clock_set_default (wall_clock);

  g_test_add ("/phosh/shell/new", PhoshTestCompositorFixture, NULL,
              compositor_setup, test_shell_new, compositor_teardown);
  g_test_add ("/phosh/shell/dualhead", PhoshTestCompositorFixture, NULL,
              compositor_setup_dualhead, test_shell_new_two_outputs, compositor_teardown);

  return g_test_run ();
}
