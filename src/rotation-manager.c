/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-rotation-manager"

#include "config.h"
#include "rotation-manager.h"
#include "shell.h"
#include "sensor-proxy-manager.h"
#include "util.h"

#define ORIENTATION_LOCK_SCHEMA_ID "org.gnome.settings-daemon.peripherals.touchscreen"
#define ORIENTATION_LOCK_KEY       "orientation-lock"

/**
 * SECTION:rotation-manager
 * @short_description: The Rotation Manager
 * @Title: PhoshRotationManager
 *
 * #PhoshRotationManager is responsible for managing the rotation of
 * a given #PhoshMonitor. Depending on the #PhoshRotationManagerMode
 * this can happen by interfacing with a #PhoshSensorProxyManager or
 * by setting the #PhoshMonitorTransform explicitly.
 * It also takes the #PhoshLockscreenManager:locked status into account
 * to ensure the lockscreen is rotated accordingly on small phones.
 */

enum {
  PROP_0,
  PROP_SENSOR_PROXY_MANAGER,
  PROP_LOCKSCREEN_MANAGER,
  PROP_ORIENTATION_LOCKED,
  PROP_MONITOR,
  PROP_MODE,
  PROP_TRANSFORM,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

typedef struct _PhoshRotationManager {
  GObject                  parent;

  gboolean                 claimed;
  PhoshSensorProxyManager *sensor_proxy_manager;
  PhoshLockscreenManager  *lockscreen_manager;
  PhoshMonitor            *monitor;
  PhoshMonitorTransform    transform;
  PhoshMonitorTransform    prelock_transform;

  GSettings               *settings;
  gboolean                 orientation_locked;

  PhoshRotationManagerMode mode;
} PhoshRotationManager;

G_DEFINE_TYPE (PhoshRotationManager, phosh_rotation_manager, G_TYPE_OBJECT);


static void
apply_transform (PhoshRotationManager *self, PhoshMonitorTransform transform)
{
  PhoshMonitorTransform current;
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (phosh_shell_get_default());

  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (monitor_manager));
  g_return_if_fail (PHOSH_IS_MONITOR (self->monitor));

  g_debug ("Rotating %s: %d", self->monitor->name, transform);

  current = phosh_monitor_get_transform (self->monitor);
  if (current == transform)
    return;

  phosh_monitor_manager_set_monitor_transform (monitor_manager,
                                               self->monitor,
                                               transform);
  phosh_monitor_manager_apply_monitor_config (monitor_manager);
}

/**
 * match_orientation:
 * @self: The #PhoshRotationManager
 *
 * Match the screen orientation to the sensor output.
 * Do nothing if orientation lock is on or there's no
 * sensor claimed.
 */
static void
match_orientation (PhoshRotationManager *self)
{
  const gchar *orient;
  PhoshMonitorTransform transform;

  if (self->orientation_locked || !self->claimed ||
      self->mode == PHOSH_ROTATION_MANAGER_MODE_OFF)
    return;

  orient = phosh_dbus_sensor_proxy_get_accelerometer_orientation (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Orientation changed: %s, locked: %d, claimed: %d",
           orient, self->orientation_locked, self->claimed);

  if (!g_strcmp0 ("normal", orient)) {
    transform = PHOSH_MONITOR_TRANSFORM_NORMAL;
  } else if (!g_strcmp0 ("right-up", orient)) {
    transform = PHOSH_MONITOR_TRANSFORM_270;
  } else if (!g_strcmp0 ("bottom-up", orient)) {
    transform = PHOSH_MONITOR_TRANSFORM_180;
  } else if (!g_strcmp0 ("left-up", orient)) {
    transform = PHOSH_MONITOR_TRANSFORM_90;
  } else if (!g_strcmp0 ("undefined", orient)) {
    return; /* just leave as is */
  } else {
    g_warning ("Unknown orientation '%s'", orient);
    return;
  }

  apply_transform (self, transform);
}

