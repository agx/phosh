/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "osd-window.h"

static void
test_osd_window_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *osd = g_object_new (PHOSH_TYPE_OSD_WINDOW,
                                 "monitor", phosh_test_get_monitor (fixture->state),
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

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/osd-window/new", test_osd_window_new);

  return g_test_run ();
}
