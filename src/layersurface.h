/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <gtk/gtk.h>
/* TODO: We use the enum constants from here, use glib-mkenums */
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#define PHOSH_TYPE_LAYER_SURFACE                 (phosh_layer_surface_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshLayerSurface, phosh_layer_surface, PHOSH, LAYER_SURFACE, GtkWindow)

/**
 * PhoshLayerSurfaceClass
 * @parent_class: The parent class
 */
struct _PhoshLayerSurfaceClass
{
  GtkWindowClass parent_class;

  /* Signals
   */
  void (*configured)   (PhoshLayerSurface    *self);
};

GtkWidget * phosh_layer_surface_new (gpointer layer_shell,
                                     gpointer wl_output);
