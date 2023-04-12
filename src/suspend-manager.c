/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-suspend-manager"

#include "phosh-config.h"

#include "login1-manager-dbus.h"
#include "util.h"
#include "shell.h"
#include "suspend-manager.h"
#include "wifimanager.h"

#include <gio/gunixfdlist.h>

#define LOGIN_BUS_NAME "org.freedesktop.login1"
#define LOGIN_OBJECT_PATH "/org/freedesktop/login1"

/**
 * PhoshSuspendManager:
 *
 * Manages suspend and inhibit's suspend when not useful.
 */
struct _PhoshSuspendManager {
  PhoshManager           parent;

  PhoshDBusLoginManager *logind_manager_proxy;
  GHashTable            *inhibitors; /* key: name, value: fd */

  GCancellable          *cancel;
};
G_DEFINE_TYPE (PhoshSuspendManager, phosh_suspend_manager, PHOSH_TYPE_MANAGER)


static int
inhibit_suspend_finish (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  gboolean success;
  PhoshDBusLoginManager *proxy;
  g_autoptr (GError) err = NULL;
  g_autoptr (GUnixFDList) fd_list = NULL;
  g_autoptr (GVariant) out_pipe_fd = NULL;
  int idx, fd;

  proxy = PHOSH_DBUS_LOGIN_MANAGER (source_object);
  success = phosh_dbus_login_manager_call_inhibit_finish (proxy,
                                                          &out_pipe_fd,
                                                          &fd_list,
                                                          res,
                                                          &err);
  if (!success) {
    g_warning ("Failed to inhibit suspend: %s", err->message);
    return -1;
  }

  g_return_val_if_fail (fd_list && g_unix_fd_list_get_length (fd_list) == 1, -1);

  g_variant_get (out_pipe_fd, "h", &idx);
  fd = g_unix_fd_list_get (fd_list, idx, &err);
  if (fd < 0) {
    g_warning ("Failed to get suspend inhibit fd: %s", err->message);
    return -1;
  }
  return fd;
}


static void
on_inhibit_wifi_suspend_finish (GObject      *source_object,
                                GAsyncResult *res,
                                gpointer      user_data)
{
  PhoshSuspendManager *self = PHOSH_SUSPEND_MANAGER (user_data);

  int fd = inhibit_suspend_finish (source_object, res, user_data);
  if (fd < 0)
    return;

  g_debug ("Inhibited logind suspend for wifi hotspot");
  g_hash_table_insert (self->inhibitors, g_strdup ("wifi-hotspot"), GINT_TO_POINTER (fd));
}


static void
on_is_hotspot_master_changed (PhoshSuspendManager *self,
                              GParamSpec          *pspec,
                              PhoshWifiManager    *wifi_manager)
{
  gboolean is_hotspot_master;

  g_return_if_fail (PHOSH_IS_SUSPEND_MANAGER (self));
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (wifi_manager));

  is_hotspot_master = phosh_wifi_manager_is_hotspot_master (wifi_manager);

  if (is_hotspot_master) {
    phosh_dbus_login_manager_call_inhibit (self->logind_manager_proxy,
                                           "sleep",
                                           g_get_user_name (),
                                           "WiFi hotspot active",
                                           "block",
                                           NULL,
                                           self->cancel,
                                           on_inhibit_wifi_suspend_finish,
                                           self);
  } else {
    g_debug ("Clearing wifi hotspot suspend inhibit");
    g_hash_table_remove (self->inhibitors, "wifi-hotspot");
  }
}



static void
uninhibit (gpointer pointer)
{
  int fd = GPOINTER_TO_INT (pointer);

  if (fd >= 0)
    close (fd);
}


static void
on_logind_manager_proxy_new_for_bus_finish (GObject             *source_object,
                                            GAsyncResult        *res,
                                            PhoshSuspendManager *self)
{
  g_autoptr (GError) err = NULL;
  g_autofree char *session_id = NULL;
  PhoshWifiManager *wifi_manager;
  PhoshDBusLoginManager *proxy;

  proxy = phosh_dbus_login_manager_proxy_new_for_bus_finish (res, &err);
  if (proxy == NULL) {
    phosh_dbus_service_error_warn (err, "Failed to get login1 manager proxy: %s", err->message);
    return;
  }

  g_return_if_fail (PHOSH_IS_SUSPEND_MANAGER (self));
  g_debug ("Connected to " LOGIN_OBJECT_PATH);
  self->logind_manager_proxy = proxy;

  wifi_manager = phosh_shell_get_wifi_manager (phosh_shell_get_default());
  g_signal_connect_swapped (wifi_manager,
                            "notify::is-hotspot-master",
                            G_CALLBACK (on_is_hotspot_master_changed),
                            self);
  on_is_hotspot_master_changed (self, NULL, wifi_manager);
}


static void
phosh_suspend_manager_idle_init (PhoshManager *manager)
{
  PhoshSuspendManager *self = PHOSH_SUSPEND_MANAGER (manager);

  phosh_dbus_login_manager_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    LOGIN_BUS_NAME,
    LOGIN_OBJECT_PATH,
    self->cancel,
    (GAsyncReadyCallback) on_logind_manager_proxy_new_for_bus_finish,
    self);
}


static void
phosh_suspend_manager_finalize (GObject *object)
{
  PhoshSuspendManager *self = PHOSH_SUSPEND_MANAGER(object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->logind_manager_proxy);
  g_clear_pointer (&self->inhibitors, g_hash_table_destroy);

  G_OBJECT_CLASS (phosh_suspend_manager_parent_class)->finalize (object);
}


static void
phosh_suspend_manager_class_init (PhoshSuspendManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->finalize = phosh_suspend_manager_finalize;
  manager_class->idle_init = phosh_suspend_manager_idle_init;
}


static void
phosh_suspend_manager_init (PhoshSuspendManager *self)
{
  self->cancel = g_cancellable_new ();
  self->inhibitors = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            uninhibit);
}


PhoshSuspendManager *
phosh_suspend_manager_new (void)
{
  return PHOSH_SUSPEND_MANAGER (g_object_new (PHOSH_TYPE_SUSPEND_MANAGER, NULL));
}
