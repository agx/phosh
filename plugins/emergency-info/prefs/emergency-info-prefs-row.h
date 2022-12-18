/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_INFO_PREFS_ROW (phosh_emergency_info_prefs_row_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyInfoPrefsRow, phosh_emergency_info_prefs_row, PHOSH, EMERGENCY_INFO_PREFS_ROW, AdwActionRow)

GtkWidget *phosh_emergency_info_prefs_row_new (const char *contact,
                                               const char *number,
                                               const char *relationship);

G_END_DECLS
