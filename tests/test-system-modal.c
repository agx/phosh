/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "system-modal.h"

#include <glib.h>


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
test_system_modal_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *modal = phosh_system_modal_new (phosh_test_get_monitor ());

  g_assert_true (PHOSH_IS_SYSTEM_MODAL (modal));
  g_assert_true (gtk_style_context_has_class (
                   gtk_widget_get_style_context (modal),
                   "phosh-system-modal"));

  gtk_widget_destroy (modal);
}


static void
test_system_modal_g_object_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *modal = g_object_new (PHOSH_TYPE_SYSTEM_MODAL,
                                   "monitor", phosh_test_get_monitor (),
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

  g_test_add ("/phosh/system-modal/new", Fixture, NULL,
              compositor_setup, test_system_modal_new, compositor_teardown);
  g_test_add ("/phosh/system-modal/g_object_new", Fixture, NULL,
              compositor_setup, test_system_modal_g_object_new, compositor_teardown);

  return g_test_run ();
}
