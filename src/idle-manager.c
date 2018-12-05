/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * This uses org_kde_kwin_idle_timeout_listener implement mutter's
 * org.gnome.Mutter.IdleMonitor. Since we don't have per monitor
 * information we only care about core.
 *
 * Somewhat based on mutter's src/backends/meta-idle-monitor-dbus.c
 */

#define G_LOG_DOMAIN "phosh-idle-manager"

#include "idle-manager.h"
#include "shell.h"

#include <gdk/gdkwayland.h>


/* A DBus watch corresponding to either an idle or active timer */
typedef struct {
  PhoshIdleDbusIdleMonitor *dbus_monitor;
  struct org_kde_kwin_idle_timeout *idle_timer;
  PhoshMonitor *monitor;
  char *dbus_name;
  guint watch_id;
  guint name_watcher_id;
} DBusWatch;


/* The IdleManager maintains all watches */
typedef struct _PhoshIdleManager
{
  GObject parent;

  GHashTable *watches;
  GDBusObjectManagerServer *manager;
  int dbus_name_id;
} PhoshIdleManager;


G_DEFINE_TYPE (PhoshIdleManager, phosh_idle_manager, G_TYPE_OBJECT);


static guint32
get_next_dbus_watch_serial (void)
{
  static guint32 serial = 1;
  g_atomic_int_inc (&serial);
  return serial;
}


/* cleanup a single watch */
static void
watch_dispose (DBusWatch *watch)
{
  org_kde_kwin_idle_timeout_release (watch->idle_timer);
  g_bus_unwatch_name (watch->name_watcher_id);
  g_object_unref (watch->monitor);
  g_object_unref (watch->dbus_monitor);
  g_free (watch->dbus_name);
  g_free (watch);
}


/* remove a watch from the list of known watches */
static void
watch_remove (DBusWatch *watch)
{
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  g_debug ("Removing watch %d", watch->watch_id);
  g_hash_table_remove (self->watches, &watch->watch_id);
}


static void
idle_timer_idle_cb (void *data, struct org_kde_kwin_idle_timeout *timer)
{
  DBusWatch *watch = data;
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (watch->dbus_monitor);

  g_debug ("Idle Timer %d fired on %s", watch->watch_id, watch->dbus_name);
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 watch->dbus_name,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 "org.gnome.Mutter.IdleMonitor",
                                 "WatchFired",
                                 g_variant_new ("(u)", watch->watch_id),
                                 NULL);
}


static void
idle_timer_resume_cb (void* data, struct org_kde_kwin_idle_timeout *timer)
{
  DBusWatch *watch = data;

  /* Nothing todo here */
  g_debug ("Idle Timer %d resumed", watch->watch_id);
}

/* An DBus idle watch uses an idle_timeout but doesn't care about resume */
static const struct org_kde_kwin_idle_timeout_listener idle_timer_listener = {
  .idle = idle_timer_idle_cb,
  .resumed = idle_timer_resume_cb,
};


static void
active_timer_idle_cb (void *data, struct org_kde_kwin_idle_timeout *timer)
{
  /* Nothing todo */
}


static void
active_timer_resume_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
  DBusWatch *watch = data;
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (watch->dbus_monitor);

  g_debug ("Active Timer %d fired", watch->watch_id);
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 watch->dbus_name,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 "org.gnome.Mutter.IdleMonitor",
                                 "WatchFired",
                                 g_variant_new ("(u)", watch->watch_id),
                                 NULL);
  watch_remove (watch);
}


/* An DBus active watch cares about resume only */
static const struct org_kde_kwin_idle_timeout_listener active_timer_listener = {
  .idle = active_timer_idle_cb,
  .resumed = active_timer_resume_cb,
};


static void
name_vanished_callback (GDBusConnection *connection,
                        const char      *name,
                        gpointer         user_data)
{
  watch_remove ((DBusWatch*)user_data);
}


