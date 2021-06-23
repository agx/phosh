/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "layersurface.h"
#include "overview.h"

#include <gtk/gtk.h>

#define PHOSH_TYPE_HOME (phosh_home_get_type())

#define PHOSH_HOME_BUTTON_HEIGHT 40

/**
 * PhoshHomeState:
 * @PHOSH_HOME_STATE_FOLDED: Only home button is visiable
 * @PHOSH_HOME_STATE_UNFOLDED: Home screen takes the whole screen except the top panel
 *
 * The state of #PhoshHome.
 */
typedef enum {
  PHOSH_HOME_STATE_FOLDED,
  PHOSH_HOME_STATE_UNFOLDED,
} PhoshHomeState;

G_DECLARE_FINAL_TYPE (PhoshHome, phosh_home, PHOSH, HOME, PhoshLayerSurface)

GtkWidget * phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                            struct wl_output *wl_output);
void phosh_home_set_state (PhoshHome *self, PhoshHomeState state);
PhoshOverview *phosh_home_get_overview (PhoshHome *self);
