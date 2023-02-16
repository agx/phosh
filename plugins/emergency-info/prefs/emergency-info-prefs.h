/*
 * Copyright (C) 2022 Chris Talbot
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <adwaita.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_INFO_PREFS (phosh_emergency_info_prefs_get_type ())
G_DECLARE_FINAL_TYPE (PhoshEmergencyInfoPrefs, phosh_emergency_info_prefs, PHOSH, EMERGENCY_INFO_PREFS, GtkDialog)

G_END_DECLS
