/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#include "calls-emergency-dbus.h"

#include "testlib.h"

G_BEGIN_DECLS

typedef struct _PhoshTestEmergencyCallsMock {
  PhoshEmergencyCalls *skel;
  gboolean             bus_acquired;
  guint                bus_id;
} PhoshTestEmergencyCallsMock;

PhoshTestEmergencyCallsMock *phosh_test_emergency_calls_mock_new (void);
void                phosh_test_emergency_calls_mock_dispose (PhoshTestEmergencyCallsMock *self);
void                phosh_test_emergency_calls_mock_export  (PhoshTestEmergencyCallsMock *mock);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshTestEmergencyCallsMock, phosh_test_emergency_calls_mock_dispose)

G_END_DECLS
