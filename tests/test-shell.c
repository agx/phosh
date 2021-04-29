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


static void
test_shell_new (Fixture *fixture, gconstpointer unused)
{
  PhoshShell *shell = NULL;
  GLogLevelFlags flags;
  gboolean success;

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

  g_idle_add (on_idle, &success);
  gtk_main ();

  g_log_set_always_fatal (flags);
  g_assert_true (success);

  g_object_unref (shell);
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
