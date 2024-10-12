/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arun.mani@tether.to>
 */

#include "quick-settings-box.h"
#include "quick-setting.h"


static void
test_phosh_quick_settings_box_new (void)
{
  GtkWidget *box;
  int max_columns;
  int spacing;
  gboolean can_show_status;
  g_autoptr (GList) children = NULL;

  box = g_object_new (PHOSH_TYPE_QUICK_SETTINGS_BOX, NULL);
  g_object_ref_sink (box);
  g_assert_true (PHOSH_IS_QUICK_SETTINGS_BOX (box));

  children = gtk_container_get_children (GTK_CONTAINER (box));
  g_assert_cmpuint (g_list_length (children), ==, 0);

  max_columns = phosh_quick_settings_box_get_max_columns (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_cmpuint (max_columns, ==, 3);

  spacing = phosh_quick_settings_box_get_spacing (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_cmpuint (spacing, ==, 0);

  can_show_status = phosh_quick_settings_box_get_can_show_status (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_true (can_show_status);

  g_assert_finalize_object (box);

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);
  g_assert_true (PHOSH_IS_QUICK_SETTINGS_BOX (box));
  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_add (void)
{
  GtkWidget *box;
  GtkWidget *child;
  g_autoptr (GList) children = NULL;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);
  child = phosh_quick_setting_new (NULL);

  gtk_container_add (GTK_CONTAINER (box), child);
  children = gtk_container_get_children (GTK_CONTAINER (box));
  g_assert_cmpuint (g_list_length (children), ==, 1);
  g_assert_true (g_list_nth_data (children, 0) == child);

  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_remove (void)
{
  GtkWidget *box;
  GtkWidget *child;
  g_autoptr (GList) children = NULL;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);
  child = phosh_quick_setting_new (NULL);

  gtk_container_add (GTK_CONTAINER (box), child);
  gtk_container_remove (GTK_CONTAINER (box), child);
  children = gtk_container_get_children (GTK_CONTAINER (box));
  g_assert_cmpuint (g_list_length (children), ==, 0);

  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_get_max_columns (void)
{
  GtkWidget *box;
  guint max_columns;
  guint got_max_columns;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);

  max_columns = 6;
  phosh_quick_settings_box_set_max_columns (PHOSH_QUICK_SETTINGS_BOX (box), max_columns);
  got_max_columns = phosh_quick_settings_box_get_max_columns (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_cmpuint (got_max_columns, ==, max_columns);

  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_get_spacing (void)
{
  GtkWidget *box;
  guint spacing;
  guint got_spacing;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);

  spacing = 6;
  phosh_quick_settings_box_set_spacing (PHOSH_QUICK_SETTINGS_BOX (box), spacing);
  got_spacing = phosh_quick_settings_box_get_spacing (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_cmpuint (got_spacing, ==, spacing);

  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_set_can_show_status (void)
{
  GtkWidget *box;
  GtkWidget *child_a;
  GtkWidget *child_b;
  gboolean can_show_status;
  gboolean got_can_show_status;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);
  child_a = g_object_new (PHOSH_TYPE_QUICK_SETTING, "can-show-status", TRUE, NULL);
  gtk_container_add (GTK_CONTAINER (box), child_a);
  child_b = g_object_new (PHOSH_TYPE_QUICK_SETTING, "can-show-status", FALSE, NULL);
  gtk_container_add (GTK_CONTAINER (box), child_b);

  can_show_status = TRUE;
  phosh_quick_settings_box_set_can_show_status (PHOSH_QUICK_SETTINGS_BOX (box), can_show_status);
  got_can_show_status = phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (child_a));
  g_assert_true (got_can_show_status == can_show_status);
  got_can_show_status = phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (child_b));
  g_assert_true (got_can_show_status == can_show_status);

  can_show_status = FALSE;
  phosh_quick_settings_box_set_can_show_status (PHOSH_QUICK_SETTINGS_BOX (box), can_show_status);
  got_can_show_status = phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (child_a));
  g_assert_true (got_can_show_status == can_show_status);
  got_can_show_status = phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (child_b));
  g_assert_true (got_can_show_status == can_show_status);

  g_assert_finalize_object (box);
}


static void
test_phosh_quick_settings_box_get_can_show_status (void)
{
  GtkWidget *box;
  gboolean can_show_status;
  gboolean got_can_show_status;

  box = phosh_quick_settings_box_new (3, 0);
  g_object_ref_sink (box);

  can_show_status = FALSE;
  phosh_quick_settings_box_set_can_show_status (PHOSH_QUICK_SETTINGS_BOX (box), can_show_status);
  got_can_show_status =
    phosh_quick_settings_box_get_can_show_status (PHOSH_QUICK_SETTINGS_BOX (box));
  g_assert_true (got_can_show_status == can_show_status);

  g_assert_finalize_object (box);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/quick-settings-box/new",
                   test_phosh_quick_settings_box_new);
  g_test_add_func ("/phosh/quick-settings-box/add",
                   test_phosh_quick_settings_box_add);
  g_test_add_func ("/phosh/quick-settings-box/remove",
                   test_phosh_quick_settings_box_remove);
  g_test_add_func ("/phosh/quick-settings-box/get_max_columns",
                   test_phosh_quick_settings_box_get_max_columns);
  g_test_add_func ("/phosh/quick-settings-box/get_spacing",
                   test_phosh_quick_settings_box_get_spacing);
  g_test_add_func ("/phosh/quick-settings-box/set_can_show_status",
                   test_phosh_quick_settings_box_set_can_show_status);
  g_test_add_func ("/phosh/quick-settings-box/get_can_show_status",
                   test_phosh_quick_settings_box_get_can_show_status);

  return g_test_run ();
}
