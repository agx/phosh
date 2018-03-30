/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef PHOSH_PANEL_H
#define PHOSH_PANEL_H

#include <gtk/gtk.h>

#define PHOSH_PANEL_TYPE                 (phosh_panel_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPanel, phosh_panel, PHOSH, PANEL, GtkWindow)

#define PHOSH_PANEL_HEIGHT 32

GtkWidget * phosh_panel_new (void);
gint        phosh_panel_get_height (PhoshPanel *self);

#endif /* PHOSH_PANEL_H */
