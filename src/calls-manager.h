/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CALLS_MANAGER (phosh_calls_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCallsManager, phosh_calls_manager, PHOSH, CALLS_MANAGER, PhoshManager)

PhoshCallsManager *phosh_calls_manager_new (void);
gboolean phosh_calls_manager_get_present (PhoshCallsManager *self);
gboolean phosh_calls_manager_get_incoming (PhoshCallsManager *self);
const char *phosh_calls_manager_get_active_call (PhoshCallsManager *self);

G_END_DECLS
