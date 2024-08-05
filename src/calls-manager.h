/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "call.h"
#include "manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PhoshCallState:
 *
 * The call state. Must match call's CallsCallState.
 */
typedef enum
{
  /*< private >*/
  PHOSH_CALL_STATE_ACTIVE = 1,
  PHOSH_CALL_STATE_HELD = 2,
  PHOSH_CALL_STATE_DIALING = 3,
  /* PHOSH_CALL_STATE_ALERTING (deprecated) */
  PHOSH_CALL_STATE_INCOMING = 5,
  /* PHOSH_CALL_STATE_WAITING (deprecated) */
  PHOSH_CALL_STATE_DISCONNECTED = 7,
} PhoshCallState;


#define PHOSH_TYPE_CALLS_MANAGER (phosh_calls_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCallsManager, phosh_calls_manager, PHOSH, CALLS_MANAGER, PhoshManager)

PhoshCallsManager *phosh_calls_manager_new (void);
gboolean           phosh_calls_manager_get_present (PhoshCallsManager *self);
gboolean           phosh_calls_manager_has_incoming_call (PhoshCallsManager *self);
const char        *phosh_calls_manager_get_active_call_handle (PhoshCallsManager *self);
PhoshCall         *phosh_calls_manager_get_call (PhoshCallsManager *self, const char *handle);

G_END_DECLS
