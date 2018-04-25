/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "favorites.h"

static void
test_phosh_favorites_new(void)
{
  GtkWidget *window = phosh_favorites_new ();
  g_assert (window);
  gtk_widget_destroy (window);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/favorites/new", test_phosh_favorites_new);
  return g_test_run();
}
