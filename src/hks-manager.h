/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>


G_BEGIN_DECLS

/**
 * PhoshHksDeviceType:
 * @PHOSH_HKS_TYPE_MIC: Microphone hardware kill switch
 *
 * Keep in sync with kernels rfkill types
 */
typedef enum {
  PHOSH_HKS_TYPE_MIC = 10,
} PhoshHksDeviceType;


#define PHOSH_TYPE_HKS_MANAGER (phosh_hks_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshHksManager, phosh_hks_manager, PHOSH, HKS_MANAGER, GObject)

PhoshHksManager *phosh_hks_manager_new (void);

G_END_DECLS
