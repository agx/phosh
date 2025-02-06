/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

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

  /* Padding for future expansion */
  void                 (*_phosh_reserved1) (void);
  void                 (*_phosh_reserved2) (void);
  void                 (*_phosh_reserved3) (void);
  void                 (*_phosh_reserved4) (void);
  void                 (*_phosh_reserved5) (void);
  void                 (*_phosh_reserved6) (void);
  void                 (*_phosh_reserved7) (void);
  void                 (*_phosh_reserved8) (void);
  void                 (*_phosh_reserved9) (void);
};

G_END_DECLS
