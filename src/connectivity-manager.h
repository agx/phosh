/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CONNECTIVITY_MANAGER (phosh_connectivity_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshConnectivityManager, phosh_connectivity_manager, PHOSH,
                      CONNECTIVITY_MANAGER, PhoshManager)

PhoshConnectivityManager *phosh_connectivity_manager_new (void);

G_END_DECLS
