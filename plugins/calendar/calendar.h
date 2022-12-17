/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */


#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_CALENDAR (phosh_calendar_get_type ())
G_DECLARE_FINAL_TYPE (PhoshCalendar, phosh_calendar, PHOSH, CALENDAR, GtkBox)

G_END_DECLS