static DBusWatch *
watch_new (PhoshIdleDbusIdleMonitor *skeleton,
           GDBusMethodInvocation                  *invocation,
           PhoshMonitor                           *monitor,
           guint32                                interval)
{
  DBusWatch *watch;
  guint32 watch_id;
  PhoshWayland *wl = phosh_wayland_get_default ();
  struct org_kde_kwin_idle_timeout *idle_timer;
  struct org_kde_kwin_idle *idle_manager = phosh_wayland_get_org_kde_kwin_idle (wl);

  watch_id = get_next_dbus_watch_serial ();
  g_return_val_if_fail (watch_id != 0, NULL); /* protect against wrap around */
  idle_timer = org_kde_kwin_idle_get_idle_timeout(idle_manager,
                                                  phosh_wayland_get_wl_seat (wl),
                                                  interval);
  g_return_val_if_fail (idle_timer, NULL);

  watch = g_new0 (DBusWatch, 1);
  watch->watch_id = watch_id;
  watch->idle_timer = idle_timer;
  watch->dbus_monitor = g_object_ref (skeleton);
  watch->monitor = g_object_ref (monitor);
  watch->dbus_name = g_strdup (g_dbus_method_invocation_get_sender (invocation));
  watch->name_watcher_id = g_bus_watch_name_on_connection (
    g_dbus_method_invocation_get_connection (invocation),
    watch->dbus_name,
    G_BUS_NAME_WATCHER_FLAGS_NONE,
    NULL, /* appeared */
    name_vanished_callback,
    watch, NULL);

  return watch;
}


static DBusWatch *
idle_watch_new (PhoshIdleDbusIdleMonitor *skeleton,
                GDBusMethodInvocation    *invocation,
                PhoshMonitor             *monitor,
                guint32                  interval)
{
  DBusWatch *watch;

  watch = watch_new (skeleton, invocation, monitor, interval);
  g_return_val_if_fail (watch, NULL);
  org_kde_kwin_idle_timeout_add_listener(watch->idle_timer,
                                         &idle_timer_listener,
                                         watch);
  return watch;
}


static DBusWatch *
active_watch_new (PhoshIdleDbusIdleMonitor *skeleton,
                  GDBusMethodInvocation    *invocation,
                  PhoshMonitor             *monitor)
{
  DBusWatch *watch;

  /* Use a idle timer of 0 since we're only interested in the active timer */
  watch = watch_new (skeleton, invocation, monitor, 0);
  g_return_val_if_fail (watch, NULL);
  org_kde_kwin_idle_timeout_add_listener(watch->idle_timer,
                                         &active_timer_listener,
                                         watch);
  return watch;
}


static gboolean
handle_add_idle_watch (PhoshIdleDbusIdleMonitor *skeleton,
                       GDBusMethodInvocation    *invocation,
                       guint64                   arg_interval,
                       PhoshMonitor             *monitor)
{
  DBusWatch *watch;
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  g_return_val_if_fail (PHOSH_IS_MONITOR (monitor), FALSE);
  /* The wayland protocol uses an unsigned int */
  if (arg_interval > G_MAXUINT32) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_INVALID_ARGS,
                                           "interval %" G_GUINT64_FORMAT " > %" G_GUINT32_FORMAT,
                                           arg_interval, G_MAXUINT32);
    return TRUE;
  }
  watch = idle_watch_new (skeleton, invocation, monitor, arg_interval);
  if (!watch) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_LIMITS_EXCEEDED,
                                           "Failed to create watch");
    return TRUE;
  }

  g_debug ("Created idle-timer %d for %" G_GUINT64_FORMAT " msec", watch->watch_id, arg_interval);
  g_hash_table_insert (self->watches, &watch->watch_id, watch);
  phosh_idle_dbus_idle_monitor_complete_add_idle_watch (
    skeleton, invocation, watch->watch_id);
  return TRUE;
}


static gboolean handle_add_user_active_watch (PhoshIdleDbusIdleMonitor *skeleton,
                                              GDBusMethodInvocation    *invocation,
                                              PhoshMonitor             *monitor)
{
  DBusWatch *watch;
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  g_return_val_if_fail (PHOSH_IS_MONITOR (monitor), FALSE);
  watch = active_watch_new (skeleton, invocation, monitor);
  if (!watch) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_LIMITS_EXCEEDED,
                                           "Failed to create watch");
    return TRUE;
  }
  g_debug ("Creating active timer %d", watch->watch_id);
  g_hash_table_insert (self->watches, &watch->watch_id, watch);
  phosh_idle_dbus_idle_monitor_complete_add_user_active_watch (
    skeleton, invocation, watch->watch_id);
  return TRUE;
}


