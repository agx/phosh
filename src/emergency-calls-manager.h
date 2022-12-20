/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#pragma once

#include "call.h"
#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS


#define PHOSH_TYPE_EMERGENCY_CALLS_MANAGER (phosh_emergency_calls_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyCallsManager, phosh_emergency_calls_manager, PHOSH, EMERGENCY_CALLS_MANAGER, PhoshManager)

PhoshEmergencyCallsManager *phosh_emergency_calls_manager_new (void);
GListStore *phosh_emergency_calls_manager_get_list_store (PhoshEmergencyCallsManager *self);
void        phosh_emergency_calls_manager_call (PhoshEmergencyCallsManager *self, const gchar *id);

G_END_DECLS