static void
on_accelerometer_claimed (PhoshSensorProxyManager *sensor_proxy_manager,
                          GAsyncResult            *res,
                          PhoshRotationManager    *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_if_fail (PHOSH_IS_SENSOR_PROXY_MANAGER (sensor_proxy_manager));
  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (sensor_proxy_manager == self->sensor_proxy_manager);

  success = phosh_dbus_sensor_proxy_call_claim_accelerometer_finish (
    PHOSH_DBUS_SENSOR_PROXY (sensor_proxy_manager),
    res, &err);
  if (success) {
    g_debug ("Claimed accelerometer");
    self->claimed = TRUE;
  } else {
    g_warning ("Failed to claim accelerometer: %s", err->message);
  }
  match_orientation (self);
  g_object_unref (self);
}

static void
on_accelerometer_released (PhoshSensorProxyManager *sensor_proxy_manager,
                           GAsyncResult            *res,
                           PhoshRotationManager    *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_if_fail (PHOSH_IS_SENSOR_PROXY_MANAGER (sensor_proxy_manager));
  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (sensor_proxy_manager == self->sensor_proxy_manager);

  success = phosh_dbus_sensor_proxy_call_release_accelerometer_finish (
    PHOSH_DBUS_SENSOR_PROXY (sensor_proxy_manager),
    res, &err);
  if (success) {
    g_debug ("Released rotation sensor");
  } else {
    g_warning ("Failed to release rotation sensor: %s", err->message);
  }
  self->claimed = FALSE;
  g_object_unref (self);
}

static void
phosh_rotation_manager_claim_accelerometer (PhoshRotationManager *self, gboolean claim)
{
  if (claim == self->claimed)
    return;

  if (!self->sensor_proxy_manager)
    return;

  if (claim) {
    phosh_dbus_sensor_proxy_call_claim_accelerometer (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      NULL,
      (GAsyncReadyCallback)on_accelerometer_claimed,
      g_object_ref (self));
  } else {
    phosh_dbus_sensor_proxy_call_release_accelerometer (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      NULL,
      (GAsyncReadyCallback)on_accelerometer_released,
      g_object_ref (self));
  }
}

static void
on_has_accelerometer_changed (PhoshRotationManager    *self,
                              GParamSpec              *pspec,
                              PhoshSensorProxyManager *proxy)
{
  gboolean has_accel;
  PhoshRotationManagerMode mode;

  has_accel = phosh_dbus_sensor_proxy_get_has_accelerometer (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Found %s accelerometer", has_accel ? "a" : "no");

  mode = has_accel ? PHOSH_ROTATION_MANAGER_MODE_SENSOR : PHOSH_ROTATION_MANAGER_MODE_OFF;
  phosh_rotation_manager_set_mode (self, mode);
}

/**
 * fixup_lockscreen_orientation:
 * @self: The PhoshRotationManager
 * @force: Whether to force the monitor to portait orientation
 *
 * On phones the lock screen doesn't work in landscape so fix that up
 * until https://source.puri.sm/Librem5/phosh/-/issues/388
 * is fixed. Keep all of this local to this function.
 */
static void
fixup_lockscreen_orientation (PhoshRotationManager *self, gboolean force)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshModeManager *mode_manager = phosh_shell_get_mode_manager(shell);

  g_return_if_fail (PHOSH_IS_MODE_MANAGER (mode_manager));
  g_return_if_fail (PHOSH_IS_MONITOR (self->monitor));

  /* Only bother on phones */
  if (phosh_mode_manager_get_device_type(mode_manager) != PHOSH_MODE_DEVICE_TYPE_PHONE &&
      phosh_mode_manager_get_device_type(mode_manager) != PHOSH_MODE_DEVICE_TYPE_UNKNOWN)
    return;

  /* Don't mess with transforms on external screens either */
  if (!phosh_monitor_is_builtin (self->monitor))
    return;

  if (phosh_lockscreen_manager_get_locked (self->lockscreen_manager)) {
    if (force) {
      PhoshMonitorTransform transform;
      /* Use prelock transform if portrait, else use normal */
      transform = (self->prelock_transform % 2) == 0 ? self->prelock_transform :
        PHOSH_MONITOR_TRANSFORM_NORMAL;
      g_debug ("Forcing portrait transform: %d", transform);
      apply_transform (self, transform);
    } else {
      self->prelock_transform = phosh_monitor_get_transform (self->monitor);
      g_debug ("Saving transform %d", self->prelock_transform);
    }
  } else {
    g_debug ("Restoring transform %d", self->prelock_transform);
    apply_transform (self, self->prelock_transform);
  }
}


