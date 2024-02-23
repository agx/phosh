/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "launcher-box.h"
#include "launcher-row.h"

#include <gio/gdesktopappinfo.h>

#define LAUNCHER_BOX_SCHEMA_ID "sm.puri.phosh.plugins.launcher-box"
#define LAUNCHER_BOX_FOLDER_KEY "folder"

#define STR_IS_NULL_OR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

/**
 * PhoshLauncherBox:
 *
 * Show launchers in a folder.
 */
struct _PhoshLauncherBox {
  GtkBox        parent;

  GFileMonitor *monitor;
  GFile        *dir;
  char         *launcher_box_path;
  GCancellable *cancel;

  GListStore   *model;
  GtkListBox   *lb_launchers;
  GtkStack     *stack_launchers;
};

G_DEFINE_TYPE (PhoshLauncherBox, phosh_launcher_box, GTK_TYPE_BOX);

static void
on_row_selected (PhoshLauncherBox *self,
                 GtkListBoxRow    *row,
                 GtkListBox       *box)
{
  g_autoptr (GError) err = NULL;
  GDesktopAppInfo *app_info;
  GdkAppLaunchContext *context;

  if (row == NULL)
    return;

  app_info = phosh_launcher_row_get_app_info (PHOSH_LAUNCHER_ROW (row));
  g_debug ("row selected: %s", g_app_info_get_display_name (G_APP_INFO (app_info)));

  context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (self)));

  g_app_info_launch (G_APP_INFO (app_info), NULL, G_APP_LAUNCH_CONTEXT (context), &err);

  gtk_list_box_select_row (box, NULL);
}


static void
on_view_close_clicked (PhoshLauncherBox *self)
{
  gtk_stack_set_visible_child_name (self->stack_launchers, "launchers");
}


static void
phosh_launcher_box_finalize (GObject *object)
{
  PhoshLauncherBox *self = PHOSH_LAUNCHER_BOX (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->model);

  g_clear_object (&self->monitor);
  g_clear_object (&self->dir);
  g_clear_pointer (&self->launcher_box_path, g_free);

  G_OBJECT_CLASS (phosh_launcher_box_parent_class)->finalize (object);
}


static void
phosh_launcher_box_class_init (PhoshLauncherBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_launcher_box_finalize;

  g_type_ensure (PHOSH_TYPE_LAUNCHER_ROW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/launcher-box/launcher-box.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshLauncherBox, lb_launchers);
  gtk_widget_class_bind_template_child (widget_class, PhoshLauncherBox, stack_launchers);
  gtk_widget_class_bind_template_callback (widget_class, on_view_close_clicked);

  gtk_widget_class_set_css_name (widget_class, "phosh-launcher-box");
}


static GtkWidget *
create_launcher_row (gpointer item, gpointer user_data)
{
  return phosh_launcher_row_new (G_DESKTOP_APP_INFO (item));
}


static gint
app_info_compare (gconstpointer a, gconstpointer b, gpointer user_data)
{
  const char * f_a =  g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (a));
  const char * f_b =  g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (b));

  return g_strcmp0 (f_a, f_b);
}


static void
on_file_child_enumerated (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GFileEnumerator) enumerator = NULL;
  GFile *dir = G_FILE (source_object);
  PhoshLauncherBox *self;
  const char *stack_child = "launchers";

  enumerator = g_file_enumerate_children_finish (dir, res, &err);
  if (enumerator == NULL) {
    g_warning ("Failed to list %s", g_file_get_path (dir));
    return;
  }

  self = PHOSH_LAUNCHER_BOX (user_data);

  while (TRUE) {
    g_autoptr (GDesktopAppInfo) app_info = NULL;
    g_autofree char *path = NULL;
    GFile *file;
    GFileInfo *info;

    if (!g_file_enumerator_iterate (enumerator, &info, &file, self->cancel, &err)) {
      g_warning ("Failed to list contents of launcher dir %s: $%s", self->launcher_box_path, err->message);
      return;
    }

    if (!file)
      break;

    if (!g_str_has_suffix (g_file_info_get_name (info), ".desktop"))
      continue;

    if (g_strcmp0 (g_file_info_get_content_type (info), "application/x-desktop") != 0)
      continue;

    path = g_file_get_path (file);
    app_info = g_desktop_app_info_new_from_filename (path);
    if (!app_info)
      continue;

    g_list_store_insert_sorted (self->model, app_info, app_info_compare, NULL);
  }

  if (g_list_model_get_n_items (G_LIST_MODEL (self->model)) == 0)
    stack_child = "no-launchers";

  gtk_stack_set_visible_child_name (self->stack_launchers, stack_child);
}


static void
load_launchers (PhoshLauncherBox *self)
{
  g_autoptr (GSettings) settings = g_settings_new (LAUNCHER_BOX_SCHEMA_ID);
  g_autofree char *folder = NULL;

  folder = g_settings_get_string (settings, LAUNCHER_BOX_FOLDER_KEY);
  if (STR_IS_NULL_OR_EMPTY (folder)) {
    self->launcher_box_path = g_build_filename (g_get_user_config_dir (),
                                                "phosh", "plugins", "launcher-box", NULL);
  } else {
    self->launcher_box_path = g_steal_pointer (&folder);
  }

  self->dir = g_file_new_for_path (self->launcher_box_path);
  /*
   * Since the lockscreen is rebuilt on lock we don't worry about changes in directory contents,
   *  should we do, we can add a GFileMonitor later on
   */
  g_file_enumerate_children_async (self->dir,
                                   G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                   G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON ","
                                   G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                                   G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                                   G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                   G_FILE_QUERY_INFO_NONE,
                                   G_PRIORITY_LOW,
                                   self->cancel,
                                   on_file_child_enumerated,
                                   self);
}


static void
phosh_launcher_box_init (PhoshLauncherBox *self)
{
  g_autoptr (GtkCssProvider) css_provider = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->model = g_list_store_new (G_TYPE_DESKTOP_APP_INFO);

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/mobi/phosh/plugins/launcher-box/stylesheet/common.css");
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_list_box_bind_model (self->lb_launchers,
                           G_LIST_MODEL (self->model),
                           create_launcher_row,
                           NULL,
                           NULL);

  g_signal_connect_swapped (self->lb_launchers,
                            "row-selected",
                            G_CALLBACK (on_row_selected),
                            self);

  load_launchers (self);
}
