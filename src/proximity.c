/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-proximity"

#include "config.h"
#include "fader.h"
#include "proximity.h"
#include "shell.h"
#include "sensor-proxy-manager.h"
#include "util.h"

/**
 * SECTION:proximity
 * @short_description: Proximity sensor handling
 * @Title: PhoshProximity
 */


enum {
  PROP_0,
  PROP_SENSOR_PROXY_MANAGER,
  PROP_LOCKSCREEN_MANAGER,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

typedef struct _PhoshProximity {
  GObject parent;

  gboolean claimed;
  PhoshSensorProxyManager *sensor_proxy_manager;
  PhoshLockscreenManager *lockscreen_manager;
  PhoshFader *fader;
} PhoshProximity;

G_DEFINE_TYPE (PhoshProximity, phosh_proximity, G_TYPE_OBJECT);


static void
on_proximity_claimed (PhoshSensorProxyManager *sensor_proxy_manager,
                      GAsyncResult            *res,
                      PhoshProximity          *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_if_fail (PHOSH_IS_SENSOR_PROXY_MANAGER (sensor_proxy_manager));
  g_return_if_fail (PHOSH_IS_PROXIMITY (self));
  g_return_if_fail (sensor_proxy_manager == self->sensor_proxy_manager);

  success = phosh_dbus_sensor_proxy_call_claim_proximity_finish (
    PHOSH_DBUS_SENSOR_PROXY(sensor_proxy_manager),
    res, &err);
  if (success) {
    g_debug ("Claimed proximity sensor");
    self->claimed = TRUE;
  } else {
    g_warning ("Failed to claim proximity sensor: %s", err->message);
  }
}

static void
on_proximity_released (PhoshSensorProxyManager *sensor_proxy_manager,
                       GAsyncResult            *res,
                       PhoshProximity          *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_if_fail (PHOSH_IS_SENSOR_PROXY_MANAGER (sensor_proxy_manager));
  g_return_if_fail (PHOSH_IS_PROXIMITY (self));
  g_return_if_fail (sensor_proxy_manager == self->sensor_proxy_manager);

  success = phosh_dbus_sensor_proxy_call_release_proximity_finish (
    PHOSH_DBUS_SENSOR_PROXY(sensor_proxy_manager),
    res, &err);
  if (success) {
    g_debug ("Released proximity sensor");
    self->claimed = FALSE;
  } else {
    g_warning ("Failed to release proximity sensor: %s", err->message);
  }
  g_clear_pointer (&self->fader, phosh_cp_widget_destroy);
}

static void
phosh_proximity_claim_proximity (PhoshProximity *self, gboolean claim)
{
  if (claim == self->claimed)
    return;

  if (claim) {
    phosh_dbus_sensor_proxy_call_claim_proximity (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      NULL,
      (GAsyncReadyCallback)on_proximity_claimed,
      self);
  } else {
    phosh_dbus_sensor_proxy_call_release_proximity (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      NULL,
      (GAsyncReadyCallback)on_proximity_released,
      self);
  }
}


static void
on_has_proximity_changed (PhoshProximity          *self,
                          GParamSpec              *pspec,
                          PhoshSensorProxyManager *proxy)
{
  gboolean has_proximity;

  /* Don't claim if locked to save power */
  if (phosh_lockscreen_manager_get_locked(self->lockscreen_manager))
    return;

  has_proximity = phosh_dbus_sensor_proxy_get_has_proximity (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Found %s proximity sensor", has_proximity ? "a" : "no");
  phosh_proximity_claim_proximity (self, has_proximity);
}

static void
on_lockscreen_manager_locked (PhoshProximity *self, GParamSpec *pspec,
                              PhoshLockscreenManager *lockscreen_manager)
{
  gboolean locked;

  g_return_if_fail (PHOSH_IS_PROXIMITY (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager));

  locked = phosh_lockscreen_manager_get_locked(self->lockscreen_manager);
  phosh_proximity_claim_proximity (self, !locked);
}


static void
on_proximity_near_changed (PhoshProximity          *self,
                           GParamSpec              *pspec,
                           PhoshSensorProxyManager *sensor)
{
  gboolean near;
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitor *monitor = phosh_shell_get_builtin_monitor (shell);

  near = phosh_dbus_sensor_proxy_get_proximity_near (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Proximity near changed: %d", near);
  if (near && monitor) {
    if (!self->fader) {
      self->fader = g_object_new (PHOSH_TYPE_FADER,
                                  "monitor", monitor,
                                  "style-class", "phosh-fader-proximity-fade",
                                  NULL);
      gtk_widget_show (GTK_WIDGET (self->fader));
    }
  } else {
      g_clear_pointer (&self->fader, phosh_cp_widget_destroy);
  }
}

static void
phosh_proximity_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  PhoshProximity *self = PHOSH_PROXIMITY (object);

  switch (property_id) {
    case PROP_SENSOR_PROXY_MANAGER:
      /* construct only */
      self->sensor_proxy_manager = g_value_dup_object (value);
      break;
    case PROP_LOCKSCREEN_MANAGER:
      /* construct only */
      self->lockscreen_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_proximity_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  PhoshProximity *self = PHOSH_PROXIMITY (object);

  switch (property_id) {
  case PROP_SENSOR_PROXY_MANAGER:
    g_value_set_object (value, self->sensor_proxy_manager);
    break;
  case PROP_LOCKSCREEN_MANAGER:
    g_value_set_object (value, self->lockscreen_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_proximity_constructed (GObject *object)
{
  PhoshProximity *self = PHOSH_PROXIMITY (object);

  g_signal_connect_swapped (self->lockscreen_manager,
                            "notify::locked",
                            G_CALLBACK (on_lockscreen_manager_locked),
                            self);

  g_signal_connect_swapped (self->sensor_proxy_manager,
                            "notify::proximity-near",
                            (GCallback) on_proximity_near_changed,
                            self);

  g_signal_connect_swapped (self->sensor_proxy_manager,
                            "notify::has-proximity",
                            (GCallback) on_has_proximity_changed,
                            self);
  on_has_proximity_changed (self, NULL, self->sensor_proxy_manager);

  G_OBJECT_CLASS (phosh_proximity_parent_class)->constructed (object);
}

static void
phosh_proximity_dispose (GObject *object)
{
  PhoshProximity *self = PHOSH_PROXIMITY (object);

  if (self->sensor_proxy_manager) {
    g_signal_handlers_disconnect_by_data (self->sensor_proxy_manager,
                                          self);
    phosh_dbus_sensor_proxy_call_release_proximity_sync (
      PHOSH_DBUS_SENSOR_PROXY(self->sensor_proxy_manager), NULL, NULL);
    g_clear_object (&self->sensor_proxy_manager);
  }

  if (self->lockscreen_manager) {
     g_signal_handlers_disconnect_by_data (self->lockscreen_manager,
                                           self);
     g_clear_object (&self->lockscreen_manager);
  }

  g_clear_pointer (&self->fader, phosh_cp_widget_destroy);
  G_OBJECT_CLASS (phosh_proximity_parent_class)->dispose (object);
}

static void
phosh_proximity_class_init (PhoshProximityClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_proximity_constructed;
  object_class->dispose = phosh_proximity_dispose;

  object_class->set_property = phosh_proximity_set_property;
  object_class->get_property = phosh_proximity_get_property;

  props[PROP_SENSOR_PROXY_MANAGER] =
    g_param_spec_object (
      "sensor-proxy-manager",
      "Sensor proxy manager",
      "The object inerfacing with iio-sensor-proxy",
      PHOSH_TYPE_SENSOR_PROXY_MANAGER,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_LOCKSCREEN_MANAGER] =
    g_param_spec_object (
      "lockscreen-manager",
      "Lockscren manager",
      "The object managing the lock screen",
      PHOSH_TYPE_LOCKSCREEN_MANAGER,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

}

static void
phosh_proximity_init (PhoshProximity *self)
{
}


PhoshProximity *
phosh_proximity_new (PhoshSensorProxyManager *sensor_proxy_manager,
                     PhoshLockscreenManager *lockscreen_manager)
{
  return g_object_new (PHOSH_TYPE_PROXIMITY,
                       "sensor-proxy-manager", sensor_proxy_manager,
                       "lockscreen-manager", lockscreen_manager,
                       NULL);
}
