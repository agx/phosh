/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "system-modal-dialog.h"

#include <glib.h>


static void
test_system_modal_dialog_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *dialog;

  dialog = g_object_new (PHOSH_TYPE_SYSTEM_MODAL_DIALOG,
                         "monitor", phosh_test_get_monitor (fixture->state),
                         "title", "Testtitle",
                         NULL);

  g_assert_true (PHOSH_IS_SYSTEM_MODAL_DIALOG (dialog));

  phosh_system_modal_dialog_set_content (PHOSH_SYSTEM_MODAL_DIALOG (dialog),
                                         gtk_grid_new ());

  phosh_system_modal_dialog_add_button (PHOSH_SYSTEM_MODAL_DIALOG (dialog),
                                        GTK_WIDGET (gtk_button_new_with_label ("Ok")), -1);  

  phosh_system_modal_dialog_add_button (PHOSH_SYSTEM_MODAL_DIALOG (dialog),
                                        GTK_WIDGET (gtk_button_new_with_label ("Cancel")), -1);  
  
  gtk_widget_show (dialog);
  /* Run the unmapped code path */
  g_assert_true (gtk_widget_get_visible (dialog));
  g_assert_true (gtk_widget_get_mapped (dialog));
  gtk_widget_hide (dialog);
  g_assert_false (gtk_widget_get_visible (dialog));
  g_assert_false (gtk_widget_get_mapped (dialog));
  gtk_widget_destroy (dialog);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/system-modal-dialog/new", test_system_modal_dialog_new);

  return g_test_run ();
}
