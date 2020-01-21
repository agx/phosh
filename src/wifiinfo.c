/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Wifi Info widget */

#define G_LOG_DOMAIN "phosh-wifiinfo"

#include "config.h"

#include "shell.h"
#include "wifiinfo.h"

/**
 * SECTION:phosh-wifi-info
 * @short_description: A widget to display the wifi status
 * @Title: PhoshWifiInfo
 */

struct _PhoshWifiInfo {
  PhoshStatusIcon parent;

  PhoshWifiManager *wifi;
};
G_DEFINE_TYPE (PhoshWifiInfo, phosh_wifi_info, PHOSH_TYPE_STATUS_ICON);


static void
update_icon (PhoshWifiInfo *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  const gchar *icon_name;

  g_debug ("Updating wifi icon");
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi));

  icon_name = phosh_wifi_manager_get_icon_name (wifi);
  if (icon_name)
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);

  gtk_widget_set_visible (GTK_WIDGET (self), icon_name ? TRUE : FALSE);
}

static void
update_info (PhoshWifiInfo *self)
{
  const gchar *info;
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));

  info = phosh_wifi_manager_get_ssid (self->wifi);
  if (info)
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), info);
  else
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Wi-Fi"));
}


static gboolean
on_idle (PhoshWifiInfo *self)
{
  update_icon (self, NULL, self->wifi);
  update_info (self);
  return FALSE;
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
    g_warning ("Failed to get wifi manager");
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

  g_idle_add ((GSourceFunc) on_idle, self);
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

  object_class->constructed = phosh_wifi_info_constructed;
  object_class->dispose = phosh_wifi_info_dispose;
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
