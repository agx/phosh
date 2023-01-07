/*
 * Copyright © 2022 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * BUILDDIR $ ./tools/run_tool ./tools/plugin-prefs-standalone
 *
 * plugin-prefs-standalone: A simple wrapper to look at plugin preferenes
 */

#include "phosh-config.h"
#include "phosh-plugin.h"

#include <adwaita.h>

#include <plugin-loader.h>

#include <glib/gi18n.h>


static const char *all_plugins[] = {
  "calendar",
  "upcoming-events",
  "ticket-box",
  NULL,
};


static GStrv
get_plugin_prefs_dirs (const char *const *plugins)
{
  g_autoptr (GPtrArray) dirs = g_ptr_array_new_with_free_func (g_free);

  for (int i = 0; plugins[i] != NULL; i++) {
    char *dir = g_strdup_printf (BUILD_DIR "/plugins/%s/prefs", plugins[i]);
    g_ptr_array_add (dirs, dir);
  }
  g_ptr_array_add (dirs, NULL);

  return (GStrv) g_ptr_array_steal (dirs, NULL);
}


static void
on_button_clicked (GtkWidget *button, gpointer data)
{
  GtkWindow *plugin_prefs = GTK_WINDOW (data);
  GtkWidget *parent;

  parent = gtk_widget_get_ancestor (button, GTK_TYPE_WINDOW);

  gtk_window_set_modal (plugin_prefs, TRUE);
  gtk_window_set_transient_for (plugin_prefs, GTK_WINDOW (parent));
  gtk_window_set_hide_on_close (plugin_prefs, TRUE);

  gtk_window_present (plugin_prefs);
}


static void
on_app_activated (GtkApplication *app)
{
  g_autoptr (PhoshPluginLoader) loader = NULL;
  g_auto (GStrv) prefs_dirs = NULL;
  GtkWidget *prefs;
  AdwApplicationWindow *window;
  GtkWidget *flowbox;

  flowbox = g_object_new (GTK_TYPE_FLOW_BOX,
                          "valign", GTK_ALIGN_CENTER,
                          NULL);
  prefs_dirs = get_plugin_prefs_dirs (all_plugins);
  loader = phosh_plugin_loader_new (prefs_dirs,
                                    PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS);

  for (int i = 0; all_plugins[i] != NULL; i++) {
    const char *plugin = all_plugins[i];
    g_autofree char *name = g_strdup_printf ("%s-prefs", plugin);
    GtkWidget *button;

    prefs = phosh_plugin_loader_load_plugin (loader, name);
    if (prefs == NULL)
      continue;

    button = g_object_new (GTK_TYPE_BUTTON,
                           "label", name,
                           "margin-start", 6,
                           "margin-end", 6,
                           "margin-top", 6,
                           "margin-bottom", 6,
                           NULL);
    g_signal_connect (button, "clicked", G_CALLBACK (on_button_clicked), prefs);
    gtk_flow_box_append (GTK_FLOW_BOX (flowbox), button);
  }

  window = g_object_new (GTK_TYPE_APPLICATION_WINDOW,
                         "application", app,
                         "title", "Plugin Preferences",
                         "child", flowbox,
                         NULL);

  gtk_window_present (GTK_WINDOW (window));
}


int
main (int argc, char *argv[])
{
  g_autoptr (AdwApplication) app = NULL;

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  app = g_object_new (ADW_TYPE_APPLICATION,
                      "application-id", "sm.puri.phosh.PluginPrefsStandalone",
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_app_activated), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return EXIT_SUCCESS;
}
