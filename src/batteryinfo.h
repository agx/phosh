/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include "status-icon.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BATTERY_INFO (phosh_battery_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshBatteryInfo, phosh_battery_info, PHOSH, BATTERY_INFO, PhoshStatusIcon)

GtkWidget * phosh_battery_info_new (void);

G_END_DECLS
