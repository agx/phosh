/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#pragma once

#include "emergency-calls-manager.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_CONTACT (phosh_emergency_contact_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyContact, phosh_emergency_contact, PHOSH, EMERGENCY_CONTACT, GObject)

PhoshEmergencyContact *phosh_emergency_contact_new (const char *id,
                                                    const char *name,
                                                    gint32      source,
                                                    GVariant   *additional_properties);
void phosh_emergency_contact_call (PhoshEmergencyContact      *self,
                                   PhoshEmergencyCallsManager *emergency_calls_manager);

G_END_DECLS
