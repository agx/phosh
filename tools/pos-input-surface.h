/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layersurface.h"

G_BEGIN_DECLS

#define POS_TYPE_INPUT_SURFACE (pos_input_surface_get_type ())

G_DECLARE_FINAL_TYPE (PosInputSurface, pos_input_surface, POS, INPUT_SURFACE, PhoshLayerSurface)

void         pos_input_surface_set_active          (PosInputSurface *self, gboolean active);
gboolean     pos_input_surface_get_active          (PosInputSurface *self);
void         pos_input_surface_set_purpose         (PosInputSurface *self, guint purpose);
void         pos_input_surface_set_hint            (PosInputSurface *self, guint hint);
void         pos_input_surface_done                (PosInputSurface *self);

G_END_DECLS
