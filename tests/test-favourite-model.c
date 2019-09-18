/*
 * Copyright (C) 2019 Zander Brown
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "favourite-list-model.h"

static void
test_phosh_favourite_list_model_get_default(void)
{
  PhoshFavouriteListModel *model1 = phosh_favourite_list_model_get_default ();
  PhoshFavouriteListModel *model2 = phosh_favourite_list_model_get_default ();

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model1));
  g_assert_true (model1 == model2);
  g_object_unref (model1);

  /* test we can do that twice in a row */
  model1 = phosh_favourite_list_model_get_default ();
  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model1));
  g_object_unref (model1);

}


static void
test_phosh_favourite_list_model_get_type (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (model)) == G_TYPE_APP_INFO);
}


static void
test_phosh_favourite_list_model_get_length (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();
  g_autoptr (GSettings) settings = NULL;
  const char *items_missing[2] = {"thing-that-wont-exist.desktop", NULL};
  const char *items[2] = {"demo.app.First.desktop", NULL};

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  g_settings_set_strv (settings, "favorites", items_missing);

  // Should have skipped the non existant item, thus still not items
  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  g_settings_set_strv (settings, "favorites", items);

  // Should have skipped the non existant item, thus still not items
  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);
}


static void
test_phosh_favourite_list_model_get_item (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();
  g_autoptr (GSettings) settings = NULL;
  const char *items[2] = {"demo.app.First.desktop", NULL};

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  g_assert_null (g_list_model_get_item (G_LIST_MODEL (model), 1));

  g_settings_set_strv (settings, "favorites", items);

  g_assert_nonnull (g_list_model_get_item (G_LIST_MODEL (model), 0));
}


static void
test_phosh_favourite_list_model_add (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();
  g_autoptr (GSettings) settings = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GAppInfo) info_desktop = NULL;

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  info = g_app_info_create_from_commandline ("foo",
                                             "com.example.foo",
                                             G_APP_INFO_CREATE_NONE,
                                             NULL);

  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  info_desktop = G_APP_INFO (g_desktop_app_info_new ("demo.app.First.desktop"));
  phosh_favourite_list_model_add_app (model, info_desktop);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);

  g_test_expect_message ("phosh-favourite-list-model",
                         G_LOG_LEVEL_WARNING,
                         "* is already a favourite");
  phosh_favourite_list_model_add_app (model, info_desktop);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);

  g_test_expect_message ("phosh-favourite-list-model",
                         G_LOG_LEVEL_CRITICAL,
                         "Can't add *, doesn't have an id");
  phosh_favourite_list_model_add_app (model, info);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);
}


static void
test_phosh_favourite_list_model_remove (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();
  g_autoptr (GSettings) settings = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GAppInfo) info_desktop = NULL;

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  info = g_app_info_create_from_commandline ("foo",
                                             "com.example.foo",
                                             G_APP_INFO_CREATE_NONE,
                                             NULL);

  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  info_desktop = G_APP_INFO (g_desktop_app_info_new ("demo.app.First.desktop"));

  // Add an app so we can remove it
  phosh_favourite_list_model_add_app (model, info_desktop);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);

  phosh_favourite_list_model_remove_app (model, info_desktop);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  g_test_expect_message ("phosh-favourite-list-model",
                         G_LOG_LEVEL_WARNING,
                         "* wasn't a favourite");
  phosh_favourite_list_model_remove_app (model, info_desktop);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  g_test_expect_message ("phosh-favourite-list-model",
                         G_LOG_LEVEL_CRITICAL,
                         "Can't remove *, doesn't have an id");
  phosh_favourite_list_model_remove_app (model, info);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);
}


static void
test_phosh_favourite_list_model_is_favourite (void)
{
  PhoshFavouriteListModel *model = phosh_favourite_list_model_get_default ();
  g_autoptr (GSettings) settings = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GAppInfo) info_desktop = NULL;

  g_assert_true (PHOSH_IS_FAVOURITE_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));

  info = g_app_info_create_from_commandline ("foo",
                                             "com.example.foo",
                                             G_APP_INFO_CREATE_NONE,
                                             NULL);

  settings = g_settings_new ("sm.puri.phosh");
  g_settings_set_strv (settings, "favorites", NULL);

  info_desktop = G_APP_INFO (g_desktop_app_info_new ("demo.app.First.desktop"));

  // Add an app so we can remove it
  phosh_favourite_list_model_add_app (model, info_desktop);

  g_assert_true (phosh_favourite_list_model_app_is_favourite (model, info_desktop));

  phosh_favourite_list_model_remove_app (model, info_desktop);

  g_assert_false (phosh_favourite_list_model_app_is_favourite (model, info_desktop));

  g_assert_false (phosh_favourite_list_model_app_is_favourite (model, info));
}


gint
main (gint argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/favourites-list-model/new", test_phosh_favourite_list_model_get_default);
  g_test_add_func ("/phosh/favourites-list-model/type", test_phosh_favourite_list_model_get_type);
  g_test_add_func ("/phosh/favourites-list-model/length", test_phosh_favourite_list_model_get_length);
  g_test_add_func ("/phosh/favourites-list-model/get", test_phosh_favourite_list_model_get_item);
  g_test_add_func ("/phosh/favourites-list-model/add", test_phosh_favourite_list_model_add);
  g_test_add_func ("/phosh/favourites-list-model/remove", test_phosh_favourite_list_model_remove);
  g_test_add_func ("/phosh/favourites-list-model/is_favourite", test_phosh_favourite_list_model_is_favourite);

  return g_test_run ();
}
