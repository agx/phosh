/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * BUILDDIR $ ./run_tool ./tools/app-grid-standalone
 *
 * PhoshAppGrid in a simple wrapper
 */

#include <app-grid.h>

static void
css_setup (void)
{
  GtkCssProvider *provider;
  GFile *file;
  GError *error = NULL;

  provider = gtk_css_provider_new ();
  file = g_file_new_for_uri ("resource:///sm/puri/phosh/style.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error)) {
    g_warning ("Failed to load CSS file: %s", error->message);
    g_clear_error (&error);
    g_object_unref (file);
    return;
  }
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider), 600);
  g_object_unref (file);
}

int
main (int argc, char *argv[])
{
  GtkWidget *win;
  GtkWidget *box;
  GtkWidget *widget;
  GtkWidget *revealer;

  gtk_init (&argc, &argv);

  css_setup ();

  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_widget_show (win);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  gtk_container_add (GTK_CONTAINER (win), box);

  gtk_widget_show (box);

  revealer = g_object_new (GTK_TYPE_REVEALER,
                           "reveal-child", TRUE,
                           "expand", TRUE,
                           FALSE);

  gtk_widget_show (revealer);

  gtk_container_add (GTK_CONTAINER (box), revealer);

  widget = g_object_new (GTK_TYPE_LABEL,
                         "label", "Running Apps",
                         "expand", TRUE,
                         NULL);

  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (revealer), widget);

  widget = g_object_new (PHOSH_TYPE_APP_GRID, NULL);

  g_object_bind_property (widget, "apps-expanded",
                          revealer, "reveal-child", G_BINDING_INVERT_BOOLEAN);

  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (box), widget);

  gtk_main ();

  return 0;
}
