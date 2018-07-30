/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings-brightness"

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "settings/brightness.h"


GDBusProxy *brightness_proxy;
gboolean setting_brightness;


static void
brightness_changed_cb (GDBusProxy *proxy,
                       GVariant *changed_props,
                       GVariant *invalidated_props,
                       gpointer *user_data)
{
  GtkScale *scale = GTK_SCALE (user_data);
  gint value;
  gboolean ret;

  setting_brightness = TRUE;
  ret = g_variant_lookup (changed_props,
                          "Brightness",
                          "i", &value);
  g_return_if_fail (ret);
  if (value < 0 || value > 100)
    value = 100.0;
  gtk_range_set_value (GTK_RANGE (scale), value);
}


void
brightness_init (GtkScale *scale)
{
  GError *err = NULL;
  GDBusConnection *session_con;
  GDBusProxy *proxy;
  GVariant *var;
  gint value;

  session_con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);
  if (err != NULL) {
        g_error ("Can not connect to session bus: %s", err->message);
        g_error_free (err);
        return;
  }

  proxy = g_dbus_proxy_new_sync(session_con,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                "org.gnome.SettingsDaemon.Power",
                                "/org/gnome/SettingsDaemon/Power",
                                "org.gnome.SettingsDaemon.Power.Screen",
                                NULL,
                                &err);
  if (!proxy || err) {
    g_warning("Could not connect to brightness service %s", err->message);
    g_error_free(err);
    return;
  }

  /* Set scale to current brightness */
  var = g_dbus_proxy_get_cached_property (proxy, "Brightness");
  if (var) {
    g_variant_get (var, "i", &value);
    setting_brightness = TRUE;
    gtk_range_set_value (GTK_RANGE (scale), value);
    setting_brightness = FALSE;
    g_variant_unref (var);
  }

  g_signal_connect (proxy,
                    "g-properties-changed",
                    G_CALLBACK(brightness_changed_cb),
                    scale);

  brightness_proxy = proxy;
}



static void
brightness_set_cb (GDBusProxy *proxy, GAsyncResult *res, gpointer unused)
{
  GError *err = NULL;
  GVariant *var;

  var = g_dbus_proxy_call_finish (proxy, res, &err);

  if (err) {
    g_warning("Could not set brightness %s", err->message);
    g_error_free(err);
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
  g_clear_pointer (&brightness_proxy, g_object_unref);
}
