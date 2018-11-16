/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen-manager"

#include "lockscreen-manager.h"
#include "lockscreen.h"
#include "lockshield.h"
#include "shell.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "phosh-wayland.h"
#include "util.h"

#include <gdk/gdkwayland.h>


enum {
  PHOSH_LOCKSCREEN_MANAGER_PROP_0,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED,
  PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP];


typedef struct {
  GObject parent;

  PhoshLockscreen *lockscreen;   /* phone display lock screen */
  GPtrArray *shields;            /* other outputs */
  gulong unlock_handler_id;
  struct org_kde_kwin_idle_timeout *lock_timer;
  struct zwlr_input_inhibitor_v1 *input_inhibitor;
  GSettings *settings;

  gint timeout;                  /* timeout in seconds before screen locks */
  gboolean locked;
} PhoshLockscreenManagerPrivate;


typedef struct _PhoshLockscreenManager {
  GObject parent;
} PhoshLockscreenManager;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreenManager, phosh_lockscreen_manager, G_TYPE_OBJECT)


static void
lockscreen_unlock_cb (PhoshLockscreenManager *self, PhoshLockscreen *lockscreen)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (lockscreen));
  g_return_if_fail (lockscreen == PHOSH_LOCKSCREEN (priv->lockscreen));

  if (priv->unlock_handler_id) {
    g_signal_handler_disconnect (lockscreen, priv->unlock_handler_id);
    priv->unlock_handler_id = 0;
  }
  g_clear_pointer (&priv->lockscreen, phosh_cp_widget_destroy);

  /* Unlock all other outputs */
  g_clear_pointer (&priv->shields, g_ptr_array_unref);

  zwlr_input_inhibitor_v1_destroy(priv->input_inhibitor);
  priv->input_inhibitor = NULL;

  priv->locked = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void
lockscreen_lock (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  PhoshMonitor *primary_monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  primary_monitor = phosh_shell_get_primary_monitor (shell);
  g_return_if_fail (primary_monitor);

  /* The primary output gets the clock, keypad, ... */
  priv->lockscreen = PHOSH_LOCKSCREEN (phosh_lockscreen_new (
                                         phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                         primary_monitor->wl_output));

  priv->input_inhibitor =
    zwlr_input_inhibit_manager_v1_get_inhibitor(
      phosh_wayland_get_zwlr_input_inhibit_manager_v1 (wl));

  /* Lock all other outputs */
  priv->shields = g_ptr_array_new_with_free_func ((GDestroyNotify) (gtk_widget_destroy));

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    if (monitor == NULL || monitor == primary_monitor)
      continue;
    g_ptr_array_add (priv->shields, phosh_lockshield_new (
                       phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                       monitor->wl_output));
  }

  priv->unlock_handler_id = g_signal_connect_swapped (
    priv->lockscreen,
    "lockscreen-unlock",
    G_CALLBACK(lockscreen_unlock_cb),
    self);

  priv->locked = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void
lock_idle_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
  PhoshLockscreenManager *self = data;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (data));
  phosh_lockscreen_manager_set_locked (self, TRUE);
}


static void
lock_resume_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
}


static const struct org_kde_kwin_idle_timeout_listener idle_timer_listener = {
  .idle = lock_idle_cb,
  .resumed = lock_resume_cb,
};


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
    if (priv->unlock_handler_id) {
      g_signal_handler_disconnect (priv->lockscreen, priv->unlock_handler_id);
      priv->unlock_handler_id = 0;
    }
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

  priv->settings = g_settings_new ("org.gnome.desktop.session");
  g_settings_bind (priv->settings, "idle-delay", self, "timeout", G_SETTINGS_BIND_GET);
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
phosh_lockscreen_manager_set_timeout (PhoshLockscreenManager *self, gint timeout)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default ();
  struct org_kde_kwin_idle *idle_manager = phosh_wayland_get_org_kde_kwin_idle (wl);

  g_return_if_fail (idle_manager);
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  if (timeout == priv->timeout)
    return;

  g_debug("Setting lock screen idle timeout to %d seconds", timeout);
  priv->timeout = timeout;

  if (priv->lock_timer) {
    org_kde_kwin_idle_timeout_release (priv->lock_timer);
  }
  /* Rearm the timer */
  priv->lock_timer = org_kde_kwin_idle_get_idle_timeout(idle_manager,
                                                        phosh_wayland_get_wl_seat (wl),
                                                        priv->timeout * 1000);
  g_return_if_fail (priv->lock_timer);
  org_kde_kwin_idle_timeout_add_listener(priv->lock_timer,
                                         &idle_timer_listener,
                                         self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_TIMEOUT]);
}


gint
phosh_lockscreen_manager_get_timeout (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), 0);
  return priv->timeout;
}
