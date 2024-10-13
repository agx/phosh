/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <adwaita.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_POMODORO_QUICK_SETTING_PREFS (phosh_pomodoro_quick_setting_prefs_get_type ())
G_DECLARE_FINAL_TYPE (PhoshPomodoroQuickSettingPrefs,
                      phosh_pomodoro_quick_setting_prefs,
                      PHOSH, POMODORO_QUICK_SETTING_PREFS, AdwPreferencesWindow)

G_END_DECLS
