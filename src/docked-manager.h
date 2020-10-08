/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <mode-manager.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_DOCKED_MANAGER (phosh_docked_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshDockedManager, phosh_docked_manager, PHOSH, DOCKED_MANAGER, GObject)

PhoshDockedManager *phosh_docked_manager_new (PhoshModeManager *mode_manager);
const char *phosh_docked_manager_get_icon_name (PhoshDockedManager *self);
gboolean    phosh_docked_manager_get_enabled (PhoshDockedManager *self);
gboolean    phosh_docked_manager_get_can_dock (PhoshDockedManager *self);
void        phosh_docked_manager_set_enabled (PhoshDockedManager *self, gboolean enabled);

G_END_DECLS
