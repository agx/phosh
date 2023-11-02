/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "quick-setting.h"
#include "status-icon.h"

#define TEST_STRING "test info"
static void
test_phosh_quick_setting_new (void)
{
  GtkWidget *widget;

  widget = phosh_quick_setting_new ();
  g_assert_true (PHOSH_IS_QUICK_SETTING (widget));

  gtk_widget_destroy (widget);
}


static void
check_each_child_cb (GtkWidget *widget,
                     gpointer  *data)
{
  if (GTK_IS_LABEL (widget)) {
    g_assert_true (g_strcmp0 (TEST_STRING, gtk_label_get_text (GTK_LABEL (widget))) == 0);
  } else if (GTK_IS_IMAGE (widget)) {
    const gchar *icon_name;
    gtk_image_get_icon_name (GTK_IMAGE (widget), &icon_name, NULL);
    g_assert_true (g_strcmp0 ("go-next-symbolic", icon_name) == 0);
  } else {
    g_assert_true (PHOSH_IS_STATUS_ICON (widget));
  }
}

static void
test_phosh_quick_setting_with_status_icon (void)
{
  GtkWidget *widget;
  GtkWidget *status_icon;
  GtkWidget *extra;

  widget = phosh_quick_setting_new ();
  g_assert_true (PHOSH_IS_QUICK_SETTING (widget));

  status_icon = phosh_status_icon_new ();
  g_assert_true (PHOSH_IS_STATUS_ICON (status_icon));

  gtk_container_add (GTK_CONTAINER (widget), status_icon);
  extra = GTK_WIDGET (phosh_quick_setting_get_status_icon (PHOSH_QUICK_SETTING (widget)));
  g_assert_true (extra == status_icon);

  g_object_set (G_OBJECT (status_icon), "info", TEST_STRING, NULL);

  extra = gtk_bin_get_child (GTK_BIN (widget));
  gtk_container_forall (GTK_CONTAINER (extra),
                      (GtkCallback) check_each_child_cb,
                      NULL);

  gtk_widget_destroy (widget);
}


int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/quick-setting/new", test_phosh_quick_setting_new);
  g_test_add_func("/phosh/quick-setting/with-status-icon", test_phosh_quick_setting_with_status_icon);

  return g_test_run();
}
