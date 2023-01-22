/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_SUSPEND_MANAGER (phosh_suspend_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshSuspendManager, phosh_suspend_manager, PHOSH, SUSPEND_MANAGER, PhoshManager)

PhoshSuspendManager *phosh_suspend_manager_new (void);

G_END_DECLS
