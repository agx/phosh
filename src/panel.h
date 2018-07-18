/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include "layersurface.h"

#define PHOSH_TYPE_PANEL                 (phosh_panel_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPanel, phosh_panel, PHOSH, PANEL, PhoshLayerSurface)

#define PHOSH_PANEL_HEIGHT 32

GtkWidget * phosh_panel_new (struct zwlr_layer_shell_v1 *layer_shell,
                             struct wl_output *wl_output);
gint        phosh_panel_get_height (PhoshPanel *self);
