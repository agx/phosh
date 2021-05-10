/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "log.h"
#include "shell.h"
#include "phosh-wayland.h"

#include <glib.h>
#include <handy.h>


typedef struct _Fixture {
  PhoshTestCompositorState *state;
} Fixture;


static void
compositor_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->state = phosh_test_compositor_new ();
  g_assert_nonnull (fixture->state);
}

static void
compositor_teardown (Fixture *fixture, gconstpointer unused)
{
  phosh_test_compositor_free (fixture->state);
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
 * Returns: A new shell object
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

  shell = phosh_shell_get_default ();
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
test_shell_new (Fixture *fixture, gconstpointer unused)
{
  PhoshShell *shell;
  PhoshMonitorManager *mm;
  GLogLevelFlags flags;
  gboolean success;

  shell = phosh_test_get_shell (&flags);

  mm = phosh_shell_get_monitor_manager (shell);
  g_assert_cmpint (phosh_monitor_manager_get_num_monitors (mm), ==, 1);

  g_idle_add (on_idle, &success);
  gtk_main ();

  /* No warnings allowed from here on */
  g_log_set_always_fatal (flags);
  g_assert_true (success);

  g_debug ("Finalizing shell");
  g_assert_finalize_object (shell);

  phosh_test_drain_events ();
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/shell/new", Fixture, NULL,
              compositor_setup, test_shell_new, compositor_teardown);

  return g_test_run ();
}
