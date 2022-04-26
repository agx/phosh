/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CALENDAR_EVENT (phosh_calendar_event_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCalendarEvent, phosh_calendar_event, PHOSH, CALENDAR_EVENT, GObject)

PhoshCalendarEvent *phosh_calendar_event_new (const char *id,
                                              const char *summary,
                                              GDateTime  *begin,
                                              GDateTime  *end,
                                              const char *color);
const char *phosh_calendar_event_get_id (PhoshCalendarEvent *self);
const char *phosh_calendar_event_get_summary (PhoshCalendarEvent *self);
GDateTime  *phosh_calendar_event_get_begin (PhoshCalendarEvent *self);
GDateTime  *phosh_calendar_event_get_end (PhoshCalendarEvent *self);
const char *phosh_calendar_event_get_color (PhoshCalendarEvent *self);

G_END_DECLS
