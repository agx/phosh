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

enum {
  PROP_0,
  PROP_SHOW_DETAIL,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


typedef struct _PhoshBatteryInfo {
  PhoshStatusIcon  parent;
  UpClient        *upower;
  UpDevice        *device;
  gboolean         show_detail;
} PhoshBatteryInfo;


G_DEFINE_TYPE (PhoshBatteryInfo, phosh_battery_info, PHOSH_TYPE_STATUS_ICON)


static void
phosh_battery_info_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  switch (property_id) {
  case PROP_SHOW_DETAIL:
    phosh_battery_info_set_show_detail (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_battery_info_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshBatteryInfo *self = PHOSH_BATTERY_INFO (object);

  switch (property_id) {
  case PROP_SHOW_DETAIL:
    g_value_set_boolean (value, self->show_detail);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


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
    g_object_bind_property (self,
                            "info",
                            phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (self)),
                            "label",
                            G_BINDING_SYNC_CREATE);
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
  object_class->get_property = phosh_battery_info_get_property;
  object_class->set_property = phosh_battery_info_set_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-battery-info");

  props[PROP_SHOW_DETAIL] =
    g_param_spec_boolean (
      "show-detail",
      "",
      "",
      FALSE,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_battery_info_init (PhoshBatteryInfo *self)
{
  GtkWidget *percentage = gtk_label_new (NULL);
  phosh_status_icon_set_extra_widget (PHOSH_STATUS_ICON (self), percentage);

  g_object_bind_property (self,
                          "show-detail",
                          percentage,
                          "visible",
                          G_BINDING_SYNC_CREATE);
}


GtkWidget *
phosh_battery_info_new (void)
{
  return g_object_new (PHOSH_TYPE_BATTERY_INFO, NULL);
}


void
phosh_battery_info_set_show_detail (PhoshBatteryInfo *self, gboolean show)
{
  g_return_if_fail (PHOSH_IS_BATTERY_INFO (self));

  if (self->show_detail == show)
    return;

  self->show_detail = !!show;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_DETAIL]);
}


gboolean
phosh_battery_info_get_show_detail (PhoshBatteryInfo *self)
{
  g_return_val_if_fail (PHOSH_IS_BATTERY_INFO (self), FALSE);

  return self->show_detail;
}
