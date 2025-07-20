/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gdesktopappinfo.h>

#include "search-source.h"


/**
 * PhoshSearchSource:
 *
 * A ref-counted (#g_rc_box_alloc0()) #GBoxed type containing information
 * about a search source.
 *
 * #PhoshSearchSource provides details about a specific search source.
 */


struct _PhoshSearchSource {
  char     *id;
  GAppInfo *app_info;
  guint     position;
};


G_DEFINE_BOXED_TYPE (PhoshSearchSource,
                     phosh_search_source,
                     phosh_search_source_ref,
                     phosh_search_source_unref)


/**
 * phosh_search_source_ref:
 * @self: the #PhoshSearchSource
 *
 * Increase the reference count of @self
 *
 * Returns: (transfer full): @self
 */
PhoshSearchSource *
phosh_search_source_ref (PhoshSearchSource *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return g_rc_box_acquire (self);
}


static void
search_source_free (gpointer source)
{
  PhoshSearchSource *self = source;

  g_clear_pointer (&self->id, g_free);
  g_clear_object (&self->app_info);
}

/**
 * phosh_search_source_unref:
 * @self: the #PhoshSearchSource
 *
 * Decrease the reference count of @self, potentially destroying it
 */
void
phosh_search_source_unref (PhoshSearchSource *self)
{
  g_return_if_fail (self != NULL);

  g_rc_box_release_full (self, search_source_free);
}

/**
 * phosh_search_source_new:
 * @id: the source id (currently the bus path, DO NOT rely on this)
 * @app_info: #GAppInfo with information about the source
 *
 * Creat a new #PhoshSearchSource with the given @app_info and @id
 *
 * Returns: (transfer full): the new #PhoshSearchSource
 */
PhoshSearchSource *
phosh_search_source_new (const char *id, GAppInfo *app_info)
{
  PhoshSearchSource *self = g_rc_box_new0 (PhoshSearchSource);

  self->id = g_strdup (id);
  g_set_object (&self->app_info, app_info);
  self->position = 0;

  return self;
}

/**
 * phosh_search_source_get_id:
 * @self: the #PhoshSearchSource
 *
 * Get the unique id of the source
 *
 * Returns: (transfer none): the source id of @self
 */
const char *
phosh_search_source_get_id (PhoshSearchSource *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->id;
}

/**
 * phosh_search_source_get_app_info:
 * @self: the #PhoshSearchSource
 *
 * Get the #GAppInfo associated with the source, this contains the name
 * and #GIcon of the source
 *
 * Returns: (transfer none): the app info of @self
 */
GAppInfo *
phosh_search_source_get_app_info (PhoshSearchSource *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->app_info;
}

/**
 * phosh_search_source_get_position:
 * @self: the #PhoshSearchSource
 *
 * Get the sort position of the source
 *
 * Returns: the sort position of @self
 */
guint
phosh_search_source_get_position (PhoshSearchSource *self)
{
  g_return_val_if_fail (self != NULL, 0);

  return self->position;
}

/**
 * phosh_search_source_set_position:
 * @self: the #PhoshSearchSource
 * @position: the sort position of the index
 *
 * The is only for use in the search daemon
 */
void
phosh_search_source_set_position (PhoshSearchSource *self, guint position)
{
  g_return_if_fail (self != NULL);

  self->position = position;
}

/**
 * phosh_search_source_serialise:
 * @self: the #PhoshSearchSource
 *
 * Generate a #GVariant representation of @self suitable for transport over
 * dbus, the exact format is not defined and may change - the variant should
 * only be interpreted with phosh_search_source_deserialise()
 *
 * Returns: (transfer full): a #GVariant representing @self
 */
GVariant *
phosh_search_source_serialise (PhoshSearchSource *self)
{
  return g_variant_new ("(ssu)", self->id, g_app_info_get_id (self->app_info), self->position);
}

/**
 * phosh_search_source_deserialise:
 * @variant: the #GVariant to deserialise
 *
 * Convert a #GVariant from phosh_search_source_serialise() back to it's
 * #PhoshSearchSource, or %NULL for invalid input
 *
 * Returns: (transfer full) (nullable): the deserialised #PhoshSearchSource
 *          or %NULL on failure
 */
PhoshSearchSource *
phosh_search_source_deserialise (GVariant *variant)
{
  PhoshSearchSource *self = NULL;
  g_autofree char *id = NULL;
  g_autofree char *app_info_id = NULL;
  g_autoptr (GAppInfo) app_info = NULL;
  guint position;

  g_variant_get (variant, "(ssu)", &id, &app_info_id, &position);

  app_info = G_APP_INFO (g_desktop_app_info_new (app_info_id));

  self = phosh_search_source_new (id, app_info);
  phosh_search_source_set_position (self, position);

  return self;
}
