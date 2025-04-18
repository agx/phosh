/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "drag-surface.h"
#include "top-panel-bg.h"

#define PHOSH_TYPE_TOP_PANEL            (phosh_top_panel_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTopPanel, phosh_top_panel, PHOSH, TOP_PANEL, PhoshDragSurface)

#define PHOSH_TOP_BAR_HEIGHT 32
#define PHOSH_TOP_BAR_ICON_PIXEL_SIZE 16
/* Minimum padding of network and indicator box to the left / right screen edge */
#define PHOSH_TOP_BAR_MIN_PADDING 9
/* Minimum padding of network and indicator box to the left / right display cutouts */
#define PHOSH_TOP_BAR_CUTOUT_MIN_PADDING (PHOSH_TOP_BAR_MIN_PADDING / 2)

/**
 * PhoshTopPanelState:
 * @PHOSH_TOP_PANEL_STATE_FOLDED: Only top-bar is visible
 * @PHOSH_TOP_PANEL_STATE_UNFOLDED: Settings menu is unfolded
 */
typedef enum {
  PHOSH_TOP_PANEL_STATE_FOLDED,
  PHOSH_TOP_PANEL_STATE_UNFOLDED,
} PhoshTopPanelState;

GtkWidget         *phosh_top_panel_new (struct zwlr_layer_shell_v1          *layer_shell,
                                        struct zphoc_layer_shell_effects_v1 *layer_shell_effects,
                                        PhoshMonitor                        *monitor,
                                        guint32                              layer);
void               phosh_top_panel_toggle_fold (PhoshTopPanel *self);
void               phosh_top_panel_fold (PhoshTopPanel *self);
void               phosh_top_panel_unfold (PhoshTopPanel *self);
PhoshTopPanelState phosh_top_panel_get_state (PhoshTopPanel *self);
void               phosh_top_panel_set_layer (PhoshTopPanel *self, guint32 layer);
void               phosh_top_panel_set_bar_transparent (PhoshTopPanel *self, gboolean transparent);
