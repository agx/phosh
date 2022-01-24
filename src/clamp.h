/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CLAMP (phosh_clamp_get_type())

G_DECLARE_FINAL_TYPE (PhoshClamp, phosh_clamp, PHOSH, CLAMP, GtkBin)

GtkWidget *phosh_clamp_new (void);
gint phosh_clamp_get_natural_size (PhoshClamp *self);
void phosh_clamp_set_natural_size (PhoshClamp *self,
                                   gint        natural_size);

G_END_DECLS
