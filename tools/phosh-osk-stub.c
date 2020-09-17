/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-osk-stub"

#include "config.h"

#include <gio/gio.h>
#include <glib-unix.h>

#define GNOME_SESSION_DBUS_NAME      "org.gnome.SessionManager"
#define GNOME_SESSION_DBUS_OBJECT    "/org/gnome/SessionManager"
#define GNOME_SESSION_DBUS_INTERFACE "org.gnome.SessionManager"
#define GNOME_SESSION_CLIENT_PRIVATE_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

static GMainLoop *loop;
static GDBusProxy *_proxy;

/* TODO:
 - handle sm.puri.OSK0
 - launch squeekboard if found and fold/unfold on desktops is fixed
   https://source.puri.sm/Librem5/squeekboard/issues/132
*/

static void
print_version (void)
{
  g_message ("OSK stub %s\n", PHOSH_VERSION);
  exit (0);
}


static gboolean
quit_cb (gpointer user_data)
{
  g_info ("Caught signal, shutting down...");

  if (loop)
    g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
  else
    exit (0);

  return FALSE;
}


static void
respond_to_end_session (GDBusProxy *proxy, gboolean shutdown)
{
  /* we must answer with "EndSessionResponse" */
  g_dbus_proxy_call (proxy, "EndSessionResponse",
                     g_variant_new ("(bs)", TRUE, ""),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
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
    quit_cb (NULL);
  }
}


static void
on_client_registered (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  GVariant *variant;
  GDBusProxy *client_proxy;
  GError *error = NULL;
  gchar *object_path = NULL;

  variant = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (!variant) {
    g_warning ("Unable to register client: %s", error->message);
    g_error_free (error);
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
    g_error_free (error);
    return;
  }

  g_signal_connect (client_proxy, "g-signal",
                    G_CALLBACK (client_proxy_signal_cb), NULL);

  g_free (object_path);
  g_variant_unref (variant);
}


static void
stub_session_register (const char *client_id)
{
  const char *startup_id;
  GError *err = NULL;

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
      g_clear_error (&err);
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
}



int main(int argc, char *argv[])
{
  g_autoptr(GSource) sigterm = NULL;
  g_autoptr(GOptionContext) opt_context = NULL;
  GError *err = NULL;
  gboolean version = FALSE;

  const GOptionEntry options [] = {
    {"version", 0, 0, G_OPTION_ARG_NONE, &version,
     "Show version information", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- A OSK stub for phosh");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  if (version) {
    print_version ();
  }

  stub_session_register ("sm.puri.OSK0");
  loop = g_main_loop_new (NULL, FALSE);

  g_unix_signal_add (SIGTERM, quit_cb, NULL);
  g_unix_signal_add (SIGINT, quit_cb, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_object_unref (_proxy);

  return EXIT_SUCCESS;
}
