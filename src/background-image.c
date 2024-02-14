/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-background-image"

#include "phosh-config.h"

#include "background-image.h"

#include <gtk/gtk.h>
#include <gio/gio.h>

/**
 * PhoshBackgroundImage:
 *
 * An image for a [type@Background] that can be loaded async via [type@BackgroundCache].
 */
enum {
  PROP_0,
  PROP_FILE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshBackgroundImage {
  GObject            parent;

  GFile             *file;
  GdkPixbuf         *pixbuf;
  GTimer            *load_timer;
};

static void initable_iface_init (GInitableIface *iface);
static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshBackgroundImage, phosh_background_image, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init));


static gboolean
initable_init (GInitable *initable, GCancellable *cancel, GError **error)
{
  PhoshBackgroundImage *self = PHOSH_BACKGROUND_IMAGE (initable);
  GError *local_error = NULL;
  g_autoptr (GdkPixbuf) rotated = NULL;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GFileInputStream) stream = NULL;

  stream = g_file_read (self->file, cancel, &local_error);
  if (stream == NULL) {
    g_propagate_error (error, local_error);
    return FALSE;
  }

  pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), cancel, &local_error);
  if (pixbuf == NULL) {
    g_propagate_error (error, local_error);
    return FALSE;
  }

  rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
  if (rotated != NULL)
    g_set_object (&pixbuf, rotated);

  self->pixbuf = g_steal_pointer (&pixbuf);

  g_timer_stop (self->load_timer);
  g_debug ("Background load took %.2fs", g_timer_elapsed (self->load_timer, NULL));

  return TRUE;
}


static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}


static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
  /* Use default: run initable_init in thread */
}


static void
phosh_background_image_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  PhoshBackgroundImage *self = PHOSH_BACKGROUND_IMAGE (object);

  switch (property_id) {
  case PROP_FILE:
    self->file = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_background_image_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PhoshBackgroundImage *self = PHOSH_BACKGROUND_IMAGE (object);

  switch (property_id) {
  case PROP_FILE:
    g_value_set_object (value, self->file);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_background_image_finalize (GObject *object)
{
  PhoshBackgroundImage *self = PHOSH_BACKGROUND_IMAGE (object);

  g_clear_object (&self->file);
  g_clear_object (&self->pixbuf);
  g_clear_pointer (&self->load_timer, g_timer_destroy);

  G_OBJECT_CLASS (phosh_background_image_parent_class)->finalize (object);
}


static void
phosh_background_image_class_init (PhoshBackgroundImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_background_image_get_property;
  object_class->set_property = phosh_background_image_set_property;
  object_class->finalize = phosh_background_image_finalize;

  props[PROP_FILE] =
    g_param_spec_object ("file", "", "",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_background_image_init (PhoshBackgroundImage *self)
{
  self->load_timer = g_timer_new ();
}


PhoshBackgroundImage *
phosh_background_image_new_sync (GFile *file, GCancellable *cancel, GError **error)
{
  return PHOSH_BACKGROUND_IMAGE (g_initable_new (PHOSH_TYPE_BACKGROUND_IMAGE,
                                                 cancel,
                                                 error,
                                                 "file", file,
                                                 NULL));
}


void
phosh_background_image_new (GFile              *file,
                            GCancellable       *cancellable,
                            GAsyncReadyCallback callback,
                            gpointer            user_data)
{
  g_return_if_fail (G_IS_FILE (file));

  g_async_initable_new_async (PHOSH_TYPE_BACKGROUND_IMAGE,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "file", file,
                              NULL);
}


PhoshBackgroundImage *
phosh_background_image_new_finish (GAsyncResult *res, GError **error)
{
  g_autoptr (GObject) source_object = NULL;
  GObject *object;

  source_object = g_async_result_get_source_object (res);
  g_assert (source_object != NULL);

  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);

  return PHOSH_BACKGROUND_IMAGE (object);
}

/**
 * phosh_background_image_get_pixbuf:
 * @self: The background image
 *
 * Gets the background image's pixbuf.
 *
 * Returns:(transfer none): The pixbuf
 */
GdkPixbuf *
phosh_background_image_get_pixbuf (PhoshBackgroundImage *self)
{
  g_return_val_if_fail (PHOSH_IS_BACKGROUND_IMAGE (self), FALSE);

  return self->pixbuf;
}

/**
 * phosh_background_image_get_file:
 * @self: The background image
 *
 * Gets the file the image was loaded from
 *
 * Returns:(transfer none): The file
 */
GFile *
phosh_background_image_get_file (PhoshBackgroundImage *self)
{
  g_return_val_if_fail (PHOSH_IS_BACKGROUND_IMAGE (self), FALSE);

  return self->file;
}
