/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layersurface.h"

#define PHOSH_TYPE_PANEL                 (phosh_panel_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPanel, phosh_panel, PHOSH, PANEL, PhoshLayerSurface)

#define PHOSH_PANEL_HEIGHT 32

/**
 * PhoshPanelState:
 * @PHOSH_PANEL_STATE_FOLDED: Only top-bar is visible
 * @PHOSH_PANEL_STATE_UNFOLDED: Settings menu is unfolded
 */
typedef enum {
  PHOSH_PANEL_STATE_FOLDED,
  PHOSH_PANEL_STATE_UNFOLDED,
} PhoshPanelState;

GtkWidget * phosh_panel_new (struct zwlr_layer_shell_v1 *layer_shell,
                             struct wl_output *wl_output);
void        phosh_panel_toggle_fold (PhoshPanel *self);
void        phosh_panel_fold (PhoshPanel *self);
void        phosh_panel_unfold (PhoshPanel *self);
PhoshPanelState phosh_panel_get_state (PhoshPanel *self);
