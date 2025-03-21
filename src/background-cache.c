/*
 * Copyright (C) 2024-2025 The Phosh Developers
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

struct _PhoshBackgroundCache {
  GObject     parent;

  GHashTable *background_images;
};
G_DEFINE_TYPE (PhoshBackgroundCache, phosh_background_cache, G_TYPE_OBJECT)


static void
on_background_image_loaded (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask) task = G_TASK (user_data);
  PhoshBackgroundCache *self;
  PhoshBackgroundImage *image;
  GError *err = NULL;
  GFile *file;

  image = phosh_background_image_new_finish (res, &err);
  if (!image) {
    g_task_return_error (task, err);
    return;
  }

  self = PHOSH_BACKGROUND_CACHE (g_task_get_source_object (task));
  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));
  file = phosh_background_image_get_file (image);
  g_hash_table_insert (self->background_images, g_object_ref (file), g_object_ref (image));

  g_task_return_pointer (task, g_steal_pointer (&image), g_object_unref);
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
 * Loads an image into the cache if not yet present. It always
 * reports success via the `image-loaded` signal.
 */
void
phosh_background_cache_fetch_async (PhoshBackgroundCache *self,
                                    GFile                *file,
                                    GCancellable         *cancel,
                                    GAsyncReadyCallback   callback,
                                    gpointer              user_data)
{
  PhoshBackgroundImage *image;
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (cancel == NULL || G_IS_CANCELLABLE (cancel));

  task = g_task_new (self, cancel, callback, user_data);
  g_task_set_source_tag (task, phosh_background_cache_fetch_async);

  image = g_hash_table_lookup (self->background_images, file);
  if (image) {
    g_debug ("Background cache hit for %s", g_file_peek_path (file));
    g_task_return_pointer (task, g_object_ref (image), g_object_unref);
  } else {
    g_debug ("Background cache miss for %s", g_file_peek_path (file));
    phosh_background_image_new (file, cancel, on_background_image_loaded, g_steal_pointer (&task));
  }
}

/**
 * phosh_background_cache_fetch_finish:
 * @self: The background cache
 * @res: The Result
 * @cancel: A cancellable
 *
 * Finished the async operation started with `phosh_background_cache_fetch_async`.
 *
 * Returns; The laoded image or `NULL` on error
 */
PhoshBackgroundImage *
phosh_background_cache_fetch_finish (PhoshBackgroundCache *self,
                                     GAsyncResult         *res,
                                     GError              **error)
{
  g_assert (PHOSH_IS_BACKGROUND_CACHE (self));
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);

  return g_task_propagate_pointer (G_TASK (res), error);
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

/**
 * phosh_background_cache_remove:
 * @self: The background cache
 * @file: The background to remove
 *
 * Drop the background identified by the given file from the background cache
 */
void
phosh_background_cache_remove (PhoshBackgroundCache *self, GFile *file)
{
  gboolean success;

  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));

  success = g_hash_table_remove (self->background_images, file);
  if (!success)
    g_warning ("'%s' not found in cache", g_file_peek_path (file));
}

/**
 * phosh_background_cache_clear_all:
 * @self: The background cache
 *
 * Drop all files from the cache.
 */
void
phosh_background_cache_clear_all (PhoshBackgroundCache *self)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND_CACHE (self));

  g_debug ("Clearing background image cache");
  g_hash_table_remove_all (self->background_images);
}
