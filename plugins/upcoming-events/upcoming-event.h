/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_UPCOMING_EVENT (phosh_upcoming_event_get_type ())

G_DECLARE_FINAL_TYPE (PhoshUpcomingEvent, phosh_upcoming_event, PHOSH, UPCOMING_EVENT, GtkBox)

GtkWidget *phosh_upcoming_event_new (const char *summary,
                                     GDateTime  *when,
                                     GDateTime  *end,
                                     GDateTime  *for_day,
                                     const char *color,
                                     gboolean    is_24h);
void       phosh_upcoming_event_set_summary    (PhoshUpcomingEvent *self,
                                                const char         *summary);

G_END_DECLS
