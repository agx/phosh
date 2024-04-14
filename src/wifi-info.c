/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Wi-Fi Info widget */

#define G_LOG_DOMAIN "phosh-wifiinfo"

#include "phosh-config.h"

#include "shell.h"
#include "wifi-info.h"

/**
 * PhoshWifiInfo:
 *
 * A widget to display the Wi-Fi status
 *
 * #PhoshWifiInfo displays the current Wi-Fi status based on information
 * from #PhoshWifiManager. To figure out if the widget should be shown
 * the #PhoshWifiInfo:enabled property can be useful.
 */

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshWifiInfo {
  PhoshStatusIcon parent;

  gboolean          enabled;
  gboolean          present;
  PhoshWifiManager *wifi;
};
G_DEFINE_TYPE (PhoshWifiInfo, phosh_wifi_info, PHOSH_TYPE_STATUS_ICON);

static void
phosh_wifi_info_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PhoshWifiInfo *self = PHOSH_WIFI_INFO (object);

  switch (property_id) {
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
update_icon (PhoshWifiInfo *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  const char *icon_name;

  g_debug ("Updating Wi-Fi icon");
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi));

  icon_name = phosh_wifi_manager_get_icon_name (wifi);
  if (icon_name)
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
}

static void
update_info (PhoshWifiInfo *self)
{
  const char *info;
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));

  info = phosh_wifi_manager_get_ssid (self->wifi);
  if (info)
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), info);
  else
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Wi-Fi"));
}

static void
on_wifi_enabled (PhoshWifiInfo *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  gboolean enabled;

  g_debug ("Updating Wi-Fi status");
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi));

  enabled = phosh_wifi_manager_get_enabled (wifi);
  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}

static void
on_wifi_present (PhoshWifiInfo *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  gboolean present;

  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi));

  present = phosh_wifi_manager_get_present (wifi);
  if (self->present == present)
    return;

  self->present = present;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
}


static void
phosh_wifi_info_idle_init (PhoshStatusIcon *icon)
{
  PhoshWifiInfo *self = PHOSH_WIFI_INFO (icon);

  update_icon (self, NULL, self->wifi);
  update_info (self);
  on_wifi_enabled (self, NULL, self->wifi);
}


static void
phosh_wifi_info_constructed (GObject *object)
{
  PhoshWifiInfo *self = PHOSH_WIFI_INFO (object);
  PhoshShell *shell;

  G_OBJECT_CLASS (phosh_wifi_info_parent_class)->constructed (object);

  shell = phosh_shell_get_default();
  self->wifi = g_object_ref(phosh_shell_get_wifi_manager (shell));

  if (self->wifi == NULL) {
    g_warning ("Failed to get Wi-Fi manager");
    return;
  }

  g_signal_connect_swapped (self->wifi,
                            "notify::icon-name",
                            G_CALLBACK (update_icon),
                            self);

  g_signal_connect_swapped (self->wifi,
                            "notify::ssid",
                            G_CALLBACK (update_info),
                            self);

  /* We don't use a binding for self->enabled so we can keep
     the property r/o */
  g_signal_connect_swapped (self->wifi,
                            "notify::enabled",
                            G_CALLBACK (on_wifi_enabled),
                            self);
  on_wifi_enabled (self, NULL, self->wifi);
  g_signal_connect_swapped (self->wifi,
                            "notify::present",
                            G_CALLBACK (on_wifi_present),
                            self);
  on_wifi_present (self, NULL, self->wifi);
}


static void
phosh_wifi_info_dispose (GObject *object)
{
  PhoshWifiInfo *self = PHOSH_WIFI_INFO(object);

  if (self->wifi) {
    g_signal_handlers_disconnect_by_data (self->wifi, self);
    g_clear_object (&self->wifi);
  }

  G_OBJECT_CLASS (phosh_wifi_info_parent_class)->dispose (object);
}


static void
phosh_wifi_info_class_init (PhoshWifiInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshStatusIconClass *status_icon_class = PHOSH_STATUS_ICON_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_wifi_info_constructed;
  object_class->dispose = phosh_wifi_info_dispose;
  object_class->get_property = phosh_wifi_info_get_property;

  status_icon_class->idle_init = phosh_wifi_info_idle_init;

  gtk_widget_class_set_css_name (widget_class, "phosh-wifi-info");

  /**
   * PhoshWifiInfo:enabled:
   *
   * Whether a Wi-Fi device is enabled
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshWifiInfo:present:
   *
   * Whether a Wi-Fi hardware is present
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_wifi_info_init (PhoshWifiInfo *self)
{
}


GtkWidget *
phosh_wifi_info_new (void)
{
  return g_object_new (PHOSH_TYPE_WIFI_INFO, NULL);
}
