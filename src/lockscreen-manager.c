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


typedef struct {
  PhoshLockscreen *lockscreen;     /* phone display lock screen */
  PhoshSessionPresence *presence;  /* gnome-session's presence interface */
  GPtrArray *shields;              /* other outputs */
  GSettings *settings;

  int timeout;                     /* timeout in seconds before screen locks */
  gboolean locked;
  gint64 active_time;              /* when lock was activated (in us) */
  int transform;                   /* the shell transform before locking */
} PhoshLockscreenManagerPrivate;


typedef struct _PhoshLockscreenManager {
  GObject parent;
} PhoshLockscreenManager;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreenManager, phosh_lockscreen_manager, G_TYPE_OBJECT)


static void
lockscreen_unlock_cb (PhoshLockscreenManager *self, PhoshLockscreen *lockscreen)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  phosh_shell_set_transform (shell, priv->transform);
  priv->transform = PHOSH_MONITOR_TRANSFORM_NORMAL;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (lockscreen));
  g_return_if_fail (lockscreen == PHOSH_LOCKSCREEN (priv->lockscreen));

  g_signal_handlers_disconnect_by_data (lockscreen, self);
  g_signal_handlers_disconnect_by_data (monitor_manager, self);
  g_clear_pointer (&priv->lockscreen, phosh_cp_widget_destroy);

  /* Unlock all other outputs */
  g_clear_pointer (&priv->shields, g_ptr_array_unref);

  priv->locked = FALSE;
  priv->active_time = 0;
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


/* Lock a particular monitor bringing up a shield */
static void
lock_monitor (PhoshLockscreenManager *self,
              PhoshMonitor           *monitor)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default ();
  GtkWidget *shield;

  shield = phosh_lockshield_new (
    phosh_wayland_get_zwlr_layer_shell_v1 (wl),
    monitor->wl_output);

  g_ptr_array_add (priv->shields, shield);
  gtk_widget_show (shield);
}


static void
on_monitor_removed (PhoshLockscreenManager *self,
                    PhoshMonitor           *monitor,
                    PhoshMonitorManager    *monitormanager)
{
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_debug ("Monitor removed");
  /* TODO: We just leave the widget dangling, it will be destroyed on
   * unlock */
}


static void
on_monitor_added (PhoshLockscreenManager *self,
                  PhoshMonitor           *monitor,
                  PhoshMonitorManager    *monitormanager)
{
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));

  g_warning ("Monitor added");
  lock_monitor (self, monitor);
}


static void
lockscreen_lock (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  PhoshMonitor *primary_monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  g_return_if_fail (!priv->locked);

  primary_monitor = phosh_shell_get_primary_monitor (shell);
  g_return_if_fail (primary_monitor);

  /* Undo any transform on the primary display so the keypad becomes usable */
  priv->transform = phosh_shell_get_transform (shell);
  phosh_shell_set_transform (shell, PHOSH_MONITOR_TRANSFORM_NORMAL);

  /* Listen for monitor changes */
  g_signal_connect_object (monitor_manager, "monitor-added",
                           G_CALLBACK (on_monitor_added),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (monitor_manager, "monitor-removed",
                           G_CALLBACK (on_monitor_removed),
                           self,
                           G_CONNECT_SWAPPED);

  /* The primary output gets the clock, keypad, ... */
  priv->lockscreen = PHOSH_LOCKSCREEN (phosh_lockscreen_new (
                                         phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                         primary_monitor->wl_output));
  gtk_widget_show (GTK_WIDGET (priv->lockscreen));

  /* Lock all other outputs */
  priv->shields = g_ptr_array_new_with_free_func ((GDestroyNotify) (gtk_widget_destroy));

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    if (monitor == NULL || monitor == primary_monitor)
      continue;
    lock_monitor (self, monitor);
  }

  g_object_connect (
    priv->lockscreen,
    "swapped-object-signal::lockscreen-unlock", G_CALLBACK(lockscreen_unlock_cb), self,
    "swapped-object-signal::wakeup-output", G_CALLBACK(lockscreen_wakeup_output_cb), self,
    NULL);

  priv->locked = TRUE;
  priv->active_time = g_get_monotonic_time ();
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
phosh_lockscreen_manager_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
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
phosh_lockscreen_manager_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private(self);

  switch (property_id) {
  case PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED:
    g_value_set_boolean (value, priv->locked);
    break;
  case PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT:
    g_value_set_uint (value, priv->timeout);
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
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_clear_pointer (&priv->shields, g_ptr_array_unref);
  if (priv->lockscreen) {
    g_signal_handlers_disconnect_by_data (priv->lockscreen, self);
    g_clear_pointer (&priv->lockscreen, phosh_cp_widget_destroy);
  }
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (phosh_lockscreen_manager_parent_class)->dispose (object);
}


static void
phosh_lockscreen_manager_constructed (GObject *object)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  G_OBJECT_CLASS (phosh_lockscreen_manager_parent_class)->constructed (object);

  priv->settings = g_settings_new ("org.gnome.desktop.session");
  g_settings_bind (priv->settings, "idle-delay", self, "timeout", G_SETTINGS_BIND_GET);

  priv->presence = phosh_session_presence_get_default_failable ();
  if (priv->presence) {
    g_signal_connect_swapped (priv->presence,
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
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  if (state == priv->locked)
    return;

  if (state)
    lockscreen_lock (self);
  else
    lockscreen_unlock_cb (self, PHOSH_LOCKSCREEN (priv->lockscreen));
}


gboolean
phosh_lockscreen_manager_get_locked (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), FALSE);
  return priv->locked;
}


void
phosh_lockscreen_manager_set_timeout (PhoshLockscreenManager *self, int timeout)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  if (timeout == priv->timeout)
    return;

  g_debug("Setting lock screen idle timeout to %d seconds", timeout);
  priv->timeout = timeout;

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT]);
}


int
phosh_lockscreen_manager_get_timeout (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), 0);
  return priv->timeout;
}


gint64
phosh_lockscreen_manager_get_active_time (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), 0);
  return priv->active_time;
}
