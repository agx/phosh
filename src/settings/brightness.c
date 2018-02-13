/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <gio/gio.h>
#include <gtk/gtk.h>

GDBusProxy *brightness_proxy;


void
brightness_changed_cb (GDBusProxy *proxy,
		       GVariant *changed_props,
		       GVariant *invalidated_props,
		       GtkAdjustment *adj)
{
  gboolean ret;
  gint value, cur;

  ret = g_variant_lookup (changed_props,
                          "Brightness",
                          "i", &value);

  /* The value returned from DBUs is one lower
   * than the value we set */
  cur = gtk_adjustment_get_value (adj);
  if (ret && cur != value + 1) {
    gtk_adjustment_set_value (adj, value + 1);
  }
}


void
brightness_init (GtkAdjustment *adj)
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

  /* Set adjustment to current brightness */
  var = g_dbus_proxy_get_cached_property (proxy, "Brightness");
  if (var) {
    g_variant_get (var, "i", &value);
    gtk_adjustment_set_value (adj, value);
    g_variant_unref (var);
  }

  g_signal_connect (proxy,
		    "g-properties-changed",
		    G_CALLBACK(brightness_changed_cb),
		    adj);

  brightness_proxy = proxy;
}



void
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
}


void
brightness_set (int brightness)
{
  g_return_if_fail (brightness_proxy);

  /* Don't let the display go completely dark for now */
  if (brightness < 10)
    brightness = 10;

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
