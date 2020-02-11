/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on code from gnome-settings-daemon
 */

#define G_LOG_DOMAIN "phosh-session"

#include "session.h"
#include "shell.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#define GNOME_SESSION_DBUS_NAME      "org.gnome.SessionManager"
#define GNOME_SESSION_DBUS_OBJECT    "/org/gnome/SessionManager"
#define GNOME_SESSION_DBUS_INTERFACE "org.gnome.SessionManager"
#define GNOME_SESSION_CLIENT_PRIVATE_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

#define SESSION_SHUTDOWN_TIMEOUT 15

/**
 * SECTION:phosh-session
 * @short_description: Manages gnome-session registration and shutdown
 * @Title: PhoshSession
 *
 * The #PhoshSession is responsible for registration with
 * gnome-session and system shutdown
 */
static GDBusProxy *_proxy;


static void
respond_to_end_session (GDBusProxy *proxy, gboolean shutdown)
{
  if (shutdown)
    phosh_shell_fade_out (phosh_shell_get_default (), SESSION_SHUTDOWN_TIMEOUT);
  /* we must answer with "EndSessionResponse" */
  g_dbus_proxy_call (proxy, "EndSessionResponse",
                     g_variant_new ("(bs)", TRUE, ""),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
}


static void
do_stop (void)
{
  gtk_main_quit ();
}


static void
client_proxy_signal_cb (GDBusProxy *proxy,
                        gchar *sender_name,
                        gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
  if (g_strcmp0 (signal_name, "QueryEndSession") == 0) {
    g_debug ("Got QueryEndSession signal");
    respond_to_end_session (proxy, FALSE);
  } else if (g_strcmp0 (signal_name, "EndSession") == 0) {
    g_debug ("Got EndSession signal");
    respond_to_end_session (proxy, TRUE);
  } else if (g_strcmp0 (signal_name, "Stop") == 0) {
    g_debug ("Got Stop signal");
    do_stop ();
  }
}


static void
on_client_registered (GObject             *source_object,
                      GAsyncResult        *res,
                      gpointer             user_data)
{
  GVariant *variant;
  GDBusProxy *client_proxy;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *object_path = NULL;

  variant = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (!variant) {
    g_warning ("Unable to register client: %s", error->message);
    return;
  }

  g_variant_get (variant, "(o)", &object_path);

  g_debug ("Registered client at path %s", object_path);

  client_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 0, NULL,
                                                GNOME_SESSION_DBUS_NAME,
                                                object_path,
                                                GNOME_SESSION_CLIENT_PRIVATE_DBUS_INTERFACE,
                                                NULL,
                                                &error);
  if (!client_proxy) {
    g_warning ("Unable to get the session client proxy: %s", error->message);
    return;
  }

  g_signal_connect (client_proxy, "g-signal",
                    G_CALLBACK (client_proxy_signal_cb), NULL);
  g_variant_unref (variant);
}


void
phosh_session_register (const char *client_id)
{
  const char *startup_id;
  g_autoptr (GError) err = NULL;

  if (!_proxy) {
    _proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
                                            NULL,
                                            GNOME_SESSION_DBUS_NAME,
                                            GNOME_SESSION_DBUS_OBJECT,
                                            GNOME_SESSION_DBUS_INTERFACE,
                                            NULL,
                                            &err);
    if (!_proxy) {
      g_debug ("Failed to contact gnome-session: %s", err->message);
      return;
    }
  };

  startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
  g_dbus_proxy_call (_proxy,
                     "RegisterClient",
                     g_variant_new ("(ss)", client_id, startup_id ? startup_id : ""),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback) on_client_registered,
                     NULL);
  g_unsetenv ("DESKTOP_AUTOSTART_ID");
}


void
phosh_session_unregister (void)
{
  g_clear_object (&_proxy);
}


void
phosh_session_shutdown (void)
{
  g_return_if_fail (G_IS_DBUS_PROXY(_proxy));

  g_dbus_proxy_call (_proxy,
                     "Shutdown",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
}
