/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "status-page.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_STATUS_PAGE phosh_wifi_status_page_get_type ()
G_DECLARE_FINAL_TYPE (PhoshWifiStatusPage, phosh_wifi_status_page, PHOSH, WIFI_STATUS_PAGE,
                      PhoshStatusPage)

GtkWidget *phosh_wifi_status_page_new (void);

G_END_DECLS
