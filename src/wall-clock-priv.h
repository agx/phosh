/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wall-clock.h>

G_BEGIN_DECLS

char *phosh_wall_clock_strip_am_pm (PhoshWallClock *self, const char *time);

G_END_DECLS
