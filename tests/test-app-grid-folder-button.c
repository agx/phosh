/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#include "app-grid-folder-button.h"
#include "folder-info.h"


static PhoshFolderInfo *
create_dummy_folder_info (void)
{
  g_autoptr (GSettings) settings;
  PhoshFolderInfo *folder_info;

  settings = g_settings_new_with_path ("org.gnome.desktop.app-folders.folder",
                                       "/org/gnome/desktop/app-folders/folders/foo/");
  g_settings_set_string (settings, "name", "Foo");

  folder_info = phosh_folder_info_new_from_folder_path ("foo");

  return folder_info;
}


static void
test_phosh_app_grid_folder_button_new (void)
{
  g_autoptr (PhoshFolderInfo) info, got_info;
  g_autofree char *label;
  GtkWidget *button;

  info = create_dummy_folder_info ();
  button = phosh_app_grid_folder_button_new_from_folder_info (info);
  g_assert_true (PHOSH_IS_APP_GRID_FOLDER_BUTTON (button));

  g_object_get (button, "folder-info", &got_info, NULL);
  g_assert_true (info == got_info);

  g_object_get (button, "label", &label, NULL);
  g_assert_cmpstr (label, ==, "Foo");

  gtk_widget_destroy (button);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/app-grid-folder-button/new", test_phosh_app_grid_folder_button_new);

  return g_test_run ();
}
