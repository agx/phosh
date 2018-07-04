/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef PHOSH_BACKGROUND_H
#define PHOSH_BACKGROUND_H

#include <gtk/gtk.h>
#include "layersurface.h"

#define PHOSH_TYPE_BACKGROUND (phosh_background_get_type())

G_DECLARE_FINAL_TYPE (PhoshBackground, phosh_background, PHOSH, BACKGROUND, PhoshLayerSurface)

GtkWidget *phosh_background_new (gpointer layer_shell, gpointer wl_output, guint width, guint height);

#endif /* PHOSH_BACKGROUND_H */
