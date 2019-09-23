/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <layersurface.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_FADER (phosh_fader_get_type())

G_DECLARE_FINAL_TYPE (PhoshFader, phosh_fader, PHOSH, FADER, PhoshLayerSurface)

PhoshFader *phosh_fader_new (gpointer layer_shell,
                             gpointer wl_output);
G_END_DECLS
