/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen-manager"

#include "lockscreen-manager.h"
#include "lockscreen.h"
#include "lockshield.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "phosh-wayland.h"
#include "shell.h"
#include "util.h"
#include "session-presence.h"
#include <gdk/gdkwayland.h>

/**
 * SECTION:lockscreen-manager
 * @short_description: The singleton that manages screen locking
 * @Title: PhoshLockscreenManager
 *
 * The #PhoshLockscreenManager is responsible for putting the #PhoshLockscreen
 * on the primary output and a #PhoshLockshield on other outputs when the session
 * becomes idle or when invoked explicitly via phosh_lockscreen_manager_set_locked().
 */

/* See https://people.gnome.org/~mccann/gnome-session/docs/gnome-session.html#org.gnome.SessionManager.Presence:status */
#define GNOME_SESSION_STATUS_AVAILABLE 0
#define GNOME_SESSION_STATUS_INVISIBLE 1
#define GNOME_SESSION_STATUS_BUSY      2
#define GNOME_SESSION_STATUS_IDLE      3

enum {
  WAKEUP_OUTPUTS,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

enum {
  PHOSH_LOCKSCREEN_MANAGER_PROP_0,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED,
  PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP];


struct _PhoshLockscreenManager {
  GObject parent;

  PhoshLockscreen *lockscreen;     /* phone display lock screen */
  PhoshSessionPresence *presence;  /* gnome-session's presence interface */
  GPtrArray *shields;              /* other outputs */
  GSettings *settings;

  int timeout;                     /* timeout in seconds before screen locks */
  gboolean locked;
  gint64 active_time;              /* when lock was activated (in us) */
  int transform;                   /* the shell transform before locking */
};

G_DEFINE_TYPE (PhoshLockscreenManager, phosh_lockscreen_manager, G_TYPE_OBJECT)


static void
lockscreen_unlock_cb (PhoshLockscreenManager *self, PhoshLockscreen *lockscreen)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  phosh_shell_set_transform (shell, self->transform);
  self->transform = PHOSH_MONITOR_TRANSFORM_NORMAL;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (lockscreen));
  g_return_if_fail (lockscreen == PHOSH_LOCKSCREEN (self->lockscreen));

  g_signal_handlers_disconnect_by_data (monitor_manager, self);
  g_signal_handlers_disconnect_by_data (shell, self);
  g_clear_pointer (&self->lockscreen, phosh_cp_widget_destroy);

  /* Unlock all other outputs */
  g_clear_pointer (&self->shields, g_ptr_array_unref);

  self->locked = FALSE;
  self->active_time = 0;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void
lockscreen_wakeup_output_cb (PhoshLockscreenManager *self, PhoshLockscreen *lockscreen)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (lockscreen));

  /* we just proxy the signal here */
  g_signal_emit (self, signals[WAKEUP_OUTPUTS], 0);
}


/* Lock a non primary monitor bringing up a shield */
static void
lock_monitor (PhoshLockscreenManager *self,
              PhoshMonitor           *monitor)
{
  PhoshWayland *wl = phosh_wayland_get_default ();
  GtkWidget *shield;

  shield = phosh_lockshield_new (
    phosh_wayland_get_zwlr_layer_shell_v1 (wl),
    monitor->wl_output);

  g_object_set_data (G_OBJECT (shield), "phosh-monitor", monitor);

  g_ptr_array_add (self->shields, shield);
  gtk_widget_show (shield);
}


static void
remove_shield_by_monitor (PhoshLockscreenManager *self,
                          PhoshMonitor           *monitor)
{
  for (int i = 0; i < self->shields->len; i++) {
    PhoshMonitor *shield_monitor;
    PhoshLockshield *shield = g_ptr_array_index (self->shields, i);

    shield_monitor = g_object_get_data (G_OBJECT (shield), "phosh-monitor");
    g_return_if_fail (PHOSH_IS_MONITOR (shield_monitor));
    if (shield_monitor == monitor) {
      g_debug ("Removing shield %p", shield);
      g_ptr_array_remove (self->shields, shield);
      break;
    }
  }
}


static void
on_monitor_removed (PhoshLockscreenManager *self,
                    PhoshMonitor           *monitor,
                    PhoshMonitorManager    *monitormanager)
{


  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_debug ("Monitor '%s' removed", monitor->name);
  remove_shield_by_monitor (self, monitor);
}


static void
on_monitor_added (PhoshLockscreenManager *self,
                  PhoshMonitor           *monitor,
                  PhoshMonitorManager    *monitormanager)
{
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_debug ("Monitor '%s' added", monitor->name);
  lock_monitor (self, monitor);
}


static void
lock_primary_monitor (PhoshLockscreenManager *self)
{
  PhoshMonitor *primary_monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshShell *shell = phosh_shell_get_default ();

  primary_monitor = phosh_shell_get_primary_monitor (shell);

  /* Undo any transform on the primary display so the keypad becomes usable */
  self->transform = phosh_shell_get_transform (shell);
  phosh_shell_set_transform (shell, PHOSH_MONITOR_TRANSFORM_NORMAL);

  /* The primary output gets the clock, keypad, ... */
  self->lockscreen = PHOSH_LOCKSCREEN (phosh_lockscreen_new (
                                         phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                                         primary_monitor->wl_output));

  g_object_connect (
    self->lockscreen,
    "swapped-object-signal::lockscreen-unlock", G_CALLBACK (lockscreen_unlock_cb), self,
    "swapped-object-signal::wakeup-output", G_CALLBACK (lockscreen_wakeup_output_cb), self,
    NULL);

  gtk_widget_show (GTK_WIDGET (self->lockscreen));
  /* Old lockscreen gets remove due to `layer_surface_closed` */
}


