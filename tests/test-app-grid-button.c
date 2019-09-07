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
  PhoshAppGridButtonMode mode;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  is_favorite = phosh_app_grid_button_is_favourite (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (is_favorite);

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_FAVOURITES);

  g_assert_true (info == phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));
  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_new_favorite(void)
{
  gboolean is_favorite;
  PhoshAppGridButtonMode mode;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new_favorite (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  is_favorite = phosh_app_grid_button_is_favourite (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (is_favorite);

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_FAVOURITES);

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_set_app_info(void)
{
  GAppInfo *info1 = g_app_info_create_from_commandline ("foo",
                                                        "com.example.foo",
                                                        G_APP_INFO_CREATE_NONE,
                                                        NULL);
  GAppInfo *info2 = g_app_info_create_from_commandline ("bar",
                                                        "com.example.bar",
                                                        G_APP_INFO_CREATE_NONE,
                                                        NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (info1);
  g_assert_true (info1 == phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));
  phosh_app_grid_button_set_app_info (PHOSH_APP_GRID_BUTTON (btn), info2);
  g_assert_true (info2 == phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_null_app_info(void)
{
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (NULL);
  g_assert_null (phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));
  gtk_widget_destroy (btn);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/app-grid-button/new", test_phosh_app_grid_button_new);
  g_test_add_func("/phosh/app-grid-button/new_favorite", test_phosh_app_grid_button_new_favorite);
  g_test_add_func("/phosh/app-grid-button/set_app_info", test_phosh_app_grid_button_set_app_info);
  g_test_add_func("/phosh/app-grid-button/null_app_info", test_phosh_app_grid_button_null_app_info);
  return g_test_run();
}
