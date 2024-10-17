/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

typedef enum _PhoshPomodoroState {
  PHOSH_POMODORO_STATE_OFF = 0,
  PHOSH_POMODORO_STATE_ACTIVE = 1,
  PHOSH_POMODORO_STATE_BREAK = 2,
} PhoshPomodoroState;

#define PHOSH_TYPE_POMODORO_QUICK_SETTING (phosh_pomodoro_quick_setting_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPomodoroQuickSetting,
                      phosh_pomodoro_quick_setting,
                      PHOSH, POMODORO_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
