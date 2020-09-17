/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_INFO (phosh_wifi_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshWifiInfo, phosh_wifi_info, PHOSH, WIFI_INFO, PhoshStatusIcon)

GtkWidget * phosh_wifi_info_new (void);

G_END_DECLS
