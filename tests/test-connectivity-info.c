/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "connectivity-info.h"

static void
test_phosh_connectivity_info_new (void)
{
  GtkWidget *widget;

  widget = phosh_connectivity_info_new ();
  g_assert_true (PHOSH_IS_CONNECTIVITY_INFO (widget));

  gtk_widget_destroy (widget);
}

int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/connectivity-info/new", test_phosh_connectivity_info_new);

  return g_test_run();
}
