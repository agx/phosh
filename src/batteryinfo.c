/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Battery Info widget */

#define G_LOG_DOMAIN "phosh-batteryinfo"

#include "config.h"

#include "batteryinfo.h"
#include "upower.h"
#include "util.h"

/**
 * SECTION:batteryinfo
 * @short_description: A widget to display the battery status
 * @Title: PhoshBatteryInfo
 */

typedef struct _PhoshBatteryInfo {
  PhoshStatusIcon parent;
  UpClient     *upower;
  UpDevice     *device;
} PhoshBatteryInfo;


G_DEFINE_TYPE (PhoshBatteryInfo, phosh_battery_info, PHOSH_TYPE_STATUS_ICON)


static void
setup_display_device (PhoshBatteryInfo *self)
{
  g_autoptr (GError) err = NULL;

  self->upower = up_client_new_full (NULL, &err);
  if (self->upower == NULL) {
    phosh_dbus_service_error_warn (err, "Failed to connect to upowerd");
    return;
  }

  /* TODO: this is a oversimplified sync call */
  self->device = up_client_get_display_device (self->upower);
  if (self->device == NULL) {
    g_warning ("Failed to get upowerd display device");
    return;
  }
}


static gboolean
format_label_cb (GBinding *binding,
                 const GValue *from_value,
                 GValue *to_value,
                 gpointer user_data) {
  g_value_take_string (to_value,
                       g_strdup_printf ("%d%%", (int) (g_value_get_double (from_value) + 0.5)));
  return TRUE;
}


static void
phosh_battery_info_constructed (GObject *object)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  G_OBJECT_CLASS (phosh_battery_info_parent_class)->constructed (object);

  setup_display_device (self);
  if (self->device) {
    g_object_bind_property (self->device, "icon-name", self, "icon-name", G_BINDING_SYNC_CREATE);
    g_object_bind_property_full (self->device,
                            "percentage",
                            self,
                            "info",
                            G_BINDING_SYNC_CREATE,
                            format_label_cb,
                            NULL,
                            NULL,
                            NULL);
  }
}


static void
phosh_battery_info_dispose (GObject *object)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  g_clear_object (&self->device);
  g_clear_object (&self->upower);

  G_OBJECT_CLASS (phosh_battery_info_parent_class)->dispose (object);
}


static void
phosh_battery_info_class_init (PhoshBatteryInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_battery_info_constructed;
  object_class->dispose = phosh_battery_info_dispose;

  gtk_widget_class_set_css_name (widget_class, "phosh-battery-info");
}


static void
phosh_battery_info_init (PhoshBatteryInfo *self)
{
}


GtkWidget *
phosh_battery_info_new (void)
{
  return g_object_new (PHOSH_TYPE_BATTERY_INFO, NULL);
}
