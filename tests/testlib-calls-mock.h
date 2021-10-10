/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

G_BEGIN_DECLS

typedef struct _PhoshCallsMock {
  GDBusObjectManagerServer *object_manager;
  gboolean bus_acquired;
  guint    bus_id;
} PhoshTestCallsMock;

PhoshTestCallsMock *phosh_test_calls_mock_new (void);
void                phosh_test_calls_mock_dispose (PhoshTestCallsMock *self);
void                phosh_calls_mock_export (PhoshTestCallsMock *mock);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshTestCallsMock, phosh_test_calls_mock_dispose)

G_END_DECLS
