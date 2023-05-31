/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
/* TODO: We use the enum constants from here, use glib-mkenums */
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_LAYER_SURFACE                 (phosh_layer_surface_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshLayerSurface, phosh_layer_surface, PHOSH, LAYER_SURFACE, GtkWindow)

/**
 * PhoshLayerSurfaceClass
 * @parent_class: The parent class
 * @configured: invoked when layer surface is configured
 */
struct _PhoshLayerSurfaceClass
{
  GtkWindowClass parent_class;

  /* Signals
   */
  void (*configured)   (PhoshLayerSurface    *self);
};

GtkWidget *phosh_layer_surface_new (gpointer layer_shell,
                                    gpointer wl_output);
struct     zwlr_layer_surface_v1 *phosh_layer_surface_get_layer_surface(PhoshLayerSurface *self);
struct     wl_surface            *phosh_layer_surface_get_wl_surface(PhoshLayerSurface *self);
void                              phosh_layer_surface_set_size(PhoshLayerSurface *self,
                                                               int width,
                                                               int height);
void                              phosh_layer_surface_set_margins(PhoshLayerSurface *self,
                                                                  int top,
                                                                  int right,
                                                                  int bottom,
                                                                  int left);
void                              phosh_layer_surface_set_exclusive_zone(PhoshLayerSurface *self,
                                                                         int zone);
void                              phosh_layer_surface_set_kbd_interactivity(PhoshLayerSurface *self,
                                                                            gboolean interactivity);
void                              phosh_layer_surface_set_layer (PhoshLayerSurface *self,
                                                                 guint32            layer);
void                              phosh_layer_surface_wl_surface_commit (PhoshLayerSurface *self);
void                              phosh_layer_surface_get_margins       (PhoshLayerSurface *self,
                                                                         int               *top,
                                                                         int               *right,
                                                                         int               *bottom,
                                                                         int               *left);
int                               phosh_layer_surface_get_configured_width  (PhoshLayerSurface *self);
int                               phosh_layer_surface_get_configured_height (PhoshLayerSurface *self);
void                              phosh_layer_surface_set_alpha (PhoshLayerSurface *self,
                                                                 double             alpha);
gboolean                          phosh_layer_surface_has_alpha (PhoshLayerSurface *self);

G_END_DECLS
