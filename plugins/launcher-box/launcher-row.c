/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"

#include "launcher-row.h"

#include <glib/gi18n.h>
#include <handy.h>


enum {
  PROP_0,
  PROP_LAUNCHER_ITEM,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshLauncherRow {
  HdyActionRow       parent;

  PhoshLauncherItem *item;

  GtkWidget         *count_label;
  GtkWidget         *progress_bar;
  GtkWidget         *box_data;
};
G_DEFINE_TYPE (PhoshLauncherRow, phosh_launcher_row, HDY_TYPE_ACTION_ROW)


static gboolean
transform_count_to_label (GBinding     *binding,
                          const GValue *from_value,
                          GValue       *to_value,
                          gpointer      user_data)
{
  gint64 count = g_value_get_int64 (from_value);
  char *label;

  label = g_strdup_printf ("%" G_GUINT64_FORMAT, count);
  g_value_take_string (to_value, label);
  return TRUE;
}


static void
set_item (PhoshLauncherRow *self, PhoshLauncherItem *item)
{
  g_autofree char *icon_name = NULL;
  const char *desc = NULL;
  GDesktopAppInfo *info;

  g_assert (!self->item);

  self->item = g_object_ref (item);
  info = phosh_launcher_item_get_app_info (item);
  hdy_preferences_row_set_title (HDY_PREFERENCES_ROW (self),
                                 g_app_info_get_display_name (G_APP_INFO (info)));

  icon_name = g_desktop_app_info_get_string (info, "Icon");
  if (icon_name)
    hdy_action_row_set_icon_name (HDY_ACTION_ROW (self), icon_name);

  desc = g_app_info_get_description (G_APP_INFO (info));
  hdy_action_row_set_subtitle (HDY_ACTION_ROW (self), desc);
  hdy_action_row_set_subtitle_lines (HDY_ACTION_ROW (self), 1);

  g_object_bind_property (self->item, "progress-visible",
                          self->progress_bar, "visible",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->item, "progress",
                          self->progress_bar, "value",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (self->item, "count-visible",
                          self->count_label, "visible",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (self->item, "count",
                               self->count_label, "label",
                               G_BINDING_SYNC_CREATE,
                               transform_count_to_label,
                               NULL, NULL, NULL);

  g_object_bind_property (self->item, "has-data",
                          self->box_data, "visible",
                          G_BINDING_SYNC_CREATE);
}


static void
phosh_launcher_row_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhoshLauncherRow *self = PHOSH_LAUNCHER_ROW (object);

  switch (property_id) {
  case PROP_LAUNCHER_ITEM:
    set_item (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_launcher_row_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshLauncherRow *self = PHOSH_LAUNCHER_ROW (object);

  switch (property_id) {
  case PROP_LAUNCHER_ITEM:
    g_value_set_object (value, self->item);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_launcher_row_finalize (GObject *object)
{
  PhoshLauncherRow *self = PHOSH_LAUNCHER_ROW (object);

  g_clear_object (&self->item);

  G_OBJECT_CLASS (phosh_launcher_row_parent_class)->finalize (object);
}


static void
phosh_launcher_row_class_init (PhoshLauncherRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_launcher_row_get_property;
  object_class->set_property = phosh_launcher_row_set_property;
  object_class->finalize = phosh_launcher_row_finalize;

  props[PROP_LAUNCHER_ITEM] =
    g_param_spec_object ("launcher-item", "", "",
                         PHOSH_TYPE_LAUNCHER_ITEM,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/launcher-box/launcher-row.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshLauncherRow, box_data);
  gtk_widget_class_bind_template_child (widget_class, PhoshLauncherRow, count_label);
  gtk_widget_class_bind_template_child (widget_class, PhoshLauncherRow, progress_bar);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_launcher_row_init (PhoshLauncherRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_launcher_row_new (PhoshLauncherItem *app_info)
{
  return GTK_WIDGET (g_object_new (PHOSH_TYPE_LAUNCHER_ROW, "launcher-item", app_info, NULL));
}


PhoshLauncherItem *
phosh_launcher_row_get_item (PhoshLauncherRow *self)
{
  g_return_val_if_fail (PHOSH_IS_LAUNCHER_ROW (self), NULL);

  return self->item;
}
