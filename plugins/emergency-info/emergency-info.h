/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Chris Talbot <chris@talbothome.com>
 */


#include <gtk/gtk.h>
#include <handy.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_INFO (phosh_emergency_info_get_type ())
G_DECLARE_FINAL_TYPE (PhoshEmergencyInfo, phosh_emergency_info, PHOSH, EMERGENCY_INFO, GtkBox)

G_END_DECLS
