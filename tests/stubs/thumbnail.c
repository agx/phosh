/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Sebastian Krzyszkowiak <sebastian.krzyszkowiak@puri.sm>
 */

#include "thumbnail.h"
#include "toplevel.h"
#include "toplevel-thumbnail.h"

enum {
  SIGNAL_READY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (PhoshThumbnail, phosh_thumbnail, G_TYPE_OBJECT);

static void
phosh_thumbnail_class_init (PhoshThumbnailClass *klass)
{
  signals[SIGNAL_READY] = g_signal_new (
    "ready",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 0);
}

static void
phosh_thumbnail_init (PhoshThumbnail *self)
{
}

void *
phosh_thumbnail_get_image (PhoshThumbnail *self)
{
  return NULL;
}

void
phosh_thumbnail_get_size (PhoshThumbnail *self, guint *width, guint *height, guint *stride)
{
}

gboolean
phosh_thumbnail_is_ready (PhoshThumbnail *self)
{
  return FALSE;
}

PhoshToplevelThumbnail *
phosh_toplevel_thumbnail_new_from_toplevel (PhoshToplevel *toplevel, guint32 max_width, guint32 max_height)
{
  return g_object_new (PHOSH_TYPE_THUMBNAIL, NULL);
}
