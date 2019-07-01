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

#define WIFI_INFO_DEFAULT_ICON_SIZE GTK_ICON_SIZE_LARGE_TOOLBAR

/**
 * SECTION:phosh-wifi-info
 * @short_description: A widget to display the wifi status
 * @Title: PhoshWifiInfo
 */

struct _PhoshWifiInfo {
  GtkImage   parent;

  PhoshWifiManager *wifi;
  GtkImage         *icon;
  gint              size;
};
G_DEFINE_TYPE (PhoshWifiInfo, phosh_wifi_info, GTK_TYPE_IMAGE);


static void
update_icon (PhoshWifiInfo *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  g_autofree gchar *icon_name = NULL;

  g_debug ("Updating wifi icon");
  g_return_if_fail (PHOSH_IS_WIFI_INFO (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi));

  g_object_get (wifi, "icon-name", &icon_name, NULL);

  if (icon_name)
    gtk_image_set_from_icon_name (GTK_IMAGE (self), icon_name, self->size);

  gtk_widget_set_visible (GTK_WIDGET (self), icon_name ? TRUE : FALSE);
}


static gboolean
on_idle (PhoshWifiInfo *self)
{
  update_icon (self, NULL, self->wifi);
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
  self->size = WIFI_INFO_DEFAULT_ICON_SIZE;

  if (self->wifi == NULL) {
    g_warning ("Failed to get wifi manager");
    return;
  }

  g_signal_connect_swapped (self->wifi,
                            "notify::icon-name",
                            G_CALLBACK (update_icon),
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
