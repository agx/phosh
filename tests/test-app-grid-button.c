/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "app-grid-button.h"
#include "favorite-list-model.h"

static void
test_phosh_app_grid_button_new (void)
{
  PhoshAppGridButtonMode mode;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GAppInfo *got_info;
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_LAUNCHER);

  g_assert_true (info == phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));

  g_object_get (btn, "app-info", &got_info, NULL);
  g_assert_true (info == got_info);

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_new_favorite (void)
{
  PhoshAppGridButtonMode mode;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new_favorite (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_FAVORITES);

  g_object_get (btn, "mode", &mode, NULL);
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_FAVORITES);

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_set_app_info (void)
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
  g_assert_true (gtk_widget_is_sensitive (btn));

  phosh_app_grid_button_set_app_info (PHOSH_APP_GRID_BUTTON (btn), NULL);
  g_assert_true (NULL == phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));
  g_assert_false (gtk_widget_is_sensitive (btn));

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_set_mode (void)
{
  PhoshAppGridButtonMode mode;
  GAppInfo *info = g_app_info_create_from_commandline ("foo",
                                                       "com.example.foo",
                                                       G_APP_INFO_CREATE_NONE,
                                                       NULL);
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  phosh_app_grid_button_set_mode (PHOSH_APP_GRID_BUTTON (btn),
                                  PHOSH_APP_GRID_BUTTON_FAVORITES);

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_FAVORITES);

  phosh_app_grid_button_set_mode (PHOSH_APP_GRID_BUTTON (btn),
                                  PHOSH_APP_GRID_BUTTON_LAUNCHER);

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_LAUNCHER);

  g_test_expect_message ("phosh-app-grid-button",
                         G_LOG_LEVEL_CRITICAL,
                         "Invalid mode*");
  phosh_app_grid_button_set_mode (PHOSH_APP_GRID_BUTTON (btn),
                                  G_MAXINT);
  // The mode shouldn't have actually changed
  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_LAUNCHER);

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_null_app_info (void)
{
  GtkWidget *btn;

  btn = phosh_app_grid_button_new (NULL);
  g_assert_null (phosh_app_grid_button_get_app_info (
                   PHOSH_APP_GRID_BUTTON (btn)));
  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_menu (void)
{
  PhoshFavoriteListModel *list;
  GtkWidget *btn;
  gboolean is_favorite;
  g_autoptr (GAppInfo) info = NULL;
  GActionGroup *actions;

  list = phosh_favorite_list_model_get_default ();

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));

  btn = phosh_app_grid_button_new (info);

  // Pretend someone pressed the menu button
  GTK_WIDGET_GET_CLASS (btn)->popup_menu (btn);
  // Ideally we would check the popover actually opened

  actions = gtk_widget_get_action_group (btn, "app-btn");
  g_action_group_activate_action (actions, "favorite-add", NULL);

  is_favorite = phosh_favorite_list_model_app_is_favorite (list, info);
  g_assert_true (is_favorite);

  g_action_group_activate_action (actions, "favorite-remove", NULL);

  is_favorite = phosh_favorite_list_model_app_is_favorite (list, info);
  g_assert_false (is_favorite);

  gtk_widget_destroy (btn);
}


static void
test_phosh_app_grid_button_is_favorite (void)
{
  PhoshFavoriteListModel *list;
  PhoshAppGridButtonMode mode;
  gboolean is_favorite;
  g_autoptr (GSettings) settings = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GtkWidget *btn;

  list = phosh_favorite_list_model_get_default ();

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));

  // Clear all favorites
  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  btn = phosh_app_grid_button_new (info);
  g_assert_true (PHOSH_IS_APP_GRID_BUTTON (btn));

  mode = phosh_app_grid_button_get_mode (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (mode == PHOSH_APP_GRID_BUTTON_LAUNCHER);

  is_favorite = phosh_app_grid_button_is_favorite (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_false (is_favorite);

  phosh_favorite_list_model_add_app (list, info);

  is_favorite = phosh_app_grid_button_is_favorite (PHOSH_APP_GRID_BUTTON (btn));
  g_assert_true (is_favorite);

  g_object_get (btn, "is-favorite", &is_favorite, NULL);
  g_assert_true (is_favorite);

  g_assert_true (info == phosh_app_grid_button_get_app_info (
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
  g_test_add_func("/phosh/app-grid-button/set_mode", test_phosh_app_grid_button_set_mode);
  g_test_add_func("/phosh/app-grid-button/null_app_info", test_phosh_app_grid_button_null_app_info);
  g_test_add_func("/phosh/app-grid-button/menu", test_phosh_app_grid_button_menu);
  g_test_add_func("/phosh/app-grid-button/is_favorite", test_phosh_app_grid_button_is_favorite);

  return g_test_run();
}
