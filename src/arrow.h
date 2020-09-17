/*
 * Copyright © 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_ARROW (phosh_arrow_get_type())

G_DECLARE_FINAL_TYPE (PhoshArrow, phosh_arrow, PHOSH, ARROW, GtkDrawingArea)

PhoshArrow *phosh_arrow_new (void);

double      phosh_arrow_get_progress (PhoshArrow *self);
void        phosh_arrow_set_progress (PhoshArrow *self,
                                      double      progress);

G_END_DECLS
