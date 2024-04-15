/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on mutter's src/backends/meta-idle-monitor-dbus.c
 */

#define G_LOG_DOMAIN "phosh-idle-manager"

#include "idle-manager.h"
#include "shell.h"

#include <gdk/gdkwayland.h>

/**
 * PhoshIdleManager:
 *
 * The idle manager singleton
 *
 * This uses ext_idle_notification_v1 Wayland protocol to
 * implement mutter's org.gnome.Mutter.IdleMonitor DBus
 * interface. Since we don't have per monitor information we only care
 * about core.
 *
 * Each DBus watch either notifies on idle *or* on activity.
 */

/* A DBus watch corresponding to either an idle or active timer */
typedef struct {
  /* DBus */
  PhoshIdleDBusIdleMonitor        *dbus_monitor;
  char                            *dbus_name;
  guint                            watch_id;
  guint                            name_watcher_id;
  /* Whether this watch reports on active or on idle */
  gboolean                         active;

  /* Wayland */
  struct ext_idle_notification_v1 *idle_noti;
  guint32                          interval;
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
  ext_idle_notification_v1_destroy (watch->idle_noti);
  g_bus_unwatch_name (watch->name_watcher_id);
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
idle_notification_idled_cb (void *data, struct ext_idle_notification_v1 *timer)
{
  DBusWatch *watch = data;
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (watch->dbus_monitor);

  if (watch->active)
    return;

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
idle_notification_resumed_cb (void* data, struct ext_idle_notification_v1 *timer)
{
  DBusWatch *watch = data;
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (watch->dbus_monitor);

  if (!watch->active)
    return;

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


static const struct ext_idle_notification_v1_listener idle_notification_listener = {
  .idled = idle_notification_idled_cb,
  .resumed = idle_notification_resumed_cb,
};


static void
name_vanished_callback (GDBusConnection *connection,
                        const char      *name,
                        gpointer         user_data)
{
  watch_remove ((DBusWatch*)user_data);
}


static DBusWatch *
watch_new (PhoshIdleDBusIdleMonitor *skeleton,
           GDBusMethodInvocation    *invocation,
           guint32                   interval,
           gboolean                  active)
{
  DBusWatch *watch;
  guint32 watch_id;
  PhoshWayland *wl = phosh_wayland_get_default ();
  struct ext_idle_notification_v1 *idle_noti;
  struct ext_idle_notifier_v1 *idle_manager = phosh_wayland_get_ext_idle_notifier_v1 (wl);

  watch_id = get_next_dbus_watch_serial ();
  g_return_val_if_fail (watch_id != 0, NULL); /* protect against wrap around */
  idle_noti = ext_idle_notifier_v1_get_idle_notification (idle_manager,
                                                          interval,
                                                          phosh_wayland_get_wl_seat (wl));
  g_assert (idle_noti);

  watch = g_new0 (DBusWatch, 1);
  watch->interval = interval;
  watch->active = active;
  watch->watch_id = watch_id;
  watch->idle_noti = idle_noti;
  watch->dbus_monitor = g_object_ref (skeleton);
  watch->dbus_name = g_strdup (g_dbus_method_invocation_get_sender (invocation));
  watch->name_watcher_id = g_bus_watch_name_on_connection (
    g_dbus_method_invocation_get_connection (invocation),
    watch->dbus_name,
    G_BUS_NAME_WATCHER_FLAGS_NONE,
    NULL, /* appeared */
    name_vanished_callback,
    watch, NULL);

  ext_idle_notification_v1_add_listener (watch->idle_noti, &idle_notification_listener, watch);

  return watch;
}


static DBusWatch *
idle_watch_new (PhoshIdleDBusIdleMonitor *skeleton,
                GDBusMethodInvocation    *invocation,
                guint32                   interval)
{
  DBusWatch *watch;

  watch = watch_new (skeleton, invocation, interval, FALSE);
  g_return_val_if_fail (watch, NULL);

  return watch;
}



static DBusWatch *
active_watch_new (PhoshIdleDBusIdleMonitor *skeleton, GDBusMethodInvocation *invocation)
{
  DBusWatch *watch;

  /* Use a idle timer of 0 since we're only interested in the active timer */
  watch = watch_new (skeleton, invocation, 0, TRUE);
  g_return_val_if_fail (watch, NULL);

  return watch;
}


static gboolean
handle_add_idle_watch (PhoshIdleDBusIdleMonitor *skeleton,
                       GDBusMethodInvocation    *invocation,
                       guint64                   arg_interval)
{
  DBusWatch *watch;
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  /* The wayland protocol uses an unsigned int */
  if (arg_interval > G_MAXUINT32) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_INVALID_ARGS,
                                           "interval %" G_GUINT64_FORMAT " > %" G_GUINT32_FORMAT,
                                           arg_interval, G_MAXUINT32);
    return TRUE;
  }
  watch = idle_watch_new (skeleton, invocation, arg_interval);
  if (!watch) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_LIMITS_EXCEEDED,
                                           "Failed to create watch");
    return TRUE;
  }

  g_debug ("Created idle-timer %d for %" G_GUINT64_FORMAT " msec", watch->watch_id, arg_interval);
  g_hash_table_insert (self->watches, &watch->watch_id, watch);
  phosh_idle_dbus_idle_monitor_complete_add_idle_watch (skeleton, invocation, watch->watch_id);
  return TRUE;
}