static void
on_power_mode_changed (PhoshRotationManager *self,
                       GParamSpec *pspec,
                       PhoshMonitor *monitor)
{
  PhoshMonitorPowerSaveMode mode;

  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  mode = phosh_monitor_get_power_save_mode (monitor);
  g_debug ("Mode: %d", mode);
  if (mode != PHOSH_MONITOR_POWER_SAVE_MODE_ON)
    return;

  fixup_lockscreen_orientation (self, TRUE);
}


static void
on_lockscreen_manager_locked (PhoshRotationManager *self, GParamSpec *pspec,
                              PhoshLockscreenManager *lockscreen_manager)
{
  gboolean claim;

  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager));

  if (self->mode == PHOSH_ROTATION_MANAGER_MODE_OFF)
    claim = FALSE;
  else
    claim = !phosh_lockscreen_manager_get_locked (self->lockscreen_manager);

  phosh_rotation_manager_claim_accelerometer (self, claim);

  fixup_lockscreen_orientation (self, FALSE);
}

static void
on_accelerometer_orientation_changed (PhoshRotationManager    *self,
                                      GParamSpec              *pspec,
                                      PhoshSensorProxyManager *sensor)
{
  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (self->sensor_proxy_manager == sensor);

  match_orientation (self);
}


static void
on_monitor_configured (PhoshRotationManager   *self,
                       PhoshMonitor           *monitor)
{
  PhoshMonitorTransform transform;

  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  transform = phosh_monitor_get_transform (monitor);
  if (transform == self->transform)
    return;

  self->transform = transform;
  g_debug ("Rotation-manager transform %d", transform);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSFORM]);
}


