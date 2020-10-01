/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mount-manager"

#include "config.h"
#include "feedback-manager.h"
#include "mount-manager.h"
#include "shell.h"

#include <gio/gio.h>

#define AUTOMOUNT_KEY "automount"

/**
 * SECTION:mount-manager
 * @short_description: Mount devices
 * @Title: PhoshMountManager
 *
 * The #PhoshMountManager is responsible for auto mounting volumes and
 * notifying about new devices.
 */

typedef struct _PhoshMountManager {
  GObject         parent;

  GVolumeMonitor *monitor;
  GSettings      *settings;
  GPtrArray      *cancellables;

} PhoshMountManager;


G_DEFINE_TYPE (PhoshMountManager, phosh_mount_manager, G_TYPE_OBJECT)


static void
on_drive_connected (PhoshMountManager *self, GDrive *drive, GVolumeMonitor *monitor)
{
  g_autofree char *name = NULL;

  g_return_if_fail (G_IS_DRIVE (drive));

  name = g_drive_get_name (drive);
  g_debug ("Drive '%s' connected", name);

  if (!phosh_shell_is_session_active (phosh_shell_get_default ()))
    return;

  phosh_trigger_feedback ("device-added-media");
}


static void
on_drive_disconnected (PhoshMountManager *self, GDrive *drive, GVolumeMonitor *monitor)
{
  g_autofree char *name = NULL;

  g_return_if_fail (G_IS_DRIVE (drive));

  name = g_drive_get_name (drive);
  g_debug ("Drive '%s' disconnected", name);

  if (!phosh_shell_is_session_active (phosh_shell_get_default ()))
    return;

  phosh_trigger_feedback ("device-removed-media");
}


static void
on_mount_finished (GVolume *vol, GAsyncResult *res, PhoshMountManager *self)
{
  g_autoptr (GError) err = NULL;
  GCancellable *cancellable;

  g_return_if_fail (PHOSH_IS_MOUNT_MANAGER (self));
  g_return_if_fail (G_IS_VOLUME (vol));

  if (!g_volume_mount_finish (vol, res, &err)) {
    g_autofree char *name = g_volume_get_name (vol);
    g_warning ("Failed to mount volume '%s': %s", name, err->message);
  }

  cancellable = g_object_get_data (G_OBJECT (vol), "phosh-cancel");
  g_ptr_array_remove_fast (self->cancellables, cancellable);
  g_ptr_array_unref (self->cancellables);
  g_object_unref (vol);
  g_object_unref (self);
}


static void
on_volume_added (PhoshMountManager *self, GVolume *vol, GVolumeMonitor *monitor)
{
  gboolean automount;
  gpointer ignore_lock;
  GCancellable *cancellable;

  g_autoptr (GMount) mount = NULL;
  g_autofree gchar *name = NULL;

  g_return_if_fail (PHOSH_IS_MOUNT_MANAGER (self));
  g_return_if_fail (G_IS_VOLUME (vol));

  name = g_volume_get_name (vol);
  g_debug ("Volume added '%s'", name);

  if (!phosh_shell_is_session_active (phosh_shell_get_default ()))
    return;

  ignore_lock = g_object_get_data (G_OBJECT (vol), "phosh-ignore-lock");
  if (phosh_shell_get_locked (phosh_shell_get_default ()) && !ignore_lock)
    return;

  mount = g_volume_get_mount (vol);
  if (mount)
    return;

  automount = g_settings_get_boolean (self->settings, AUTOMOUNT_KEY);
  if (!automount || !g_volume_should_automount (vol))
    return;

  if (!g_volume_can_mount (vol)) {
    g_debug ("Volume '%s' can not be mounted", name);
    return;
  }

  cancellable = g_cancellable_new ();
  g_object_set_data (G_OBJECT (vol), "phosh-cancel", cancellable);
  g_ptr_array_add (self->cancellables, cancellable);
  g_ptr_array_ref (self->cancellables);
  g_debug ("Mounting '%s'", name);
  g_volume_mount (g_object_ref (vol), G_MOUNT_MOUNT_NONE, NULL, cancellable,
                  (GAsyncReadyCallback)on_mount_finished, g_object_ref (self));
}


