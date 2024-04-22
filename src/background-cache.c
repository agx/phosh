/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-background-cache"

#include "phosh-config.h"

#include "background-cache.h"
#include "background-image.h"
#include "util.h"

#include <gio/gio.h>

/**
 * PhoshBackgroundCache:
 *
 * A cache of background images
 */

enum {
  IMAGE_PRESENT,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshBackgroundCache {
  GObject     parent;

  GHashTable *background_images;
};
G_DEFINE_TYPE (PhoshBackgroundCache, phosh_background_cache, G_TYPE_OBJECT)


static void
on_background_image_loaded (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  PhoshBackgroundCache *self = PHOSH_BACKGROUND_CACHE (user_data);
  PhoshBackgroundImage *image;
  g_autoptr (GError) err = NULL;
  GFile *file;

  image = phosh_background_image_new_finish (res, &err);
  if (!image) {
    phosh_async_error_warn (err, "Failed to load background image");
    return;
  }

  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));
  file = phosh_background_image_get_file (image);
  g_hash_table_insert (self->background_images, g_object_ref (file), image);

  g_signal_emit (self, signals[IMAGE_PRESENT], 0, image);
}


static void
phosh_background_cache_finalize (GObject *object)
{
  PhoshBackgroundCache *self = PHOSH_BACKGROUND_CACHE (object);

  g_clear_pointer (&self->background_images, g_hash_table_destroy);

  G_OBJECT_CLASS (phosh_background_cache_parent_class)->finalize (object);
}


static void
phosh_background_cache_class_init (PhoshBackgroundCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_background_cache_finalize;

  /**
   * PhoshBackgroundCache:image-present
   * @self: The background cache
   * @image: The loaded background image
   *
   * Emitted when an image requested via [type@fetch_background] can be fetched
   * from the cache.
   */
  signals[IMAGE_PRESENT] = g_signal_new ("image-present",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST, 0,
                                         NULL, NULL, NULL,
                                         G_TYPE_NONE,
                                         1,
                                         PHOSH_TYPE_BACKGROUND_IMAGE);
}


static void
phosh_background_cache_init (PhoshBackgroundCache *self)
{
  self->background_images = g_hash_table_new_full (g_file_hash,
                                                   (GEqualFunc) g_file_equal,
                                                   g_object_unref,
                                                   g_object_unref);
}

/**
 * phosh_background_cache_get_default:
 *
 * Gets the background cache singleton.
 *
 * Returns:(transfer none): The background cache singleton.
 */
PhoshBackgroundCache *
phosh_background_cache_get_default (void)
{
  static PhoshBackgroundCache *instance;

  if (instance == NULL) {
    g_debug ("Creating background cache");
    instance = g_object_new (PHOSH_TYPE_BACKGROUND_CACHE, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}

/**
 * phosh_background_cache_fetch_background:
 * @self: The background cache
 * @file: The file to lookup or load
 * @cancel: A cancellable
 *
 * Loads an image into the cache and if not yet present. It always
 * reports success via the `image-loaded` signal.
 */
void
phosh_background_cache_fetch_background (PhoshBackgroundCache *self,
                                         GFile                *file,
                                         GCancellable         *cancel)
{
  PhoshBackgroundImage *image;

  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (cancel == NULL || G_IS_CANCELLABLE (cancel));

  image = g_hash_table_lookup (self->background_images, file);
  if (image) {
    g_debug ("Background cache hit for %s", g_file_peek_path (file));
    g_signal_emit (self, signals[IMAGE_PRESENT], 0, image);
  } else {
    g_debug ("Background cache miss for %s", g_file_peek_path (file));
    phosh_background_image_new (file, cancel, on_background_image_loaded, self);
  }
}

/**
 * phosh_background_cache_lookup_background:
 * @self: The background cache
 * @file: The file to lookup
 *
 * Looks up an image in the cache. If missing returns %NULL.
 *
 * Returns:(transfer none)(nullable): The looked up background
 */
PhoshBackgroundImage *
phosh_background_cache_lookup_background (PhoshBackgroundCache *self, GFile *file)
{
  g_return_val_if_fail (PHOSH_IS_BACKGROUND_CACHE (self), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return g_hash_table_lookup (self->background_images, file);
}

void
phosh_background_cache_clear_all (PhoshBackgroundCache *self)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));

  g_debug ("Clearing background image cache");
  g_hash_table_remove_all (self->background_images);
}
