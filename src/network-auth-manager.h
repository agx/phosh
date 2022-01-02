/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_NETWORK_AUTH_MANAGER (phosh_network_auth_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshNetworkAuthManager, phosh_network_auth_manager, PHOSH, NETWORK_AUTH_MANAGER, GObject)

PhoshNetworkAuthManager  *phosh_network_auth_manager_new (void);

G_END_DECLS