static void
on_primary_monitor_changed (PhoshLockscreenManager *self,
                            GParamSpec *pspec,
                            PhoshShell *shell)
{
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_debug ("primary monitor changed, need to move lockscreen");
  lock_primary_monitor (self);
  /* We don't remove a shield that might exist to avoid the screen
     content flickering in. The shield will be removed on unlock */
}


static void
lockscreen_lock (PhoshLockscreenManager *self)
{
  PhoshMonitor *primary_monitor;
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  g_return_if_fail (!self->locked);

  primary_monitor = phosh_shell_get_primary_monitor (shell);
  g_return_if_fail (primary_monitor);

  /* Listen for monitor changes */
  g_signal_connect_object (monitor_manager, "monitor-added",
                           G_CALLBACK (on_monitor_added),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (monitor_manager, "monitor-removed",
                           G_CALLBACK (on_monitor_removed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (shell,
                           "notify::primary-monitor",
                           G_CALLBACK (on_primary_monitor_changed),
                           self,
                           G_CONNECT_SWAPPED);

  lock_primary_monitor (self);
  /* Lock all other outputs */
  self->shields = g_ptr_array_new_with_free_func ((GDestroyNotify) (gtk_widget_destroy));

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    if (monitor == NULL || monitor == primary_monitor)
      continue;
    lock_monitor (self, monitor);
  }

  self->locked = TRUE;
  self->active_time = g_get_monotonic_time ();
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void
presence_status_changed_cb (PhoshLockscreenManager *self, guint32 status, gpointer *data)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_debug ("Presence status changed: %d", status);
  if (status == GNOME_SESSION_STATUS_IDLE)
    phosh_lockscreen_manager_set_locked (self, TRUE);
}


static void
phosh_lockscreen_manager_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);

  switch (property_id) {
  case PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED:
    phosh_lockscreen_manager_set_locked (self, g_value_get_boolean (value));
    break;
  case PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT:
    phosh_lockscreen_manager_set_timeout (self, g_value_get_int (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_lockscreen_manager_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);

  switch (property_id) {
  case PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED:
    g_value_set_boolean (value, self->locked);
    break;
  case PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT:
    g_value_set_uint (value, self->timeout);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_lockscreen_manager_dispose (GObject *object)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);

  g_clear_pointer (&self->shields, g_ptr_array_unref);
  g_clear_pointer (&self->lockscreen, phosh_cp_widget_destroy);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_lockscreen_manager_parent_class)->dispose (object);
}


static void
phosh_lockscreen_manager_constructed (GObject *object)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);

  G_OBJECT_CLASS (phosh_lockscreen_manager_parent_class)->constructed (object);

  self->settings = g_settings_new ("org.gnome.desktop.session");
  g_settings_bind (self->settings, "idle-delay", self, "timeout", G_SETTINGS_BIND_GET);

  self->presence = phosh_session_presence_get_default_failable ();
  if (self->presence) {
    g_signal_connect_swapped (self->presence,
                              "status-changed",
                              (GCallback) presence_status_changed_cb,
                              self);
  }
}


static void
phosh_lockscreen_manager_class_init (PhoshLockscreenManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_lockscreen_manager_constructed;
  object_class->dispose = phosh_lockscreen_manager_dispose;

  object_class->set_property = phosh_lockscreen_manager_set_property;
  object_class->get_property = phosh_lockscreen_manager_get_property;

  props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED] =
    g_param_spec_boolean ("locked",
                          "Locked",
                          "Whether the screen is locked",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  props[PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT] =
    g_param_spec_int ("timeout",
                      "Timeout",
                      "Idle timeout in seconds until screen locks",
                      0,
                      G_MAXINT,
                      300,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP, props);

  /**
   * PhoshLockscreenManager::wakeup-outputs
   * @self: The #PhoshLockscreenManager emitting this signal
   *
   * Emitted when the outputs should be woken up.
   */
  signals[WAKEUP_OUTPUTS] = g_signal_new (
    "wakeup-outputs",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 0);
}


static void
phosh_lockscreen_manager_init (PhoshLockscreenManager *self)
{
}


PhoshLockscreenManager *
phosh_lockscreen_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_LOCKSCREEN_MANAGER, NULL);
}


void
phosh_lockscreen_manager_set_locked (PhoshLockscreenManager *self, gboolean state)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  if (state == self->locked)
    return;

  if (state)
    lockscreen_lock (self);
  else
    lockscreen_unlock_cb (self, PHOSH_LOCKSCREEN (self->lockscreen));
}


gboolean
phosh_lockscreen_manager_get_locked (PhoshLockscreenManager *self)
{
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), FALSE);

  return self->locked;
}


void
phosh_lockscreen_manager_set_timeout (PhoshLockscreenManager *self, int timeout)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  if (timeout == self->timeout)
    return;

  g_debug ("Setting lock screen idle timeout to %d seconds", timeout);
  self->timeout = timeout;

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT]);
}


int
phosh_lockscreen_manager_get_timeout (PhoshLockscreenManager *self)
{
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), 0);

  return self->timeout;
}


gint64
phosh_lockscreen_manager_get_active_time (PhoshLockscreenManager *self)
{
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), 0);

  return self->active_time;
}
