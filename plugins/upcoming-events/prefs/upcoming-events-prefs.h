/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <adwaita.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_UPCOMING_EVENTS_PREFS (phosh_upcoming_events_prefs_get_type ())
G_DECLARE_FINAL_TYPE (PhoshUpcomingEventsPrefs, phosh_upcoming_events_prefs, PHOSH, UPCOMING_EVENTS_PREFS, AdwPreferencesWindow)

G_END_DECLS