static gboolean
handle_get_idle_time (PhoshIdleDbusIdleMonitor *skeleton,
                      GDBusMethodInvocation    *invocation,
                      PhoshMonitor             *monitor)
{
  g_debug ("Unimplemented DBus call %s", __func__);
  g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                         G_DBUS_ERROR_NOT_SUPPORTED,
                                         "Not supported");
  return TRUE;
}


static gboolean
handle_remove_watch (PhoshIdleDbusIdleMonitor *skeleton,
                     GDBusMethodInvocation    *invocation,
                     guint                     arg_id,
                     PhoshMonitor             *monitor)
{
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  g_debug ("Removing watch %d", arg_id);
  g_hash_table_remove (self->watches, &arg_id);

  phosh_idle_dbus_idle_monitor_complete_remove_watch (
    skeleton, invocation);
  return TRUE;
}


static void
create_monitor_skeleton (GDBusObjectManagerServer *manager,
                         PhoshMonitor             *monitor,
                         const char               *path)
{
  g_autoptr(GDBusObjectSkeleton) object = NULL;
  g_autoptr(GDBusInterfaceSkeleton) interface = NULL;

  object = G_DBUS_OBJECT_SKELETON (phosh_idle_dbus_object_skeleton_new (path));
  interface = G_DBUS_INTERFACE_SKELETON (
    phosh_idle_dbus_idle_monitor_skeleton_new ());

  g_signal_connect_object (interface, "handle-add-idle-watch",
                           G_CALLBACK (handle_add_idle_watch), monitor, 0);
  g_signal_connect_object (interface, "handle-add-user-active-watch",
                           G_CALLBACK (handle_add_user_active_watch), monitor, 0);
  g_signal_connect_object (interface, "handle-remove-watch",
                           G_CALLBACK (handle_remove_watch), monitor, 0);
  g_signal_connect_object (interface, "handle-get-idletime",
                           G_CALLBACK (handle_get_idle_time), monitor, 0);

  g_dbus_object_skeleton_add_interface (object, interface);
  g_dbus_object_manager_server_export (manager, object);
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  g_debug ("Acquired name %s", name);
}


static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
  g_debug ("Lost or failed to acquire name %s", name);
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshIdleManager *self = user_data;
  PhoshMonitor *monitor;
  g_autofree gchar *path = NULL;

  /* We need to use Mutter's object path here to make gnome-session happy */
  self->manager = g_dbus_object_manager_server_new ("/org/gnome/Mutter/IdleMonitor");

  /* We never clear the core monitor, as that's supposed to cumulate
     idle times from all devices */
  monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default());
  path = g_strdup ("/org/gnome/Mutter/IdleMonitor/Core");
  create_monitor_skeleton (self->manager, monitor, path);

  g_dbus_object_manager_server_set_connection (self->manager, connection);
}


static void
phosh_idle_manager_dispose (GObject *object)
{
  PhoshIdleManager *self = PHOSH_IDLE_MANAGER (object);

  g_hash_table_destroy (self->watches);
  g_object_unref (self->manager);
  G_OBJECT_CLASS (phosh_idle_manager_parent_class)->dispose (object);
}


static void
phosh_idle_manager_constructed (GObject *object)
{
  PhoshIdleManager *self = PHOSH_IDLE_MANAGER (object);

  G_OBJECT_CLASS (phosh_idle_manager_parent_class)->constructed (object);

  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       "org.gnome.Mutter.IdleMonitor",
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT
                                       | G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       self, NULL);

  self->watches = g_hash_table_new_full (g_int_hash,
                                         g_int_equal,
                                         NULL,
                                         (GDestroyNotify) watch_dispose);
}


static void
phosh_idle_manager_class_init (PhoshIdleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_idle_manager_constructed;
  object_class->dispose = phosh_idle_manager_dispose;
}


static void
phosh_idle_manager_init (PhoshIdleManager *self)
{
  self->dbus_name_id = -1;
}


PhoshIdleManager *
phosh_idle_manager_get_default (void)
{
  static PhoshIdleManager *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_IDLE_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
