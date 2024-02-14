/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layersurface.h"
#include "monitor/monitor.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-bg.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BACKGROUND (phosh_background_get_type())

G_DECLARE_FINAL_TYPE (PhoshBackground, phosh_background, PHOSH, BACKGROUND, PhoshLayerSurface)

typedef struct _PhoshBackgroundData {
  GFile                  *uri;
  GdkRGBA                 color;
  GDesktopBackgroundStyle style;
} PhoshBackgroundData;

GtkWidget          *phosh_background_new              (gpointer                 layer_shell,
                                                       PhoshMonitor            *monitor,
                                                       gboolean                 primary);
void                phosh_background_set_primary      (PhoshBackground         *self,
                                                       gboolean                 primary);
float               phosh_background_get_scale        (PhoshBackground         *self);
void                phosh_background_set_scale        (PhoshBackground         *self,
                                                       float                    scale);
void                phosh_background_needs_update     (PhoshBackground         *self);

void                phosh_background_data_free        (PhoshBackgroundData    *bg_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshBackgroundData, phosh_background_data_free)

G_END_DECLS
