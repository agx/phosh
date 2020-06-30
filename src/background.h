/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include "layersurface.h"
#include "monitor/monitor.h"

#include <gtk/gtk.h>


#define PHOSH_TYPE_BACKGROUND (phosh_background_get_type())

G_DECLARE_FINAL_TYPE (PhoshBackground, phosh_background, PHOSH, BACKGROUND, PhoshLayerSurface)

GtkWidget *phosh_background_new (gpointer layer_shell,
                                 gpointer wl_output,
                                 gboolean primary);
void phosh_background_set_primary (PhoshBackground *self, gboolean primary);
