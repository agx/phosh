/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_VPN_INFO (phosh_vpn_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshVpnInfo, phosh_vpn_info, PHOSH, VPN_INFO, PhoshStatusIcon)

GtkWidget * phosh_vpn_info_new (void);

G_END_DECLS
