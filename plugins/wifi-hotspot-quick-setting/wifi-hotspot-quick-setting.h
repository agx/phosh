/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arun.mani@tether.to>
 */

#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_HOTSPOT_QUICK_SETTING (phosh_wifi_hotspot_quick_setting_get_type ())
G_DECLARE_FINAL_TYPE (PhoshWifiHotspotQuickSetting,
                      phosh_wifi_hotspot_quick_setting,
                      PHOSH, WIFI_HOTSPOT_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
