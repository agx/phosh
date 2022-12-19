/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "emergency-info.h"

#include <gtk/gtk.h>
#include <handy.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_INFO_ROW (phosh_emergency_info_row_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyInfoRow, phosh_emergency_info_row, PHOSH, EMERGENCY_INFO_ROW, HdyActionRow)

GtkWidget *phosh_emergency_info_row_new (const char *contact,
                                         const char *number,
                                         const char *relationship);

G_END_DECLS
