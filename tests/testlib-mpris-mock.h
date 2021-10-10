/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "mpris-dbus.h"

#include "testlib.h"

G_BEGIN_DECLS

typedef struct _PhoshMprisMock {
  PhoshMprisDBusMediaPlayer2Player *skel;
  gboolean bus_acquired;
  guint    bus_id;
} PhoshTestMprisMock;

PhoshTestMprisMock *phosh_test_mpris_mock_new (void);
void                phosh_test_mpris_mock_dispose (PhoshTestMprisMock *self);
void                phosh_mpris_mock_export (PhoshTestMprisMock *mock);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshTestMprisMock, phosh_test_mpris_mock_dispose)

G_END_DECLS
