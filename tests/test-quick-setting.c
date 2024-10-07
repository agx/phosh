/*
 * Copyright (C) 2020 Purism SPC
 *               2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Guido Günther <agx@sigxcpu.org>
 *          Arun Mani J <arun.mani@tether.to>
 */

#include "quick-setting.h"
#include "status-icon.h"


static void
test_phosh_quick_setting_new (void)
{
  GtkWidget *quick_setting;
  gboolean active;
  gboolean show_status;
  gboolean can_show_status;
  PhoshStatusPage *status_page;

  quick_setting = g_object_new (PHOSH_TYPE_QUICK_SETTING, NULL);
  g_assert_true (PHOSH_IS_QUICK_SETTING (quick_setting));

  active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_false (active);

  show_status = phosh_quick_setting_get_showing_status (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_false (show_status);

  can_show_status = phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_false (can_show_status);

  status_page = phosh_quick_setting_get_status_page (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_true (status_page == NULL);

  gtk_widget_destroy (quick_setting);

  status_page = phosh_status_page_new ();
  quick_setting = phosh_quick_setting_new (status_page);
  g_assert_true (PHOSH_IS_QUICK_SETTING (quick_setting));
  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_add_status_icon (void)
{
  GtkWidget *quick_setting;
  GtkWidget *status_icon;
  GtkWidget *button_box_wid;
  g_autoptr (GList) box_children = NULL, button_box_children = NULL;
  GtkWidget *icon_wid;
  GtkWidget *label_wid;
  const char *label;
  const char *got_label;

  quick_setting = phosh_quick_setting_new (NULL);

  label = "Foo";
  status_icon = g_object_new (PHOSH_TYPE_STATUS_ICON, "icon-name", "face-smile-symbolic", "info",
                              label, NULL);
  gtk_container_add (GTK_CONTAINER (quick_setting), status_icon);

  box_children = gtk_container_get_children (GTK_CONTAINER (quick_setting));
  button_box_wid = gtk_bin_get_child (g_list_nth_data (box_children, 0));
  button_box_children = gtk_container_get_children (GTK_CONTAINER (button_box_wid));
  icon_wid = g_list_nth_data (button_box_children, 0);
  label_wid = g_list_nth_data (button_box_children, 1);

  g_assert_true (icon_wid == status_icon);

  got_label = gtk_label_get_text (GTK_LABEL (label_wid));
  g_assert_cmpstr (label, ==, got_label);

  gtk_widget_destroy (GTK_WIDGET (quick_setting));
}


static void
test_phosh_quick_setting_remove_status_icon (void)
{
  GtkWidget *quick_setting;
  GtkWidget *status_icon;
  GtkWidget *button_box_wid;
  g_autoptr (GList) box_children = NULL, button_box_children = NULL;

  quick_setting = phosh_quick_setting_new (NULL);

  status_icon = phosh_status_icon_new ();
  gtk_container_add (GTK_CONTAINER (quick_setting), status_icon);
  gtk_container_remove (GTK_CONTAINER (quick_setting), status_icon);

  box_children = gtk_container_get_children (GTK_CONTAINER (quick_setting));
  button_box_wid = gtk_bin_get_child (g_list_nth_data (box_children, 0));
  button_box_children = gtk_container_get_children (GTK_CONTAINER (button_box_wid));

  g_assert_cmpuint (g_list_length (button_box_children), ==, 1);
  g_assert_false (PHOSH_IS_STATUS_ICON (g_list_nth_data (button_box_children, 0)));

  gtk_widget_destroy (GTK_WIDGET (quick_setting));
}


static void
test_phosh_quick_setting_set_active (void)
{
  GtkWidget *quick_setting;
  GtkStateFlags flags;

  quick_setting = phosh_quick_setting_new (NULL);

  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (quick_setting), TRUE);
  flags = gtk_widget_get_state_flags (quick_setting);
  g_assert_true (flags & GTK_STATE_FLAG_ACTIVE);

  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (quick_setting), FALSE);
  flags = gtk_widget_get_state_flags (quick_setting);
  g_assert_true (flags & ~GTK_STATE_FLAG_ACTIVE);

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_get_active (void)
{
  GtkWidget *quick_setting;
  gboolean active;
  gboolean got_active;

  quick_setting = phosh_quick_setting_new (NULL);

  active = TRUE;
  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (quick_setting), active);
  got_active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_true (got_active == active);

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_set_showing_status (void)
{
  GtkWidget *quick_setting;
  g_autoptr (GList) box_children = NULL;
  GtkWidget *arrow;
  const char *icon_name;

  quick_setting = phosh_quick_setting_new (NULL);

  box_children = gtk_container_get_children (GTK_CONTAINER (quick_setting));
  arrow = gtk_bin_get_child (g_list_nth_data (box_children, 2));

  phosh_quick_setting_set_showing_status (PHOSH_QUICK_SETTING (quick_setting), TRUE);
  gtk_image_get_icon_name (GTK_IMAGE (arrow), &icon_name, NULL);
  g_assert_cmpstr ("go-down-symbolic", ==, icon_name);

  phosh_quick_setting_set_showing_status (PHOSH_QUICK_SETTING (quick_setting), FALSE);
  gtk_image_get_icon_name (GTK_IMAGE (arrow), &icon_name, NULL);
  g_assert_cmpstr ("go-next-symbolic", ==, icon_name);

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_get_showing_status (void)
{
  GtkWidget *quick_setting;
  gboolean showing_status;
  gboolean got_showing_status;

  quick_setting = phosh_quick_setting_new (NULL);

  showing_status = TRUE;
  phosh_quick_setting_set_showing_status (PHOSH_QUICK_SETTING (quick_setting), showing_status);
  got_showing_status = phosh_quick_setting_get_showing_status (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_true (got_showing_status == showing_status);

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_set_can_show_status (void)
{
  GtkWidget *quick_setting;
  g_autoptr (GList) box_children = NULL;
  GtkWidget *arrow_btn;
  gboolean can_show_status;

  quick_setting = phosh_quick_setting_new (phosh_status_page_new ());

  box_children = gtk_container_get_children (GTK_CONTAINER (quick_setting));
  arrow_btn = g_list_nth_data (box_children, 2);

  can_show_status = TRUE;
  phosh_quick_setting_set_can_show_status (PHOSH_QUICK_SETTING (quick_setting), can_show_status);
  g_assert_true (gtk_widget_get_visible (arrow_btn));

  can_show_status = FALSE;
  phosh_quick_setting_set_can_show_status (PHOSH_QUICK_SETTING (quick_setting), can_show_status);
  g_assert_false (gtk_widget_get_visible (arrow_btn));

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_get_can_show_status (void)
{
  GtkWidget *quick_setting;
  gboolean can_show_status;
  gboolean got_can_show_status;

  quick_setting = phosh_quick_setting_new (NULL);

  can_show_status = TRUE;
  phosh_quick_setting_set_can_show_status (PHOSH_QUICK_SETTING (quick_setting), can_show_status);
  got_can_show_status =
    phosh_quick_setting_get_can_show_status (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_true (got_can_show_status == can_show_status);

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_set_status_page (void)
{
  GtkWidget *quick_setting;
  PhoshStatusPage *status_page;
  g_autoptr (GList) box_children = NULL;
  GtkWidget *arrow_btn;

  quick_setting = phosh_quick_setting_new (NULL);
  phosh_quick_setting_set_can_show_status (PHOSH_QUICK_SETTING (quick_setting), TRUE);

  box_children = gtk_container_get_children (GTK_CONTAINER (quick_setting));
  arrow_btn = g_list_nth_data (box_children, 2);

  g_assert_false (gtk_widget_get_visible (arrow_btn));

  status_page = phosh_status_page_new ();
  phosh_quick_setting_set_status_page (PHOSH_QUICK_SETTING (quick_setting), status_page);
  g_assert_true (gtk_widget_get_visible (arrow_btn));

  gtk_widget_destroy (quick_setting);
}


static void
test_phosh_quick_setting_get_status_page (void)
{
  GtkWidget *quick_setting;
  PhoshStatusPage *status_page;
  PhoshStatusPage *got_status_page;

  quick_setting = phosh_quick_setting_new (NULL);

  status_page = phosh_status_page_new ();
  phosh_quick_setting_set_status_page (PHOSH_QUICK_SETTING (quick_setting), status_page);
  got_status_page = phosh_quick_setting_get_status_page (PHOSH_QUICK_SETTING (quick_setting));
  g_assert_true (got_status_page == status_page);

  gtk_widget_destroy (quick_setting);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/quick-setting/new",
                   test_phosh_quick_setting_new);
  g_test_add_func ("/phosh/quick-setting/add_status_icon",
                   test_phosh_quick_setting_add_status_icon);
  g_test_add_func ("/phosh/quick-setting/remove_status_icon",
                   test_phosh_quick_setting_remove_status_icon);
  g_test_add_func ("/phosh/quick-setting/set_active",
                   test_phosh_quick_setting_set_active);
  g_test_add_func ("/phosh/quick-setting/get_active",
                   test_phosh_quick_setting_get_active);
  g_test_add_func ("/phosh/quick-setting/set_can_show_status",
                   test_phosh_quick_setting_set_can_show_status);
  g_test_add_func ("/phosh/quick-setting/get_can_show_status",
                   test_phosh_quick_setting_get_can_show_status);
  g_test_add_func ("/phosh/quick-setting/set_showing_status",
                   test_phosh_quick_setting_set_showing_status);
  g_test_add_func ("/phosh/quick-setting/get_showing_status",
                   test_phosh_quick_setting_get_showing_status);
  g_test_add_func ("/phosh/quick-setting/set_status_page",
                   test_phosh_quick_setting_set_status_page);
  g_test_add_func ("/phosh/quick-setting/get_status_page",
                   test_phosh_quick_setting_get_status_page);

  return g_test_run ();
}
