/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "app-grid-button.h"

static void
test_phosh_app_grid_button_new(void)
{
  gboolean is_favorite;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (G_APP_INFO (info));
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON(btn));
  g_object_get (btn, "is-favorite", &is_favorite, NULL);
  g_assert_false (is_favorite);
  gtk_widget_destroy (btn);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/app-grid-button/new", test_phosh_app_grid_button_new);
  return g_test_run();
}
