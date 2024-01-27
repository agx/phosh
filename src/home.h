/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "drag-surface.h"
#include "monitor/monitor.h"
#include "overview.h"

#include <gtk/gtk.h>

#define PHOSH_TYPE_HOME (phosh_home_get_type())

#define PHOSH_HOME_BAR_HEIGHT 15

/**
 * PhoshHomeState:
 * @PHOSH_HOME_STATE_FOLDED: Only home button is visible
 * @PHOSH_HOME_STATE_UNFOLDED: Home screen takes the whole screen except the top panel
 *
 * The state of #PhoshHome.
 */
typedef enum {
  PHOSH_HOME_STATE_FOLDED,
  PHOSH_HOME_STATE_UNFOLDED,
} PhoshHomeState;

G_DECLARE_FINAL_TYPE (PhoshHome, phosh_home, PHOSH, HOME, PhoshDragSurface)

GtkWidget     *phosh_home_new (struct zwlr_layer_shell_v1          *layer_shell,
                               struct zphoc_layer_shell_effects_v1 *layer_shell_effects,
                               PhoshMonitor                        *monitor);
PhoshHomeState phosh_home_get_state (PhoshHome *self);
void           phosh_home_set_state (PhoshHome *self, PhoshHomeState state);
PhoshOverview *phosh_home_get_overview (PhoshHome *self);
