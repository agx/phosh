/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CELL_BROADCAST_MANAGER (phosh_cell_broadcast_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCellBroadcastManager, phosh_cell_broadcast_manager,
                      PHOSH, CELL_BROADCAST_MANAGER, GObject)

PhoshCellBroadcastManager *phosh_cell_broadcast_manager_new (void);

G_END_DECLS
