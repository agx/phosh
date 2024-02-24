/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */


#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SIMPLE_CUSTOM_QUICK_SETTING (phosh_simple_custom_quick_setting_get_type ())
G_DECLARE_FINAL_TYPE (PhoshSimpleCustomQuickSetting,
                      phosh_simple_custom_quick_setting,
                      PHOSH, SIMPLE_CUSTOM_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
