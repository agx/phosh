/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Battery Info widget */

#define G_LOG_DOMAIN "phosh-batteryinfo"

#include "config.h"

#include "batteryinfo.h"
#include "upower.h"

#define BATTERY_INFO_DEFAULT_ICON_SIZE GTK_ICON_SIZE_LARGE_TOOLBAR

/**
 * SECTION:phosh-battery-info
 * @short_description: A widget to display the battery status
 * @Title: PhoshBatteryInfo
 */

typedef struct _PhoshBatteryInfo {
  GtkImage      parent;

  UpClient     *upower;
  UpDevice     *device;
  GtkImage     *icon;
  gint          size;
  guint         update_icon_id;
} PhoshBatteryInfo;


G_DEFINE_TYPE (PhoshBatteryInfo, phosh_battery_info, GTK_TYPE_IMAGE)


static void
update_icon (PhoshBatteryInfo *self, gpointer unused)
{
  g_autofree gchar *icon_name = NULL;

  g_debug ("Updating battery icon");
  g_return_if_fail (PHOSH_IS_BATTERY_INFO (self));

  g_return_if_fail (self->device);

  g_object_get (self->device, "icon-name", &icon_name, NULL);

  if (icon_name)
    gtk_image_set_from_icon_name (GTK_IMAGE (self), icon_name, self->size);
}


static void
setup_display_device (PhoshBatteryInfo *self)
{
  GError *err = NULL;

  self->upower = up_client_new_full (NULL, &err);
  if (self->upower == NULL) {
    g_warning ("Failed to connect to upowerd: %s", err->message);
    g_clear_error (&err);
    return;
  }

  /* TODO: this is a oversimplified sync call */
  self->device = up_client_get_display_device (self->upower);
  if (self->device == NULL) {
    g_warning ("Failed to get upowerd display device");
    return;
  }

  self->update_icon_id = g_signal_connect_swapped (self->device,
                                                   "notify::icon-name",
                                                   G_CALLBACK (update_icon),
                                                   self);
}


static void
phosh_battery_info_constructed (GObject *object)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  G_OBJECT_CLASS (phosh_battery_info_parent_class)->constructed (object);

  setup_display_device (self);
  if (self->device)
    update_icon (self, NULL);
}


static void
phosh_battery_info_dispose (GObject *object)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  if (self->device) {
    g_signal_handler_disconnect (self->device, self->update_icon_id);
    self->update_icon_id = 0;
    g_clear_object (&self->device);
  }

  if (self->upower)
    g_clear_object (&self->upower);

  G_OBJECT_CLASS (phosh_battery_info_parent_class)->dispose (object);
}


static void
phosh_battery_info_class_init (PhoshBatteryInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_battery_info_constructed;
  object_class->dispose = phosh_battery_info_dispose;
}


static void
phosh_battery_info_init (PhoshBatteryInfo *self)
{
  /* TODO: make scalable? */
  self->size = BATTERY_INFO_DEFAULT_ICON_SIZE;
}


GtkWidget *
phosh_battery_info_new (void)
{
  return g_object_new (PHOSH_TYPE_BATTERY_INFO, NULL);
}
