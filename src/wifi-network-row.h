/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include <handy.h>
#include "util.h"
#include "wifi-network.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_NETWORK_ROW phosh_wifi_network_row_get_type ()
G_DECLARE_FINAL_TYPE (PhoshWifiNetworkRow, phosh_wifi_network_row, PHOSH, WIFI_NETWORK_ROW,
                      HdyActionRow)

GtkWidget *phosh_wifi_network_row_new (PhoshWifiNetwork *network);

G_END_DECLS
