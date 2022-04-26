/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WIDGET_BOX (phosh_widget_box_get_type ())

G_DECLARE_FINAL_TYPE (PhoshWidgetBox, phosh_widget_box, PHOSH, WIDGET_BOX, GtkBox)

PhoshWidgetBox *phosh_widget_box_new (GStrv plugin_dirs);
void            phosh_widget_box_set_plugins (PhoshWidgetBox *self, GStrv plugins);
gboolean        phosh_widget_box_has_plugins (PhoshWidgetBox *self);

G_END_DECLS
