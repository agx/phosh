/*
 * Copyright (C) 2025 The Phosh Authors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-metainfo-cache"

#include "config.h"
#include "metainfo-cache.h"
#include "util.h"

#include <appstream/appstream.h>

#include <gio/gio.h>

enum {
  PROP_0,
  PROP_READY,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

enum {
  CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

/**
 * PhoshMetainfoCache:
 *
 * Cache for application meta information
 */
typedef struct _PhoshMetainfoCache {
  GObject       parent;

  AsPool       *as_pool;
  GCancellable *cancel;
  gboolean      ready;
} PhoshMetainfoCache;


G_DEFINE_TYPE (PhoshMetainfoCache, phosh_metainfo_cache, G_TYPE_OBJECT)


static void
on_as_pool_changed (PhoshMetainfoCache *self)
{
  g_debug ("Metainfo pool changed");
  g_signal_emit (self, signals[CHANGED], 0);
}


static void
on_as_pool_load_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
  gboolean success;
  PhoshMetainfoCache *self;
  g_autoptr (GError) err = NULL;

  success = as_pool_load_finish (AS_POOL (source_object), res, &err);
  if (!success) {
    phosh_async_error_warn (err, "Failed to load metainfo pool");
    return;
  }

  g_return_if_fail (PHOSH_IS_METAINFO_CACHE (user_data));
  self = PHOSH_METAINFO_CACHE (user_data);

  g_signal_connect_object (self->as_pool, "changed", G_CALLBACK (on_as_pool_changed), self,
                           G_CONNECT_SWAPPED);

  self->ready = TRUE;
  g_debug ("Metainfo cache loaded");
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_READY]);
}


static void
phosh_metainfo_cache_constructed (GObject *object)
{
  PhoshMetainfoCache *self = PHOSH_METAINFO_CACHE (object);

  G_OBJECT_CLASS (phosh_metainfo_cache_parent_class)->constructed (object);

  self->as_pool = as_pool_new ();
  /* We only bother about metainfo in a non-system cache */
  as_pool_set_flags (self->as_pool,
                     AS_POOL_FLAG_LOAD_OS_CATALOG | AS_POOL_FLAG_LOAD_OS_DESKTOP_FILES |
                     AS_POOL_FLAG_LOAD_OS_METAINFO | AS_POOL_FLAG_LOAD_FLATPAK |
                     AS_POOL_FLAG_MONITOR);

  as_pool_load_async (self->as_pool, self->cancel,
                      on_as_pool_load_ready,
                      self);
}


static void
phosh_metainfo_cache_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshMetainfoCache *self = PHOSH_METAINFO_CACHE (object);

  switch (property_id) {
  case PROP_READY:
    g_value_set_boolean (value, self->ready);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_metainfo_cache_dispose (GObject *object)
{
  PhoshMetainfoCache *self = PHOSH_METAINFO_CACHE (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->as_pool);

  G_OBJECT_CLASS (phosh_metainfo_cache_parent_class)->dispose (object);
}


static void
phosh_metainfo_cache_class_init (PhoshMetainfoCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_metainfo_cache_constructed;
  object_class->get_property = phosh_metainfo_cache_get_property;
  object_class->dispose = phosh_metainfo_cache_dispose;

  /**
   * PhoshMetainfoCache:ready:
   *
   * Whether the cache is ready to use
   */
  props[PROP_READY] =
    g_param_spec_boolean ("ready", "", "", FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  signals[CHANGED] = g_signal_new ("changed",
                                   G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                   NULL, G_TYPE_NONE, 0);
}


static void
phosh_metainfo_cache_init (PhoshMetainfoCache *self)
{
  self->cancel = g_cancellable_new ();
}


PhoshMetainfoCache *
phosh_metainfo_cache_get_default (void)
{
  static PhoshMetainfoCache *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_METAINFO_CACHE, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}

/**
 * phosh_metainfo_get_data_id:
 * @app_id: desktop app id
 *
 * Returns (transfer full): the 5-part AppStream identifier or NULL
 */
char *
phosh_metainfo_get_data_id (PhoshMetainfoCache *self, const char *app_id)
{
  g_autoptr (AsComponentBox) result = NULL;

  result = as_pool_get_components_by_launchable (self->as_pool, AS_LAUNCHABLE_KIND_DESKTOP_ID,
                                                 app_id);

  if (!result || as_component_box_is_empty (result))
    return NULL;

  return g_strdup (as_component_get_data_id (as_component_box_index_safe (result, 0)));
}
