/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gtk/gtk.h>
#include "monitor/monitor.h"
#include "layersurface.h"

#define PHOSH_TYPE_LOCKSHIELD (phosh_lockshield_get_type())

G_DECLARE_FINAL_TYPE (PhoshLockshield, phosh_lockshield, PHOSH, LOCKSHIELD, PhoshLayerSurface)

GtkWidget *phosh_lockshield_new (struct zwlr_layer_shell_v1 *layer_shell,
                                 PhoshMonitor               *monitor);
