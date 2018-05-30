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

typedef struct {
  UpClient     *upower;
  UpDevice     *device;
  GtkImage     *icon;
  gint          size;
  guint         update_icon_id;
} PhoshBatteryInfoPrivate;


typedef struct _PhoshBatteryInfo {
  GtkImage parent;
} PhoshBatteryInfo;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshBatteryInfo, phosh_battery_info, GTK_TYPE_IMAGE)


static void
update_icon (PhoshBatteryInfo *self, gpointer unused)
{
  PhoshBatteryInfoPrivate *priv;
  const gchar *icon_name;

  g_debug ("Updating battery icon");
  g_return_if_fail (PHOSH_IS_BATTERY_INFO (self));
  priv = phosh_battery_info_get_instance_private (self);

  g_object_get (priv->device, "icon-name", &icon_name, NULL);

  if (icon_name)
    gtk_image_set_from_icon_name (GTK_IMAGE (self), icon_name, priv->size);
}


static void
setup_display_device (PhoshBatteryInfo *self)
{
  GError *err = NULL;
  PhoshBatteryInfoPrivate *priv = phosh_battery_info_get_instance_private (self);

  priv->upower = up_client_new_full (NULL, &err);
  if (priv->upower == NULL) {
    g_warning ("Failed to connect to upowerd: %s", err->message);
    g_clear_error (&err);
  }

  /* TODO: this is a oversimplified sync call */
  priv->device = up_client_get_display_device (priv->upower);
  if (priv->device == NULL) {
    g_warning ("Failed to get upowerd display device");
  }

  priv->update_icon_id = g_signal_connect_swapped (priv->device,
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
  update_icon (self, NULL);
}


static void
phosh_battery_info_dispose (GObject *object)
{
  PhoshBatteryInfoPrivate *priv = phosh_battery_info_get_instance_private (PHOSH_BATTERY_INFO(object));

  if (priv->device) {
    g_signal_handler_disconnect (priv->device, priv->update_icon_id);
    priv->update_icon_id = 0;
    g_clear_object (&priv->device);
  }

  if (priv->upower)
    g_clear_object (&priv->upower);

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
  PhoshBatteryInfoPrivate *priv = phosh_battery_info_get_instance_private (self);

  /* TODO: make scalable? */
  priv->size = BATTERY_INFO_DEFAULT_ICON_SIZE;
}


GtkWidget *
phosh_battery_info_new (void)
{
  return g_object_new (PHOSH_TYPE_BATTERY_INFO, NULL);
}
