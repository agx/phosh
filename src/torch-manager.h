/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_TORCH_MANAGER (phosh_torch_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTorchManager, phosh_torch_manager, PHOSH, TORCH_MANAGER, PhoshManager)

PhoshTorchManager *phosh_torch_manager_new (void);
const char        *phosh_torch_manager_get_icon_name (PhoshTorchManager *self);
gboolean           phosh_torch_manager_get_enabled (PhoshTorchManager *self);
gboolean           phosh_torch_manager_get_present (PhoshTorchManager *self);
void               phosh_torch_manager_toggle (PhoshTorchManager *self);
gboolean           phosh_torch_manager_get_can_scale (PhoshTorchManager *self);
int                phosh_torch_manager_get_brightness (PhoshTorchManager *self);
double             phosh_torch_manager_get_scaled_brightness (PhoshTorchManager *self);
void               phosh_torch_manager_set_scaled_brightness (PhoshTorchManager *self, double frac);
int                phosh_torch_manager_get_max_brightness (PhoshTorchManager *self);

G_END_DECLS
