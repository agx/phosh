/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "app.h"

static void
test_phosh_app_new(void)
{
  PhoshApp *app = PHOSH_APP (phosh_app_new ("com.example.foo", "bar"));
  g_assert (app);
  g_assert_cmpstr (phosh_app_get_app_id (app), ==, "com.example.foo");
  g_assert_cmpstr (phosh_app_get_title (app), ==, "bar");
  gtk_widget_destroy (GTK_WIDGET (app));
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/app/new", test_phosh_app_new);
  return g_test_run();
}