static gboolean
handle_add_user_active_watch (PhoshIdleDBusIdleMonitor *skeleton,
                              GDBusMethodInvocation    *invocation)
{
  DBusWatch *watch;
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  watch = active_watch_new (skeleton, invocation);
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
handle_get_idle_time (PhoshIdleDBusIdleMonitor *skeleton,
                      GDBusMethodInvocation    *invocation)
{
  g_debug ("Unimplemented DBus call %s", __func__);
  g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                         G_DBUS_ERROR_NOT_SUPPORTED,
                                         "Not supported");
  return TRUE;
}


static gboolean
handle_remove_watch (PhoshIdleDBusIdleMonitor *skeleton,
                     GDBusMethodInvocation    *invocation,
                     guint                     arg_id)
{
  PhoshIdleManager *self = phosh_idle_manager_get_default ();

  g_debug ("Removing watch %d", arg_id);
  g_hash_table_remove (self->watches, &arg_id);

  phosh_idle_dbus_idle_monitor_complete_remove_watch (skeleton, invocation);
  return TRUE;
}


static void
create_monitor_skeleton (GDBusObjectManagerServer *manager,
                         const char               *path)
{
  g_autoptr(GDBusObjectSkeleton) object = NULL;
  g_autoptr(GDBusInterfaceSkeleton) interface = NULL;

  object = G_DBUS_OBJECT_SKELETON (phosh_idle_dbus_object_skeleton_new (path));
  interface = G_DBUS_INTERFACE_SKELETON (
    phosh_idle_dbus_idle_monitor_skeleton_new ());

  g_signal_connect (interface, "handle-add-idle-watch",
                    G_CALLBACK (handle_add_idle_watch), NULL);
  g_signal_connect (interface, "handle-add-user-active-watch",
                    G_CALLBACK (handle_add_user_active_watch), NULL);
  g_signal_connect (interface, "handle-remove-watch",
                    G_CALLBACK (handle_remove_watch), NULL);
  g_signal_connect (interface, "handle-get-idletime",
                    G_CALLBACK (handle_get_idle_time), NULL);

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
  PhoshIdleManager *self = PHOSH_IDLE_MANAGER (user_data);
  g_autofree char *path = NULL;

  /* We need to use Mutter's object path here to make gnome-session happy */
  self->manager = g_dbus_object_manager_server_new ("/org/gnome/Mutter/IdleMonitor");

  /* We never clear the core monitor, as that's supposed to cumulate
     idle times from all devices */
  path = g_strdup ("/org/gnome/Mutter/IdleMonitor/Core");
  create_monitor_skeleton (self->manager, path);

  g_dbus_object_manager_server_set_connection (self->manager, connection);
}


static void
phosh_idle_manager_dispose (GObject *object)
{
  PhoshIdleManager *self = PHOSH_IDLE_MANAGER (object);

  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  g_clear_pointer (&self->watches, g_hash_table_destroy);
  g_clear_object (&self->manager);
  G_OBJECT_CLASS (phosh_idle_manager_parent_class)->dispose (object);
}

void
phosh_idle_manager_reset_timers (PhoshIdleManager *self)
{
  GHashTableIter iter;
  DBusWatch *watch;
  PhoshWayland *wl = phosh_wayland_get_default ();
  struct ext_idle_notifier_v1 *idle_manager = phosh_wayland_get_ext_idle_notifier_v1 (wl);

  g_return_if_fail (PHOSH_IS_IDLE_MANAGER (self));

  g_debug ("Resetting idle timers");

  g_hash_table_iter_init (&iter, self->watches);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &watch)) {
    if (watch->active)
      continue;

    /* Recreate the idle timers to reset their interval */
    ext_idle_notification_v1_destroy (watch->idle_noti);
    watch->idle_noti = ext_idle_notifier_v1_get_idle_notification (idle_manager,
                                                                   watch->interval,
                                                                   phosh_wayland_get_wl_seat (wl));
    ext_idle_notification_v1_add_listener (watch->idle_noti, &idle_notification_listener, watch);
  }
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
  self->dbus_name_id = 0;
}

/**
 * phosh_idle_manager_get_default:
 *
 * Get the idle manager singleton
 *
 * Returns:(transfer none): The idle manager
 */
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
