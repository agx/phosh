/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "osd-window.h"

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
test_osd_window_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *osd = g_object_new (PHOSH_TYPE_OSD_WINDOW,
                                 "monitor", phosh_test_get_monitor (),
                                 "connector", "a-connector",
                                 "label", "a-label",
                                 "icon-name", "a-icon",
                                 "level", 0.5,
                                 "max-level", 1.0,
                                 NULL);
  g_assert_true (PHOSH_IS_OSD_WINDOW (osd));

  /* Flip some properties so we'd catch leaks with ASAN */
  g_object_set (osd,
                "connector", "b-connector",
                "label", "b-label",
                "icon-name", "b-icon",
                NULL);

  gtk_widget_show (osd);
  gtk_widget_destroy (osd);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/osd-window/new", Fixture, NULL,
              compositor_setup, test_osd_window_new, compositor_teardown);

  return g_test_run ();
}
