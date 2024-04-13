/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-app-grid-folder-button"

#include "app-grid-button.h"
#include "app-grid-folder-button.h"

/**
 * PhoshAppGridFolderButton:
 *
 * A widget to display the apps in a folder.
 */

enum {
  PROP_0,
  PROP_FOLDER_INFO,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  FOLDER_LAUNCHED,
  N_SIGNALS,
};
static guint signals[N_SIGNALS];

struct _PhoshAppGridFolderButton {
  PhoshAppGridBaseButton parent;

  PhoshFolderInfo       *folder_info;
  GtkGrid               *grid;
};

G_DEFINE_TYPE (PhoshAppGridFolderButton, phosh_app_grid_folder_button, PHOSH_TYPE_APP_GRID_BASE_BUTTON);


static void
phosh_app_grid_folder_button_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  PhoshAppGridFolderButton *self = PHOSH_APP_GRID_FOLDER_BUTTON (object);

  switch (property_id) {
  case PROP_FOLDER_INFO:
    self->folder_info = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_app_grid_folder_button_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  PhoshAppGridFolderButton *self = PHOSH_APP_GRID_FOLDER_BUTTON (object);

  switch (property_id) {
  case PROP_FOLDER_INFO:
    g_value_set_object (value, self->folder_info);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_activated_cb (PhoshAppGridFolderButton *self)
{
  g_signal_emit (self, signals[FOLDER_LAUNCHED], 0, self->folder_info);
}


static void
build_2x2_grid_icon (PhoshAppGridFolderButton *self)
{
  GListModel *apps;
  g_autoptr (GAppInfo) app_info = NULL;
  GIcon *icon;

  gtk_grid_remove_row (self->grid, 1);
  gtk_grid_remove_row (self->grid, 0);

  apps = G_LIST_MODEL (phosh_folder_info_get_app_infos (self->folder_info));

  for (int i = 0;
       i < 4 && (app_info = g_list_model_get_item (apps, i)) != NULL;
       i++) {
    GtkWidget *image = NULL;
    app_info = g_list_model_get_item (apps, i);

    icon = g_app_info_get_icon (app_info);

    if (icon == NULL) {
      image = gtk_image_new_from_icon_name (PHOSH_APP_UNKNOWN_ICON,
                                            GTK_ICON_SIZE_BUTTON);
    } else {
      if (G_IS_THEMED_ICON (icon)) {
        g_themed_icon_append_name (G_THEMED_ICON (icon),
                                   PHOSH_APP_UNKNOWN_ICON);
      }
      image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
    }

    gtk_widget_set_visible (image, TRUE);
    gtk_grid_attach (self->grid, image, i % 2, i >= 2, 1, 1);
  }
}


static void
phosh_app_grid_folder_button_dispose (GObject *object)
{
  PhoshAppGridFolderButton *self = PHOSH_APP_GRID_FOLDER_BUTTON (object);

  g_clear_object (&self->folder_info);

  G_OBJECT_CLASS (phosh_app_grid_folder_button_parent_class)->dispose (object);
}


static void
phosh_app_grid_folder_button_constructed (GObject *object)
{
  PhoshAppGridFolderButton *self = PHOSH_APP_GRID_FOLDER_BUTTON (object);
  GListModel *apps;

  G_OBJECT_CLASS (phosh_app_grid_folder_button_parent_class)->constructed (object);

  apps = G_LIST_MODEL (phosh_folder_info_get_app_infos (self->folder_info));

  g_signal_connect_object (apps, "items-changed", G_CALLBACK (build_2x2_grid_icon), self, G_CONNECT_SWAPPED);
  g_object_bind_property (self->folder_info, "name", self, "label", G_BINDING_SYNC_CREATE);
  build_2x2_grid_icon (self);
}


static void
phosh_app_grid_folder_button_class_init (PhoshAppGridFolderButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_app_grid_folder_button_set_property;
  object_class->get_property = phosh_app_grid_folder_button_get_property;
  object_class->dispose = phosh_app_grid_folder_button_dispose;
  object_class->constructed = phosh_app_grid_folder_button_constructed;

  /**
   * PhoshAppGridFolderButton:folder-info:
   *
   * The folder info whose apps the widget displays
   */
  props[PROP_FOLDER_INFO] =
    g_param_spec_object ("folder-info", "", "",
                         PHOSH_TYPE_FOLDER_INFO,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[FOLDER_LAUNCHED] = g_signal_new ("folder-launched",
                                           G_OBJECT_CLASS_TYPE (object_class),
                                           G_SIGNAL_RUN_LAST,
                                           0, NULL, NULL, NULL,
                                           G_TYPE_NONE, 1, PHOSH_TYPE_FOLDER_INFO);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid-folder-button.ui");

  gtk_widget_class_bind_template_callback (widget_class, on_activated_cb);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppGridFolderButton, grid);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid-folder-button");
}


static void
phosh_app_grid_folder_button_init (PhoshAppGridFolderButton *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_app_grid_folder_button_new_from_folder_info (PhoshFolderInfo *folder_info)
{
  return g_object_new (PHOSH_TYPE_APP_GRID_FOLDER_BUTTON, "folder-info", folder_info, NULL);
}
