/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <handy.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SCALE_ROW phosh_scale_row_get_type ()
G_DECLARE_FINAL_TYPE (PhoshScaleRow, phosh_scale_row, PHOSH, SCALE_ROW, HdyActionRow)

GtkWidget *phosh_scale_row_new (double scale, gboolean selected);
double     phosh_scale_row_get_scale (PhoshScaleRow *self);

G_END_DECLS
