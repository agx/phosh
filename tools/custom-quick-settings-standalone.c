/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-custom-quick-settings"

/**
 * A widget to test custom quick settings
 *
 * BUILDIR $ ./tools/run-tool ./tools/custom-quick-settings
 */

#include "phosh-config.h"

#include <handy.h>

#include "plugin-loader.h"
#include "quick-setting.h"


static void
css_setup (void)
{
  g_autoptr (GtkCssProvider) provider = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GError) error = NULL;

  provider = gtk_css_provider_new ();
  file = g_file_new_for_uri ("resource:///sm/puri/phosh/stylesheet/adwaita-dark.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error)) {
    g_warning ("Failed to load CSS file: %s", error->message);
    return;
  }
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}


static GStrv
get_plugin_dirs (GStrv plugins)
{
  g_autoptr (GPtrArray) dirs = g_ptr_array_new_with_free_func (g_free);

  for (int i = 0; i < g_strv_length (plugins); i++) {
    char *dir = g_strdup_printf (BUILD_DIR "/plugins/%s", plugins[i]);
    g_ptr_array_add (dirs, dir);
  }
  g_ptr_array_add (dirs, NULL);

  return (GStrv) g_ptr_array_steal (dirs, NULL);
}


static GtkWidget *
setup_plugins (GStrv plugin_dirs, GStrv plugins)
{
  GtkWidget *flow_box;
  g_autoptr (PhoshPluginLoader) loader = NULL;

  flow_box = g_object_new (GTK_TYPE_FLOW_BOX,
                           "name", "phosh_quick_settings",
                           "column-spacing", 12,
                           "row-spacing", 12,
                           "min-children-per-line", 2,
                           "max-children-per-line", 3,
                           "selection-mode", GTK_SELECTION_NONE,
                           "homogeneous", TRUE,
                           NULL);
  loader = phosh_plugin_loader_new (plugin_dirs, PHOSH_EXTENSION_POINT_QUICK_SETTING_WIDGET);

  for (int i = 0; i < g_strv_length (plugins); i++) {
    char *plugin = plugins[i];
    GtkWidget* widget = phosh_plugin_loader_load_plugin (loader, plugin);

    if (widget == NULL) {
      g_warning ("Unable to load plugin: %s", plugin);
    } else {
      g_print ("Adding custom quick setting '%s'", plugin);
      gtk_container_add (GTK_CONTAINER (flow_box), widget);
    }
  }

  return flow_box;
}


int
main (int argc, char *argv[])
{
  GtkWidget *win;
  GtkWidget *flow_box;
  g_auto (GStrv) plugins = g_strsplit (PLUGINS, " ", -1);
  g_auto (GStrv) plugin_dirs = NULL;

  gtk_init (&argc, &argv);
  hdy_init ();

  css_setup ();

  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (win), "Custom Quick Settings");
  g_signal_connect (win, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show (win);

  plugin_dirs = get_plugin_dirs (plugins);
  flow_box = setup_plugins (plugin_dirs, plugins);
  gtk_widget_show (flow_box);

  gtk_container_add (GTK_CONTAINER (win), flow_box);

  gtk_main ();

  return 0;
}
