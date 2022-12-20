/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#pragma once

#include "emergency-contact.h"
#include "emergency-calls-manager.h"

#include <handy.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_CONTACT_ROW (phosh_emergency_contact_row_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyContactRow, phosh_emergency_contact_row, PHOSH, EMERGENCY_CONTACT_ROW, HdyActionRow)

PhoshEmergencyContactRow *phosh_emergency_contact_row_new  (PhoshEmergencyContact      *contact);
void                      phosh_emergency_contact_row_call (PhoshEmergencyContactRow   *self,
                                                            PhoshEmergencyCallsManager *manager);

G_END_DECLS
