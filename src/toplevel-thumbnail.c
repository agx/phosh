/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Sebastian Krzyszkowiak <sebastian.krzyszkowiak@puri.sm>
 */

#define G_LOG_DOMAIN "phosh-toplevel-thumbnail"

#include "phosh-wayland.h"
#include "shell.h"
#include "toplevel-thumbnail.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

/**
 * SECTION:toplevel-thumbnail
 * @short_description: Represents an image snapshot of PhoshToplevel obtained via phosh-private and wlr-screencopy Wayland protocols.
 * @Title: PhoshToplevelThumbnail
 */

enum {
  PHOSH_TOPLEVEL_THUMBNAIL_PROP_0,
  PHOSH_TOPLEVEL_THUMBNAIL_PROP_HANDLE,
  PHOSH_TOPLEVEL_THUMBNAIL_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_TOPLEVEL_THUMBNAIL_PROP_LAST_PROP];

struct _PhoshToplevelThumbnail {
  GObject parent;
  struct zwlr_screencopy_frame_v1 *handle;
  struct wl_buffer *buffer;
  void *data;
  int width, height, stride;
  gboolean ready;
};

G_DEFINE_TYPE (PhoshToplevelThumbnail, phosh_toplevel_thumbnail, PHOSH_TYPE_THUMBNAIL);


static void
phosh_toplevel_thumbnail_set_ready (PhoshThumbnail *self, gboolean ready)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL_THUMBNAIL (self));
  PHOSH_TOPLEVEL_THUMBNAIL (self)->ready = ready;
  PHOSH_THUMBNAIL_CLASS (phosh_toplevel_thumbnail_parent_class)->set_ready (self, ready);
}


static void
screencopy_handle_buffer (void *data,
                          struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                          uint32_t format,
                          uint32_t width,
                          uint32_t height,
                          uint32_t stride)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (data);

  PhoshWayland *way = phosh_wayland_get_default ();

  void *d;
  struct wl_shm_pool *pool;
  size_t size = stride * height;
  int fd;

  g_debug ("screencopy_handle_buffer: width %d height %d stride %d", width, height, stride);

  if (!size) {
    g_warning ("Got screencopy_handle_buffer with no size!");
    return;
  }

  fd = phosh_create_shm_file (size);
  if (fd == -1) {
    g_warning ("Could not create shm file for thumbnail buffer! %s", g_strerror (errno));
    return;
  }

  d = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (d == MAP_FAILED) {
    g_warning ("Could not mmap thumbnail buffer file! [fd: %d] %s", fd, g_strerror (errno));
    close (fd);
    return;
  }

  pool = wl_shm_create_pool (phosh_wayland_get_wl_shm (way), fd, size);
  self->buffer = wl_shm_pool_create_buffer (pool, 0, width, height, stride, format);
  wl_shm_pool_destroy (pool);
  close(fd);

  self->data = d;
  zwlr_screencopy_frame_v1_copy (zwlr_screencopy_frame_v1, self->buffer);

  self->width = width;
  self->height = height;
  self->stride = stride;
}

static void
screencopy_handle_flags(void *data,
                        struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                        uint32_t flags)
{
  g_return_if_fail (flags != 0);
}

static void
screencopy_handle_ready(void *data,
                        struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                        uint32_t tv_sec_hi,
                        uint32_t tv_sec_lo,
                        uint32_t tv_nsec)
{
  phosh_toplevel_thumbnail_set_ready (PHOSH_THUMBNAIL (data), TRUE);
}

static void
screencopy_handle_failed (void *data,
                          struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1)
{
  g_warning ("screencopy failed! %p", data);
}

static void
screencopy_handle_damage (void *data,
                          struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
                          uint32_t x,
                          uint32_t y,
                          uint32_t width,
                          uint32_t height)
{
}

static const struct zwlr_screencopy_frame_v1_listener zwlr_screencopy_frame_listener = {
  screencopy_handle_buffer,
  screencopy_handle_flags,
  screencopy_handle_ready,
  screencopy_handle_failed,
  screencopy_handle_damage
};


static void *
phosh_toplevel_thumbnail_get_image (PhoshThumbnail *self)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL_THUMBNAIL (self), NULL);
  return PHOSH_TOPLEVEL_THUMBNAIL (self)->data;
}

static void
phosh_toplevel_thumbnail_get_size (PhoshThumbnail *self, guint *width, guint *height, guint *stride)
{
  PhoshToplevelThumbnail *thumbnail = PHOSH_TOPLEVEL_THUMBNAIL (self);
  if (width) {
    *width = thumbnail->width;
  }
  if (height) {
    *height = thumbnail->height;
  }
  if (stride) {
    *stride = thumbnail->stride;
  }
}

