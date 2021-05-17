/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <manager.h>

#include <glib-object.h>


G_BEGIN_DECLS

/**
 * PhoshModeDeviceType:
 * @PHOSH_MODE_DEVICE_TYPE_UNKNOWN: unknown device type
 * @PHOSH_MODE_DEVICE_TYPE_PHONE: a phone/handset
 * @PHOSH_MODE_DEVICE_TYPE_LAPTOP: a laptop
 * @PHOSH_MODE_DEVICE_TYPE_DESKTOP: a desktop computer
 * @PHOSH_MODE_DEVICE_TYPE_TABLET: a tablet computer
 * @PHOSH_MODE_DEVICE_TYPE_CONVERTIBLE: a convertible
 *
 * A type of device
 */
typedef enum {
  PHOSH_MODE_DEVICE_TYPE_UNKNOWN,
  PHOSH_MODE_DEVICE_TYPE_PHONE,
  PHOSH_MODE_DEVICE_TYPE_LAPTOP,
  PHOSH_MODE_DEVICE_TYPE_DESKTOP,
  PHOSH_MODE_DEVICE_TYPE_TABLET,
  PHOSH_MODE_DEVICE_TYPE_CONVERTIBLE,
} PhoshModeDeviceType;

/**
 * PhoshModeHwFlags:
 * @PHOSH_MODE_HW_NONE: nothing
 * @PHOSH_MODE_HW_EXT_DISPLAY: external display
 * @PHOSH_MODE_HW_KEYBOARD: keyboard
 * @PHOSH_MODE_HW_POINTER: pointing device
 *
 * Attached external hardware
 */
typedef enum {
  PHOSH_MODE_HW_NONE        = 0,
  PHOSH_MODE_HW_EXT_DISPLAY = (1 << 1),
  PHOSH_MODE_HW_KEYBOARD    = (1 << 2),
  PHOSH_MODE_HW_POINTER     = (1 << 3),
} PhoshModeHwFlags;

/* TODO: keyboard is hard to detect due to gpio keys, etc */
#define PHOSH_MODE_DOCKED_PHONE_MASK (PHOSH_MODE_HW_EXT_DISPLAY \
                                      | PHOSH_MODE_HW_POINTER)
#define PHOSH_MODE_DOCKED_TABLET_MASK (PHOSH_MODE_HW_POINTER)

#define PHOSH_TYPE_MODE_MANAGER (phosh_mode_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshModeManager, phosh_mode_manager, PHOSH, MODE_MANAGER, PhoshManager)

PhoshModeManager *phosh_mode_manager_new (void);
PhoshModeDeviceType phosh_mode_manager_get_device_type (PhoshModeManager *self);
PhoshModeDeviceType phosh_mode_manager_get_mimicry (PhoshModeManager *self);

G_END_DECLS
