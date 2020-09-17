/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "dbus/gnome-session-presence-dbus.h"

typedef enum {
  PHOSH_SESSION_PRESENCE_STATUS_AVAILABLE = 0,
  PHOSH_SESSION_PRESENCE_STATUS_INVISIBLE,
  PHOSH_SESSION_PRESENCE_STATUS_BUSY,
  PHOSH_SESSION_PRESENCE_STATUS_IDLE,
} PhoshSessionPresenceStatus ;

#define PHOSH_TYPE_SESSION_PRESENCE                 (phosh_session_presence_get_type ())

G_DECLARE_FINAL_TYPE (PhoshSessionPresence, phosh_session_presence, PHOSH, SESSION_PRESENCE, PhoshSessionPresenceDbusPresenceProxy)

PhoshSessionPresence *phosh_session_presence_get_default_failable (void);
