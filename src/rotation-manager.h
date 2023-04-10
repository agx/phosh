/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "lockscreen-manager.h"
#include "sensor-proxy-manager.h"
#include "monitor/monitor.h"

G_BEGIN_DECLS

/**
 * PhoshRotationManagerMode:
 * @PHOSH_ROTATION_MANAGER_MODE_OFF: automatic rotation off
 * @PHOSH_ROTATION_MANAGER_MODE_SENSOR: rotation driven by sensor orientation
 *
 * The mode of a #PhoshRotationManager
 */
typedef enum {
  PHOSH_ROTATION_MANAGER_MODE_OFF,
  PHOSH_ROTATION_MANAGER_MODE_SENSOR,
} PhoshRotationManagerMode;

#define PHOSH_TYPE_ROTATION_MANAGER (phosh_rotation_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshRotationManager, phosh_rotation_manager, PHOSH, ROTATION_MANAGER, GObject);

PhoshRotationManager *phosh_rotation_manager_new (PhoshSensorProxyManager *sensor_proxy_manager,
                                                  PhoshLockscreenManager  *lockscreen_manager,
                                                  PhoshMonitor            *monitor);
void                  phosh_rotation_manager_set_orientation_locked (PhoshRotationManager *self,
                                                                     gboolean              locked);
gboolean              phosh_rotation_manager_get_orientation_locked (PhoshRotationManager *self);

PhoshRotationManagerMode phosh_rotation_manager_get_mode (PhoshRotationManager *self);
gboolean                 phosh_rotation_manager_set_mode (PhoshRotationManager *self,
                                                          PhoshRotationManagerMode mode);
void                     phosh_rotation_manager_set_transform (PhoshRotationManager  *self,
                                                               PhoshMonitorTransform  transform);
PhoshMonitorTransform    phosh_rotation_manager_get_transform  (PhoshRotationManager *self);
PhoshMonitor            *phosh_rotation_manager_get_monitor    (PhoshRotationManager *self);
void                     phosh_rotation_manager_set_monitor    (PhoshRotationManager *self,
                                                                PhoshMonitor *monitor);

G_END_DECLS
