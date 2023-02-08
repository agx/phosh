/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_REVEALER (phosh_revealer_get_type ())

G_DECLARE_FINAL_TYPE (PhoshRevealer, phosh_revealer, PHOSH, REVEALER, GtkRevealer)

PhoshRevealer *phosh_revealer_new (void);
gboolean       phosh_revealer_get_show_child (PhoshRevealer *self);
void           phosh_revealer_set_show_child (PhoshRevealer *self, gboolean show_child);

G_END_DECLS
