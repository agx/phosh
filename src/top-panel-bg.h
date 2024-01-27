/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layersurface.h"
#include "monitor/monitor.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_TOP_PANEL_BG (phosh_top_panel_bg_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTopPanelBg, phosh_top_panel_bg, PHOSH, TOP_PANEL_BG, PhoshLayerSurface)

PhoshTopPanelBg  *phosh_top_panel_bg_new              (struct zwlr_layer_shell_v1 *layer_shell,
                                                       PhoshMonitor               *monitor,
                                                       guint32                     layer);
void              phosh_top_panel_bg_set_transparency (PhoshTopPanelBg *self,
                                                       double            transparency);

G_END_DECLS
