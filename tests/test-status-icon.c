/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "status-icon.h"

static void
test_phosh_status_icon_new (void)
{
  GtkWidget *widget;
  g_autofree char *icon_name = NULL;
  GtkIconSize icon_size;

  widget = phosh_status_icon_new ();
  g_assert_true (PHOSH_IS_STATUS_ICON (widget));

  icon_name = phosh_status_icon_get_icon_name (PHOSH_STATUS_ICON (widget));
  g_assert_null (icon_name);

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (widget), "does-not-matter");
  icon_name = phosh_status_icon_get_icon_name (PHOSH_STATUS_ICON (widget));
  g_assert_cmpstr (icon_name, ==, "does-not-matter");

  icon_size = phosh_status_icon_get_icon_size (PHOSH_STATUS_ICON (widget));
  g_assert_true (icon_size == GTK_ICON_SIZE_LARGE_TOOLBAR);

  gtk_widget_destroy (widget);
}


static void
test_phosh_status_icon_icon_size (void)
{
  GtkWidget *widget;
  GtkIconSize icon_size;

  widget = phosh_status_icon_new ();
  g_assert_true (PHOSH_IS_STATUS_ICON (widget));

  icon_size = phosh_status_icon_get_icon_size (PHOSH_STATUS_ICON (widget));
  g_assert_true (icon_size == GTK_ICON_SIZE_LARGE_TOOLBAR);

  phosh_status_icon_set_icon_size (PHOSH_STATUS_ICON (widget), GTK_ICON_SIZE_SMALL_TOOLBAR);
  icon_size = phosh_status_icon_get_icon_size (PHOSH_STATUS_ICON (widget));
  g_assert_true (icon_size == GTK_ICON_SIZE_SMALL_TOOLBAR);

  gtk_widget_destroy (widget);
}


static void
test_phosh_status_icon_icon_name (void)
{
  GtkWidget *widget;
  g_autofree char *icon_name = NULL;

  widget = phosh_status_icon_new ();
  g_assert_true (PHOSH_IS_STATUS_ICON (widget));

  icon_name = phosh_status_icon_get_icon_name (PHOSH_STATUS_ICON (widget));
  g_assert_null (icon_name);

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (widget), "test-symbolic");
  g_free (icon_name);
  icon_name = phosh_status_icon_get_icon_name (PHOSH_STATUS_ICON (widget));
  g_assert_cmpstr (icon_name, ==, "test-symbolic");

  g_free (icon_name);
  g_object_get (widget, "icon-name", &icon_name, NULL);
  g_assert_cmpstr (icon_name, ==, "test-symbolic");

  gtk_widget_destroy (widget);
}

static void
test_phosh_status_icon_extra_widget (void)
{
  GtkWidget *widget;
  GtkWidget *extra;
  GtkWidget *new_extra;

  extra = gtk_label_new (NULL);
  widget = phosh_status_icon_new ();
  g_assert_true (PHOSH_IS_STATUS_ICON (widget));

  extra = phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (widget));
  g_assert_null (extra);

  phosh_status_icon_set_extra_widget (PHOSH_STATUS_ICON (widget), extra);
  new_extra = phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (widget));
  g_assert_true (extra == new_extra);

  gtk_widget_destroy (widget);
}



int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/status-icon/new", test_phosh_status_icon_new);
  g_test_add_func("/phosh/status-icon/icon-size", test_phosh_status_icon_icon_size);
  g_test_add_func("/phosh/status-icon/icon-name", test_phosh_status_icon_icon_name);
  g_test_add_func("/phosh/status-icon/extra-widget", test_phosh_status_icon_extra_widget);

  return g_test_run();
}
