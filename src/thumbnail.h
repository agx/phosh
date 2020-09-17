/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#define PHOSH_TYPE_THUMBNAIL (phosh_thumbnail_get_type())

G_DECLARE_DERIVABLE_TYPE (PhoshThumbnail,
                          phosh_thumbnail,
                          PHOSH,
                          THUMBNAIL,
                          GObject)

struct _PhoshThumbnailClass
{
  GObjectClass parent_class;
  void*        (*get_image) (PhoshThumbnail *self);
  void         (*get_size)  (PhoshThumbnail *self, guint *width, guint *height, guint *stride);
  gboolean     (*is_ready)  (PhoshThumbnail *self);
  void         (*set_ready) (PhoshThumbnail *self, gboolean ready);
};

void     *phosh_thumbnail_get_image (PhoshThumbnail *self);
void      phosh_thumbnail_get_size  (PhoshThumbnail *self, guint *width, guint *height, guint *stride);
gboolean  phosh_thumbnail_is_ready  (PhoshThumbnail *self);
