/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings-brightness"

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "settings/brightness.h"
#include "util.h"


GDBusProxy *brightness_proxy;
GCancellable *gsd_power_cancel;
gboolean setting_brightness;


static void
brightness_changed_cb (GDBusProxy *proxy,
                       GVariant   *changed_props,
                       GVariant   *invalidated_props,
                       gpointer   *user_data)
{
  GtkScale *scale = GTK_SCALE (user_data);
  int value;
  gboolean ret;

  if (setting_brightness)
    return;

  ret = g_variant_lookup (changed_props,
                          "Brightness",
                          "i", &value);
  g_return_if_fail (ret);
  if (value < 0 || value > 100)
    value = 100.0;
  gtk_range_set_value (GTK_RANGE (scale), value);
}


static void
brightness_init_cb (GObject      *source_object,
                    GAsyncResult *res,
                    GtkScale     *scale)
{
  g_autoptr(GError) err = NULL;
  GVariant *var;
  int value;

  brightness_proxy = g_dbus_proxy_new_finish (res, &err);
  if (brightness_proxy == NULL) {
    phosh_async_error_warn (err, "Could not connect to brightness service");
    return;
  }

  g_return_if_fail (GTK_IS_SCALE (scale));

  /* Set scale to current brightness */
  var = g_dbus_proxy_get_cached_property (brightness_proxy, "Brightness");
  if (var) {
    g_variant_get (var, "i", &value);
    setting_brightness = TRUE;
    gtk_range_set_value (GTK_RANGE (scale), value);
    setting_brightness = FALSE;
    g_variant_unref (var);
  }

  g_signal_connect (brightness_proxy,
                    "g-properties-changed",
                    G_CALLBACK(brightness_changed_cb),
                    scale);
}


void
brightness_init (GtkScale *scale)
{
  g_autoptr(GError) err = NULL;
  g_autoptr(GDBusConnection) session_con = NULL;

  session_con = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
  if (err != NULL) {
    g_warning ("Can not connect to session bus: %s", err->message);
    return;
  }

  gsd_power_cancel = g_cancellable_new ();
  g_dbus_proxy_new (session_con,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,
                    "org.gnome.SettingsDaemon.Power",
                    "/org/gnome/SettingsDaemon/Power",
                    "org.gnome.SettingsDaemon.Power.Screen",
                    gsd_power_cancel,
                    (GAsyncReadyCallback)brightness_init_cb,
                    scale);
}


static void
brightness_set_cb (GDBusProxy *proxy, GAsyncResult *res, gpointer unused)
{
  GError *err = NULL;
  GVariant *var;

  var = g_dbus_proxy_call_finish (proxy, res, &err);

  if (err) {
    g_warning ("Could not set brightness %s", err->message);
    g_error_free (err);
    return;
  }

  if (var)
    g_variant_unref (var);

  setting_brightness = FALSE;
}


void
brightness_set (int brightness)
{
  if (!brightness_proxy)
    return;

  if (setting_brightness)
    return;

  setting_brightness = TRUE;
  g_dbus_proxy_call (brightness_proxy,
                     "org.freedesktop.DBus.Properties.Set",
                     g_variant_new (
                         "(ssv)",
                         "org.gnome.SettingsDaemon.Power.Screen",
                         "Brightness",
                         g_variant_new ("i", brightness)),
                     G_DBUS_CALL_FLAGS_NONE,
                     2000,
                     NULL,
                     (GAsyncReadyCallback)brightness_set_cb,
                     NULL);
}


void
brightness_dispose (void)
{
  g_cancellable_cancel (gsd_power_cancel);
  g_clear_object (&gsd_power_cancel);
  g_clear_object (&brightness_proxy);
}
