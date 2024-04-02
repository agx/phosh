/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_LAUNCHER_ENTRY_MANAGER (phosh_launcher_entry_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLauncherEntryManager, phosh_launcher_entry_manager,
                      PHOSH, LAUNCHER_ENTRY_MANAGER, PhoshManager)

PhoshLauncherEntryManager *phosh_launcher_entry_manager_new (void);

G_END_DECLS
