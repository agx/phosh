/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "lockshield.h"

static void
test_lockshield_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  PhoshMonitor *monitor = phosh_test_get_monitor (fixture->state);
  GtkWidget *panel = phosh_lockshield_new (phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                           monitor);

  g_assert_true (PHOSH_IS_LOCKSHIELD (panel));
  gtk_widget_show (panel);
  gtk_widget_destroy (panel);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/lockshield/new", test_lockshield_new);

  return g_test_run ();
}