static void
phosh_rotation_manager_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  PhoshRotationManager *self = PHOSH_ROTATION_MANAGER (object);

  switch (property_id) {
  case PROP_SENSOR_PROXY_MANAGER:
    /* construct only */
    self->sensor_proxy_manager = g_value_dup_object (value);
    break;
  case PROP_LOCKSCREEN_MANAGER:
    /* construct only */
    self->lockscreen_manager = g_value_dup_object (value);
    break;
  case PROP_MONITOR:
    /* construct only */
    self->monitor = g_value_dup_object (value);
    break;
  case PROP_ORIENTATION_LOCKED:
    phosh_rotation_manager_set_orientation_locked (self,
                                                   g_value_get_boolean (value));
    break;
  case PROP_MODE:
    phosh_rotation_manager_set_mode (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_rotation_manager_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PhoshRotationManager *self = PHOSH_ROTATION_MANAGER (object);

  switch (property_id) {
  case PROP_SENSOR_PROXY_MANAGER:
    g_value_set_object (value, self->sensor_proxy_manager);
    break;
  case PROP_LOCKSCREEN_MANAGER:
    g_value_set_object (value, self->lockscreen_manager);
    break;
  case PROP_ORIENTATION_LOCKED:
    g_value_set_boolean (value, self->orientation_locked);
    break;
  case PROP_MODE:
    g_value_set_enum (value, self->mode);
    break;
  case PROP_TRANSFORM:
    g_value_set_enum (value, self->transform);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_rotation_manager_constructed (GObject *object)
{
  PhoshRotationManager *self = PHOSH_ROTATION_MANAGER (object);

  G_OBJECT_CLASS (phosh_rotation_manager_parent_class)->constructed (object);

  self->settings = g_settings_new (ORIENTATION_LOCK_SCHEMA_ID);

  g_settings_bind (self->settings,
                   ORIENTATION_LOCK_KEY,
                   self,
                   "orientation-locked",
                   G_BINDING_SYNC_CREATE
                   | G_BINDING_BIDIRECTIONAL);

  g_signal_connect_swapped (self->lockscreen_manager,
                            "notify::locked",
                            (GCallback) on_lockscreen_manager_locked,
                            self);
  on_lockscreen_manager_locked (self, NULL, self->lockscreen_manager);

  g_signal_connect_swapped (self->monitor,
                            "notify::power-mode",
                            (GCallback) on_power_mode_changed,
                            self);
  on_power_mode_changed (self, NULL, self->monitor);

  g_signal_connect_swapped (self->monitor,
                            "configured",
                            G_CALLBACK (on_monitor_configured),
                            self);
  on_monitor_configured (self, self->monitor);

  if (!self->sensor_proxy_manager) {
    g_warning ("Got not sensor-proxy, no automatic rotation");
    return;
  }

  g_signal_connect_swapped (self->sensor_proxy_manager,
                            "notify::accelerometer-orientation",
                            (GCallback) on_accelerometer_orientation_changed,
                            self);

  g_signal_connect_swapped (self->sensor_proxy_manager,
                            "notify::has-accelerometer",
                            (GCallback) on_has_accelerometer_changed,
                            self);
  on_has_accelerometer_changed (self, NULL, self->sensor_proxy_manager);
}


static void
phosh_rotation_manager_dispose (GObject *object)
{
  PhoshRotationManager *self = PHOSH_ROTATION_MANAGER (object);

  g_clear_object (&self->settings);

  if (self->sensor_proxy_manager) {
    g_signal_handlers_disconnect_by_data (self->sensor_proxy_manager,
                                          self);
    /* Sync call since we're going away */
    phosh_dbus_sensor_proxy_call_release_accelerometer_sync (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager), NULL, NULL);
    g_clear_object (&self->sensor_proxy_manager);
  }

  if (self->lockscreen_manager) {
    g_signal_handlers_disconnect_by_data (self->lockscreen_manager,
                                          self);
    g_clear_object (&self->lockscreen_manager);
  }

  if (self->monitor) {
    g_signal_handlers_disconnect_by_data (self->monitor,
                                          self);
    g_clear_object (&self->monitor);
  }

  G_OBJECT_CLASS (phosh_rotation_manager_parent_class)->dispose (object);
}

static void
phosh_rotation_manager_class_init (PhoshRotationManagerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_rotation_manager_constructed;
  object_class->dispose = phosh_rotation_manager_dispose;

  object_class->set_property = phosh_rotation_manager_set_property;
  object_class->get_property = phosh_rotation_manager_get_property;

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

  props[PROP_MONITOR] =
    g_param_spec_object (
      "monitor",
      "Monitor",
      "The monitor to rotate",
      PHOSH_TYPE_MONITOR,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_ORIENTATION_LOCKED] =
    g_param_spec_boolean (
      "orientation-locked",
      "Screen orientation locked",
      "Whether the screen orientation is locked",
      TRUE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_MODE] =
    g_param_spec_enum (
      "mode",
      "Rotation mode",
      "The current rotation mode",
      PHOSH_TYPE_ROTATION_MANAGER_MODE,
      PHOSH_ROTATION_MANAGER_MODE_OFF,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_TRANSFORM] =
    g_param_spec_enum ("transform",
                       "Transform",
                       "Monitor transform of the rotation monitor",
                       PHOSH_TYPE_MONITOR_TRANSFORM,
                       PHOSH_MONITOR_TRANSFORM_NORMAL,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
phosh_rotation_manager_init (PhoshRotationManager *self)
{
}


PhoshRotationManager *
phosh_rotation_manager_new (PhoshSensorProxyManager *sensor_proxy_manager,
                            PhoshLockscreenManager  *lockscreen_manager,
                            PhoshMonitor *monitor)
{
  return g_object_new (PHOSH_TYPE_ROTATION_MANAGER,
                       "sensor-proxy-manager", sensor_proxy_manager,
                       "lockscreen-manager", lockscreen_manager,
                       "monitor", monitor,
                       NULL);
}

void
phosh_rotation_manager_set_orientation_locked (PhoshRotationManager *self, gboolean locked)
{
  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));

  if (locked == self->orientation_locked)
    return;

  self->orientation_locked = locked;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ORIENTATION_LOCKED]);
  match_orientation (self);
}

