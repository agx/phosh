/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "lockshield.h"

typedef struct _Fixture {
  PhoshTestCompositorState *state;
} Fixture;


static void
compositor_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->state = phosh_test_compositor_new (TRUE);
  g_assert_nonnull (fixture->state);
}

static void
compositor_teardown (Fixture *fixture, gconstpointer unused)
{
  phosh_test_compositor_free (fixture->state);
}

static void
test_lockshield_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *panel = phosh_lockshield_new (phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                           fixture->state->output);

  g_assert_true (PHOSH_IS_LOCKSHIELD (panel));
  gtk_widget_show (panel);
  gtk_widget_destroy (panel);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/lockshield/new", Fixture, NULL,
              compositor_setup, test_lockshield_new, compositor_teardown);

  return g_test_run ();
}
