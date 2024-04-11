/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Adam Honse <calcprogrammer1@gmail.com>
 */


#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SCALING_QUICK_SETTING (phosh_scaling_quick_setting_get_type ())
G_DECLARE_FINAL_TYPE (PhoshScalingQuickSetting, phosh_scaling_quick_setting, PHOSH,
                      SCALING_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
