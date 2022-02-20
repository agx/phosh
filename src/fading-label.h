/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_FADING_LABEL (phosh_fading_label_get_type())

G_DECLARE_FINAL_TYPE (PhoshFadingLabel, phosh_fading_label, PHOSH, FADING_LABEL, GtkBin)

GtkWidget   *phosh_fading_label_new       (const char       *label);
const char  *phosh_fading_label_get_label (PhoshFadingLabel *self);
void         phosh_fading_label_set_label (PhoshFadingLabel *self,
                                           const char       *label);

float        phosh_fading_label_get_align (PhoshFadingLabel *self);
void         phosh_fading_label_set_align (PhoshFadingLabel *self,
                                           gfloat            align);

G_END_DECLS
