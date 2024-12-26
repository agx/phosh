/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "wall-clock.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_FAKE_CLOCK (phosh_fake_clock_get_type ())

G_DECLARE_FINAL_TYPE (PhoshFakeClock, phosh_fake_clock, PHOSH, FAKE_CLOCK, PhoshWallClock)

PhoshFakeClock *phosh_fake_clock_new (GDateTime *fake_offset);

G_END_DECLS