gboolean
phosh_rotation_manager_get_orientation_locked (PhoshRotationManager *self)
{
  g_return_val_if_fail (PHOSH_IS_ROTATION_MANAGER (self), TRUE);

  return self->orientation_locked;
}

PhoshRotationManagerMode
phosh_rotation_manager_get_mode (PhoshRotationManager *self)
{
  g_return_val_if_fail (PHOSH_IS_ROTATION_MANAGER (self), PHOSH_ROTATION_MANAGER_MODE_OFF);

  return self->mode;
}

/**
 * phosh_rotation_manager_set_mode:
 * @self: The #PhoshRotationManager
 * @mode: The #PhoshRotationManagerMode to set
 *
 * Sets the given mode.
 * Returns: %TRUE if setting the mode was possible, otherwise %FALSE (e.g. when trying
 * to set %PHOSH_ROTATION_MANAGER_MODE_SENSOR without having a sensor.
 */
gboolean
phosh_rotation_manager_set_mode (PhoshRotationManager *self, PhoshRotationManagerMode mode)
{
  gboolean has_accel;

  g_return_val_if_fail (PHOSH_IS_ROTATION_MANAGER (self), FALSE);

  if (mode == self->mode)
    return TRUE;

  has_accel = phosh_dbus_sensor_proxy_get_has_accelerometer (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  if (mode == PHOSH_ROTATION_MANAGER_MODE_SENSOR && !has_accel)
    return FALSE;

  self->mode = mode;

  g_debug ("Setting mode: %d", mode);
  switch (mode) {
  case PHOSH_ROTATION_MANAGER_MODE_OFF:
    phosh_rotation_manager_claim_accelerometer (self, FALSE);
    break;
  case PHOSH_ROTATION_MANAGER_MODE_SENSOR:
    /* Don't claim during screen lock, enables runtime pm and will be
       claimed on unlock */
    if (!phosh_lockscreen_manager_get_locked (self->lockscreen_manager))
      phosh_rotation_manager_claim_accelerometer (self, TRUE);
    break;
  default:
    g_assert_not_reached ();
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MODE]);
  return TRUE;
}


void
phosh_rotation_manager_set_transform (PhoshRotationManager   *self,
                                      PhoshMonitorTransform  transform)
{
  g_return_if_fail (PHOSH_IS_ROTATION_MANAGER (self));
  g_return_if_fail (self->mode == PHOSH_ROTATION_MANAGER_MODE_OFF);

  apply_transform (self, transform);
}

PhoshMonitorTransform
phosh_rotation_manager_get_transform (PhoshRotationManager *self)
{
  g_return_val_if_fail (PHOSH_IS_ROTATION_MANAGER (self),
                        PHOSH_MONITOR_TRANSFORM_NORMAL);

  return self->monitor->transform;
}

/**
 * phosh_rotation_manager_get_monitor:
 * @self: The PhoshRotationManager
 *
 * Returns: The #PhoshMonitor this manager acts on
 */
PhoshMonitor *
phosh_rotation_manager_get_monitor (PhoshRotationManager *self)
{
  g_return_val_if_fail (PHOSH_IS_ROTATION_MANAGER (self), NULL);

  return self->monitor;
}
