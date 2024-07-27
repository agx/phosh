/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quick-setting.h"
#include "shell.h"
#include "status-page.h"

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define PHOSH_TYPE_BT_STATUS_PAGE phosh_bt_status_page_get_type ()
G_DECLARE_FINAL_TYPE (PhoshBtStatusPage, phosh_bt_status_page, PHOSH, BT_STATUS_PAGE,
                      PhoshStatusPage)

GtkWidget *phosh_bt_status_page_new (void);

G_END_DECLS
