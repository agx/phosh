/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

/**
 * PhoshRotateInfoMode:
 * @PHOSH_ROTATE_INFO_MODE_LOCK: Button toggles rotation lock
 * @PHOSH_ROTATE_INFO_MODE_TOGGLE: Button toggles portrait/landscape
 *
 * What is toggled when short pressing the rotation info quick setting
 */
typedef enum {
  PHOSH_ROTATE_INFO_MODE_LOCK,
  PHOSH_ROTATE_INFO_MODE_TOGGLE,
} PhoshRotateInfoMode;

#define PHOSH_TYPE_ROTATE_INFO (phosh_rotate_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshRotateInfo, phosh_rotate_info, PHOSH, ROTATE_INFO, PhoshStatusIcon)

GtkWidget           *phosh_rotate_info_new (void);
PhoshRotateInfoMode  phosh_rotate_info_get_mode (PhoshRotateInfo *self);
void                 phosh_rotate_info_set_mode (PhoshRotateInfo *self, PhoshRotateInfoMode mode);

G_END_DECLS
