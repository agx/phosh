/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "monitor/monitor.h"

#include <glib-object.h>

#pragma once

G_BEGIN_DECLS

/**
 * PhoshShellLayout:
 * @PHOSH_SHELL_LAYOUT_NONE: Don't perform any additional layouting
 * @PHOSH_SHELL_LAYOUT_DEVICE: Use device information to optimize layout
 *
 * How the shell's UI elements are layed out.
 */
typedef enum {
  PHOSH_SHELL_LAYOUT_NONE     = 0,
  PHOSH_SHELL_LAYOUT_DEVICE   = 1,
} PhoshShellLayout;


typedef enum {
  PHOSH_LAYOUT_CLOCK_POS_CENTER = 0,
  PHOSH_LAYOUT_CLOCK_POS_LEFT   = 1,
  PHOSH_LAYOUT_CLOCK_POS_RIGHT  = 2,
} PhoshLayoutClockPosition;

#define PHOSH_TYPE_LAYOUT_MANAGER (phosh_layout_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLayoutManager, phosh_layout_manager, PHOSH, LAYOUT_MANAGER, GObject)

PhoshLayoutManager          *phosh_layout_manager_new (void);
PhoshLayoutClockPosition     phosh_layout_manager_get_clock_pos        (PhoshLayoutManager *self);
guint                        phosh_layout_manager_get_clock_shift      (PhoshLayoutManager *self);
guint                        phosh_layout_manager_get_corner_shift     (PhoshLayoutManager *self);

G_END_DECLS
