/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "fading-label.h"

static void
test_phosh_fading_label_new (void)
{
  PhoshFadingLabel *fading_label;

  fading_label = PHOSH_FADING_LABEL (phosh_fading_label_new ("label"));

  gtk_widget_show (GTK_WIDGET (fading_label));
  g_assert_cmpstr (phosh_fading_label_get_label (fading_label), ==, "label");
  phosh_fading_label_set_label (fading_label, "label2");
  g_assert_cmpstr (phosh_fading_label_get_label (fading_label), ==, "label2");

  gtk_widget_destroy (GTK_WIDGET (fading_label));
}


int
main (int   argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/fading-label/new", test_phosh_fading_label_new);

  return g_test_run();
}
