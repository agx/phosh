/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "wall-clock.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define TESTLIB_TYPE_WALL_CLOCK_MOCK (testlib_wall_clock_mock_get_type ())

G_DECLARE_FINAL_TYPE (TestlibWallClockMock, testlib_wall_clock_mock, TESTLIB, WALL_CLOCK_MOCK,
                      PhoshWallClock)

TestlibWallClockMock *testlib_wall_clock_mock_new (GDateTime *fake_offset);

G_END_DECLS
