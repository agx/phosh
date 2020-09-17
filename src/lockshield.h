/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gtk/gtk.h>
#include "layersurface.h"

#define PHOSH_TYPE_LOCKSHIELD (phosh_lockshield_get_type())

G_DECLARE_FINAL_TYPE (PhoshLockshield, phosh_lockshield, PHOSH, LOCKSHIELD, PhoshLayerSurface)

GtkWidget * phosh_lockshield_new (gpointer layer_shell,
                                  gpointer wl_output);