static gboolean
phosh_toplevel_thumbnail_is_ready (PhoshThumbnail *self)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL_THUMBNAIL (self), FALSE);
  return PHOSH_TOPLEVEL_THUMBNAIL (self)->ready;
}


static void
phosh_toplevel_thumbnail_set_property (GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (object);

  switch (property_id) {
    case PHOSH_TOPLEVEL_THUMBNAIL_PROP_HANDLE:
      self->handle = g_value_get_pointer (value);
      break;
    default:
      PHOSH_THUMBNAIL_CLASS (self)->parent_class.set_property (object, property_id, value, pspec);
      break;
  }
}


static void
phosh_toplevel_thumbnail_get_property (GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (object);

  switch (property_id) {
    case PHOSH_TOPLEVEL_THUMBNAIL_PROP_HANDLE:
      g_value_set_pointer (value, self->handle);
      break;
    default:
      PHOSH_THUMBNAIL_CLASS (self)->parent_class.get_property (object, property_id, value, pspec);
      break;
  }
}


static void
phosh_toplevel_thumbnail_constructed (GObject *object)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (object);
  zwlr_screencopy_frame_v1_add_listener (self->handle, &zwlr_screencopy_frame_listener, self);

  G_OBJECT_CLASS (phosh_toplevel_thumbnail_parent_class)->constructed (object);
}


static void
phosh_toplevel_thumbnail_dispose (GObject *object)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (object);

  g_clear_pointer (&self->handle, zwlr_screencopy_frame_v1_destroy);
  g_clear_pointer (&self->buffer, wl_buffer_destroy);

  G_OBJECT_CLASS (phosh_toplevel_thumbnail_parent_class)->dispose (object);
}


static void
phosh_toplevel_thumbnail_finalize (GObject *object)
{
  PhoshToplevelThumbnail *self = PHOSH_TOPLEVEL_THUMBNAIL (object);

  if (self->data) {
    if (munmap (self->data, self->stride * self->height) != 0) {
      g_warning ("Could not munmap toplevel thumbnail data! [%p] %s", self, g_strerror (errno));
    }
  }

  G_OBJECT_CLASS (phosh_toplevel_thumbnail_parent_class)->finalize (object);
}


static void
phosh_toplevel_thumbnail_class_init (PhoshToplevelThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = phosh_toplevel_thumbnail_set_property;
  object_class->get_property = phosh_toplevel_thumbnail_get_property;
  object_class->constructed = phosh_toplevel_thumbnail_constructed;
  object_class->dispose = phosh_toplevel_thumbnail_dispose;
  object_class->finalize = phosh_toplevel_thumbnail_finalize;

  klass->parent_class.is_ready = phosh_toplevel_thumbnail_is_ready;
  klass->parent_class.get_image = phosh_toplevel_thumbnail_get_image;
  klass->parent_class.get_size = phosh_toplevel_thumbnail_get_size;
  klass->parent_class.set_ready = phosh_toplevel_thumbnail_set_ready;

  props[PHOSH_TOPLEVEL_THUMBNAIL_PROP_HANDLE] =
    g_param_spec_pointer ("handle",
                          "handle",
                          "The zwlr_screencopy_frame_v1 object associated with this thumbnail",
                          G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PHOSH_TOPLEVEL_THUMBNAIL_PROP_LAST_PROP, props);

}


static void
phosh_toplevel_thumbnail_init (PhoshToplevelThumbnail *self)
{
}


static PhoshToplevelThumbnail *
phosh_toplevel_thumbnail_new_from_handle (struct zwlr_screencopy_frame_v1 *handle)
{
  return g_object_new (PHOSH_TYPE_TOPLEVEL_THUMBNAIL, "handle", handle, NULL);
}

PhoshToplevelThumbnail *
phosh_toplevel_thumbnail_new_from_toplevel (PhoshToplevel *toplevel, guint32 max_width, guint32 max_height)
{
  struct zwlr_foreign_toplevel_handle_v1 *handle = phosh_toplevel_get_handle (PHOSH_TOPLEVEL (toplevel));
  struct phosh_private *phosh = phosh_wayland_get_phosh_private (phosh_wayland_get_default ());
  struct zwlr_screencopy_frame_v1 *frame;

  if (!phosh || phosh_private_get_version (phosh) < PHOSH_PRIVATE_GET_THUMBNAIL_SINCE)
    return NULL;

  g_debug ("Requesting a %dx%d thumbnail for toplevel %p [%s]", max_width, max_height,
          toplevel, phosh_toplevel_get_title (toplevel));

  frame = phosh_private_get_thumbnail (
    phosh,
    handle,
    max_width, max_height
   );

  return phosh_toplevel_thumbnail_new_from_handle (frame);
}
