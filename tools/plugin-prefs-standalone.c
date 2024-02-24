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
on_activated (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       data)
{
  GtkApplication *app = GTK_APPLICATION (data);
  PhoshPluginLoader *loader;
  const char *name;
  GtkWindow *plugin_prefs, *parent;

  name = g_variant_get_string (parameter, NULL);
  g_debug ("Loading plugin '%s'", name);

  loader = g_object_get_data (G_OBJECT (app), "loader");
  plugin_prefs = GTK_WINDOW (phosh_plugin_loader_load_plugin (loader, name));

  parent = gtk_application_get_active_window (app);

  gtk_window_set_modal (plugin_prefs, TRUE);
  gtk_window_set_transient_for (plugin_prefs, GTK_WINDOW (parent));
  gtk_window_set_hide_on_close (plugin_prefs, TRUE);

  gtk_window_present (plugin_prefs);
}


static void
on_app_activated (GtkApplication *app)
{
  g_auto (GStrv) prefs_dirs = NULL;
  PhoshPluginLoader *loader;
  AdwApplicationWindow *window;
  GtkWidget *flowbox;
  GtkWidget *prefs;
  g_auto (GStrv) all_plugins = g_strsplit (PLUGINS, " ", -1);

  flowbox = g_object_new (GTK_TYPE_FLOW_BOX,
                          "valign", GTK_ALIGN_CENTER,
                          NULL);
  prefs_dirs = get_plugin_prefs_dirs ((const char * const*)all_plugins);
  loader = phosh_plugin_loader_new (prefs_dirs,
                                    PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS);
  g_object_set_data_full (G_OBJECT (app), "loader", loader, g_object_unref);

  for (int i = 0; all_plugins[i] != NULL; i++) {
    const char *plugin = all_plugins[i];
    g_autofree char *name = g_strdup_printf ("%s-prefs", plugin);
    g_autofree char *action_name = NULL;
    g_autofree char *shortcut = NULL;
    GtkWidget *button;
    char *accels[] = { NULL, NULL };

    prefs = phosh_plugin_loader_load_plugin (loader, name);
    if (prefs == NULL)
      continue;

    /* The plugins don't know that we're using a prefix for the installed data */
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

    button = g_object_new (GTK_TYPE_BUTTON,
                           "label", name,
                           "margin-start", 6,
                           "margin-end", 6,
                           "margin-top", 6,
                           "margin-bottom", 6,
                           NULL);
    action_name = g_strdup_printf ("app.show-prefs::%s", name);
    gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), action_name);
    gtk_flow_box_append (GTK_FLOW_BOX (flowbox), button);

    /* Set up shortcut, beware of collisions */
    shortcut = g_strdup_printf ("<ctrl>%c", name[0]);
    accels[0] = shortcut;
    gtk_application_set_accels_for_action (app, action_name, (const char *const *)accels);
  }

  window = g_object_new (GTK_TYPE_APPLICATION_WINDOW,
                         "application", app,
                         "title", "Plugin Preferences",
                         "child", flowbox,
                         NULL);

  gtk_window_present (GTK_WINDOW (window));
}


static GActionEntry entries[] =
{
  { .name = "show-prefs", .parameter_type = "s", .activate = on_activated },
};

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
  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   app);
  g_application_run (G_APPLICATION (app), argc, argv);

  return EXIT_SUCCESS;
}
