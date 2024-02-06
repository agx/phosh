/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quick-setting.h"
#include "shell.h"
#include "status-page.h"
#include "wifi-manager.h"
#include "wifi-network.h"
#include "wifi-network-row.h"

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_STATUS_PAGE phosh_wifi_status_page_get_type ()
G_DECLARE_FINAL_TYPE (PhoshWifiStatusPage, phosh_wifi_status_page, PHOSH, WIFI_STATUS_PAGE,
                      PhoshStatusPage)

GtkWidget *phosh_wifi_status_page_new (void);

G_END_DECLS
