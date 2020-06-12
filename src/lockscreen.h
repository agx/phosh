/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include "layersurface.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_LOCKSCREEN (phosh_lockscreen_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLockscreen, phosh_lockscreen, PHOSH, LOCKSCREEN,
                      PhoshLayerSurface)

GtkWidget * phosh_lockscreen_new (gpointer layer_shell, gpointer wl_output);

G_END_DECLS