static void
on_volume_removed (PhoshMountManager *self, GVolume *vol, GVolumeMonitor *monitor)
{
  g_autofree gchar *name = NULL;

  g_return_if_fail (PHOSH_IS_MOUNT_MANAGER (self));
  g_return_if_fail (G_IS_VOLUME (vol));

  if (!phosh_shell_is_session_active (phosh_shell_get_default ()))
    return;

  name = g_volume_get_name (vol);
  g_debug ("Volume '%s' removed", name);
}


static void
on_session_active_changed (PhoshMountManager *self, GParamSpec *pspec, PhoshSessionManager *sm)
{
  /* on_volume_added takes a ref on vol itself so we can cleanup here */
  g_autolist (GVolume) volumes = NULL;
  gboolean active;

  g_return_if_fail (PHOSH_IS_MOUNT_MANAGER (self));
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (sm));

  active = phosh_shell_is_session_active (phosh_shell_get_default ());
  g_debug ("Session active: %d", active);

  if (!active)
    return;

  /* oneshot */
  g_signal_handlers_disconnect_by_func (sm, on_session_active_changed, self);

  g_object_connect (self->monitor,
                    "swapped-signal::drive-connected", on_drive_connected, self,
                    "swapped-signal::drive-disconnected", on_drive_disconnected, self,
                    "swapped-signal::volume-added", on_volume_added, self,
                    "swapped-signal::volume-removed", on_volume_removed, self,
                    NULL);
  volumes = g_volume_monitor_get_volumes (self->monitor);
  for (GList *elem = volumes; elem != NULL; elem = elem->next) {
    GVolume *vol = G_VOLUME (elem->data);
    /* Ignore screen lock on initial startup */
    g_object_set_data (G_OBJECT (vol), "phosh-ignore-lock", GINT_TO_POINTER (TRUE));
    on_volume_added (self, vol, self->monitor);
  }

  return;
}


static void
phosh_mount_manager_constructed (GObject *object)
{
  PhoshMountManager *self = PHOSH_MOUNT_MANAGER (object);
  PhoshSessionManager *sm;

  self->cancellables = g_ptr_array_new_with_free_func (g_object_unref);
  self->monitor = g_volume_monitor_get ();
  self->settings = g_settings_new ("org.gnome.desktop.media-handling");

  sm = phosh_shell_get_session_manager (phosh_shell_get_default ());
  /* Wait for session to become active initially to mount everything so we don't
     race against #PhoshSessionManager binding to the bus */
  g_signal_connect_object (sm,
                           "notify::active",
                           G_CALLBACK (on_session_active_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_session_active_changed (self, NULL, sm);

  G_OBJECT_CLASS (phosh_mount_manager_parent_class)->constructed (object);
}


static void
phosh_mount_manager_dispose (GObject *object)
{
  PhoshMountManager *self = PHOSH_MOUNT_MANAGER (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->monitor);

  /* Cancel all ongoing mount opertions */
  for (int i = 0; i < self->cancellables->len; i++) {
    GCancellable *cancellable = g_ptr_array_index (self->cancellables, i);
    g_cancellable_cancel (cancellable);
  }
  g_clear_pointer (&self->cancellables, g_ptr_array_unref);

  G_OBJECT_CLASS (phosh_mount_manager_parent_class)->dispose (object);
}


static void
phosh_mount_manager_class_init (PhoshMountManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_mount_manager_constructed;
  object_class->dispose = phosh_mount_manager_dispose;
}


static void
phosh_mount_manager_init (PhoshMountManager *self)
{
}


PhoshMountManager *
phosh_mount_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_MOUNT_MANAGER, NULL);
}
