/*
 * Copyright (C) 2022 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <adwaita.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_TICKET_BOX_PREFS (phosh_ticket_box_prefs_get_type ())
G_DECLARE_FINAL_TYPE (PhoshTicketBoxPrefs, phosh_ticket_box_prefs, PHOSH, TICKET_BOX_PREFS, AdwPreferencesWindow)

G_END_DECLS
