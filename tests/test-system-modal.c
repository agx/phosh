/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "system-modal.h"

#include <glib.h>


static void
test_system_modal_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *modal = phosh_system_modal_new (phosh_test_get_monitor (fixture->state));

  g_assert_true (PHOSH_IS_SYSTEM_MODAL (modal));
  g_assert_true (gtk_style_context_has_class (
                   gtk_widget_get_style_context (modal),
                   "phosh-system-modal"));

  gtk_widget_destroy (modal);
}


static void
test_system_modal_g_object_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *modal = g_object_new (PHOSH_TYPE_SYSTEM_MODAL,
                                   "monitor", phosh_test_get_monitor (fixture->state),
                                   NULL);

  g_assert_true (PHOSH_IS_SYSTEM_MODAL (modal));

  gtk_widget_show (modal);
  /* Run the unmapped code path */
  g_assert_true (gtk_widget_get_visible (modal));
  g_assert_true (gtk_widget_get_mapped (modal));
  gtk_widget_hide (modal);
  g_assert_false (gtk_widget_get_visible (modal));
  g_assert_false (gtk_widget_get_mapped (modal));
  gtk_widget_destroy (modal);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/system-modal/new", test_system_modal_new);
  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/system-modal/g_object_new", test_system_modal_g_object_new);

  return g_test_run ();
}
