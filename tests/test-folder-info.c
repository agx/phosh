/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#include "folder-info.h"

#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>


static void
test_phosh_folder_info_new (void)
{
  g_autoptr (PhoshFolderInfo) folder_info;
  g_autofree char *path = NULL;

  folder_info = phosh_folder_info_new_from_folder_path ("foo");
  g_object_get (folder_info, "path", &path, NULL);
  g_assert_cmpstr (path, ==, "foo");
}


static void
test_phosh_folder_info_get_name (void)
{
  g_autoptr (GSettings) settings;
  g_autoptr (PhoshFolderInfo) folder_info;

  settings = g_settings_new_with_path ("org.gnome.desktop.app-folders.folder",
                                       "/org/gnome/desktop/app-folders/folders/foo/");
  folder_info = phosh_folder_info_new_from_folder_path ("foo");

  g_settings_set_string (settings, "name", "X-Phosh-foo.directory");
  g_assert_cmpstr (phosh_folder_info_get_name (folder_info), ==, "Phosh Test Folder");

  g_settings_set_string (settings, "name", "doesnotexist.directory");
  g_assert_cmpstr (phosh_folder_info_get_name (folder_info), ==, "doesnotexist.directory");

  g_settings_set_string (settings, "name", "broken.directory");
  g_assert_cmpstr (phosh_folder_info_get_name (folder_info), ==, "broken.directory");

  g_settings_set_string (settings, "name", "Bar");
  g_assert_cmpstr (phosh_folder_info_get_name (folder_info), ==, "Bar");
}


static void
test_phosh_folder_info_get_app_infos (void)
{
  g_autoptr (GSettings) settings;
  g_autoptr (PhoshFolderInfo) folder_info;
  const char *app_ids[] = {"demo.app.First.desktop", NULL};
  GListModel *apps;
  g_autoptr (GAppInfo) got, made;

  settings = g_settings_new_with_path ("org.gnome.desktop.app-folders.folder",
                                       "/org/gnome/desktop/app-folders/folders/foo/");
  g_settings_set_strv (settings, "apps", app_ids);
  folder_info = phosh_folder_info_new_from_folder_path ("foo");

  apps = phosh_folder_info_get_app_infos (folder_info);
  g_assert_cmpuint (g_list_model_get_n_items (apps), ==, 1);

  made = G_APP_INFO (g_desktop_app_info_new (app_ids[0]));
  got = g_list_model_get_item (apps, 0);
  g_assert_true (g_app_info_equal (made, got));
}


static void
test_phosh_folder_info_contains (void)
{
  g_autoptr (GSettings) settings;
  g_autoptr (PhoshFolderInfo) folder_info;
  const char *app_ids[] = {"demo.app.First.desktop", NULL};
  g_autoptr (GAppInfo) info_1, info_2;

  settings = g_settings_new_with_path ("org.gnome.desktop.app-folders.folder",
                                       "/org/gnome/desktop/app-folders/folders/foo/");
  g_settings_set_strv (settings, "apps", app_ids);
  folder_info = phosh_folder_info_new_from_folder_path ("foo");

  info_1 = G_APP_INFO (g_desktop_app_info_new ("demo.app.First.desktop"));
  g_assert_true (phosh_folder_info_contains (folder_info, info_1));

  info_2 = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_false (phosh_folder_info_contains (folder_info, info_2));
}


static void
test_phosh_folder_info_refilter (void)
{
  g_autoptr (GSettings) settings;
  g_autoptr (PhoshFolderInfo) folder_info;
  const char *app_ids[] = {"demo.app.First.desktop", NULL};
  GListModel *apps;

  settings = g_settings_new_with_path ("org.gnome.desktop.app-folders.folder",
                                       "/org/gnome/desktop/app-folders/folders/foo/");
  g_settings_set_strv (settings, "apps", app_ids);
  g_object_unref (settings);

  settings = g_settings_new ("sm.puri.phosh");
  folder_info = phosh_folder_info_new_from_folder_path ("foo");
  apps = phosh_folder_info_get_app_infos (folder_info);

  /* Clear all favorites */
  g_settings_set_strv (settings, "favorites", NULL);
  /* Its apps are not favorite, hence non-empty, so should be true */
  g_assert_true (phosh_folder_info_refilter (folder_info, NULL));
  /* No favorite set, so there should be 1 element */
  g_assert_cmpuint (g_list_model_get_n_items (apps), ==, 1);

  /* Set an app as favorite */
  g_settings_set_strv (settings, "favorites", app_ids);
  /* Its apps are favorite, hence empty, so should be false */
  g_assert_false (phosh_folder_info_refilter (folder_info, NULL));
  /* A favorite set, so there should be 0 elements */
  g_assert_cmpuint (g_list_model_get_n_items (apps), ==, 0);

  /* Search is done for both favorites and non-favorites */
  /* Search for existing app "terminal" (aka `demo.app.First`) */
  g_assert_true (phosh_folder_info_refilter (folder_info, "terminal"));
  g_assert_cmpuint (g_list_model_get_n_items (apps), ==, 1);
  /* Search for "doesnotexist", which doesn't exist, hence empty, so should be false */
  g_assert_false (phosh_folder_info_refilter (folder_info, "doesnotexist"));
  g_assert_cmpuint (g_list_model_get_n_items (apps), ==, 0);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/folder-info/new", test_phosh_folder_info_new);
  g_test_add_func ("/phosh/folder-info/get_name", test_phosh_folder_info_get_name);
  g_test_add_func ("/phosh/folder-info/get_app_infos", test_phosh_folder_info_get_app_infos);
  g_test_add_func ("/phosh/folder-info/contains", test_phosh_folder_info_contains);
  g_test_add_func ("/phosh/folder-info/refilter", test_phosh_folder_info_refilter);

  return g_test_run ();
}
