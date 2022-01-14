/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_VPN_MANAGER (phosh_vpn_manager_get_type())

G_DECLARE_FINAL_TYPE (PhoshVpnManager, phosh_vpn_manager, PHOSH, VPN_MANAGER, GObject)

PhoshVpnManager  *phosh_vpn_manager_new                    (void);
const char       *phosh_vpn_manager_get_icon_name          (PhoshVpnManager *self);
gboolean          phosh_vpn_manager_get_enabled            (PhoshVpnManager *self);
gboolean          phosh_vpn_manager_get_present            (PhoshVpnManager *self);
const char *      phosh_vpn_manager_get_last_connection    (PhoshVpnManager *self);
void              phosh_vpn_manager_toggle_last_connection (PhoshVpnManager *self);

G_END_DECLS
