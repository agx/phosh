/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-meta"

#include "search-result-meta.h"


/**
 * PhoshSearchResultMeta:
 *
 * A low level immutable representation of a search result
 *
 * It is a ref-counted ( g_rc_box_alloc0() ) #GBoxed type (rather than a full
 * #GObject) as a large number may be created and destroyed during a search,
 * when the data is immutable the overhead of #GObject is unnecessary
 *
 * A #PhoshSearchResultMeta can be serialised to a #GVariant for transport
 * over dbus
 *
 * #PhoshSearchResult provides a #GObject wrapper
 */


struct _PhoshSearchResultMeta {
  char  *id;
  char  *title;
  char  *desc;
  char  *clipboard_text;
  GIcon *icon;
};


/**
 * phosh_search_result_meta_ref:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Increase the reference count of @self
 *
 * Returns: (transfer full): @self
 */
PhoshSearchResultMeta *
phosh_search_result_meta_ref (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return g_rc_box_acquire (self);
}


static void
result_free (gpointer source)
{
  PhoshSearchResultMeta *self = source;

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->title, g_free);
  g_clear_pointer (&self->desc, g_free);
  g_clear_object (&self->icon);
  g_clear_pointer (&self->clipboard_text, g_free);
}

/**
 * phosh_search_result_meta_unref:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Decrease the reference count of @self, potentially destroying it
 */
void
phosh_search_result_meta_unref (PhoshSearchResultMeta *self)
{
  g_return_if_fail (self != NULL);

  g_rc_box_release_full (self, result_free);
}


G_DEFINE_BOXED_TYPE (PhoshSearchResultMeta,
                     phosh_search_result_meta,
                     phosh_search_result_meta_ref,
                     phosh_search_result_meta_unref)


/**
 * phosh_search_result_meta_get_id:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Get the result id, note this **should** be unique withing the source of @self
 * but this is not guaranteed
 *
 * Returns: (transfer none): the result "id" of @self
 */
const char *
phosh_search_result_meta_get_id (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->id;
}

/**
 * phosh_search_result_meta_get_title:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Get the result title
 *
 * Returns: (transfer none): the title of @self
 */
const char *
phosh_search_result_meta_get_title (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->title;
}

/**
 * phosh_search_result_meta_get_description:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Get the result description, this optional text providing further
 * information about @self
 *
 * Returns: (transfer none) (nullable): the description of @self
 */
const char *
phosh_search_result_meta_get_description (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->desc;
}

/**
 * phosh_search_result_meta_get_clipboard_text:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Get the clipboard text, this is a string that should be sent to the
 * clipboard when the user activates @self
 *
 * NOTE: This is rarely provided
 *
 * Returns: (transfer none) (nullable): the clipboard text of @self
 */
const char *
phosh_search_result_meta_get_clipboard_text (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->clipboard_text;
}

/**
 * phosh_search_result_meta_get_icon:
 * @self: A #PhoshSearchResultMeta instance.
 *
 * Get the icon, this is a small image representing @self (such as an app icon
 * or file thumbnail)
 *
 * Returns: (transfer none) (nullable): the icon of @self
 */
GIcon *
phosh_search_result_meta_get_icon (PhoshSearchResultMeta *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return self->icon;
}

/**
 * phosh_search_result_meta_new:
 * @id: the result id
 * @title: the title
 * @desc: (optional): the description
 * @icon: (optional): the icon
 * @clipboard: (optional): the clipboard text
 *
 * Construct a new #PhoshSearchResultMeta for the given data
 *
 * The @icon #GIcon implementation **must** support g_icon_serialize()
 *
 * Returns: (transfer full): the new #PhoshSearchResultMeta
 */
PhoshSearchResultMeta *
phosh_search_result_meta_new (const char *id,
                              const char *title,
                              const char *desc,
                              GIcon      *icon,
                              const char *clipboard)
{
  PhoshSearchResultMeta *self = g_rc_box_new0 (PhoshSearchResultMeta);

  self->id = g_strdup (id);
  self->title = g_strdup (title);
  self->desc = g_strdup (desc);
  g_set_object (&self->icon, icon);
  self->clipboard_text = g_strdup (clipboard);

  return self;
}

/**
 * phosh_search_result_meta_serialise:
 * @self: tA #PhoshSearchResultMeta instance.
 *
 * Generate a #GVariant representation of @self suitable for transport over
 * dbus, the exact format is not defined and may change - the variant should
 * only be interpreted with phosh_search_result_meta_deserialise()
 *
 * Returns: (transfer full): a #GVariant representing @self
 */
GVariant *
phosh_search_result_meta_serialise (PhoshSearchResultMeta *self)
{
  g_autoptr (GVariant) icon = NULL;
  GVariantDict dict;

  g_variant_dict_init (&dict, NULL);

  g_variant_dict_insert (&dict, "id", "s", self->id);
  g_variant_dict_insert (&dict, "title", "s", self->title);
  if (self->desc)
    g_variant_dict_insert (&dict, "desc", "s", self->desc);

  if (self->clipboard_text)
    g_variant_dict_insert (&dict, "clipboard-text", "s", self->clipboard_text);

  if (self->icon) {
    icon = g_icon_serialize (self->icon);
    if (icon)
      g_variant_dict_insert_value (&dict, "icon", icon);
    else
      g_warning ("Can't serialise icon of type %s", G_OBJECT_TYPE_NAME (self->icon));
  }

  return g_variant_dict_end (&dict);
}

/**
 * phosh_search_result_meta_deserialise:
 * @variant: The #GVariant to deserialise
 *
 * Convert a #GVariant from phosh_search_result_meta_serialise() back to it's
 * #PhoshSearchResultMeta, or %NULL for invalid input
 *
 * Returns: (transfer full) (nullable): the deserialised #PhoshSearchResultMeta
 *          or %NULL on failure
 */
PhoshSearchResultMeta *
phosh_search_result_meta_deserialise (GVariant *variant)
{
  PhoshSearchResultMeta *self = NULL;
  g_autofree char *id = NULL;
  g_autofree char *title = NULL;
  g_autofree char *desc = NULL;
  g_autoptr (GVariant) icon_data = NULL;
  g_autofree char *clipboard_text = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  dict = g_variant_dict_new (variant);

  g_variant_dict_lookup (dict, "id", "s", &id);
  g_variant_dict_lookup (dict, "title", "s", &title);
  g_variant_dict_lookup (dict, "desc", "s", &desc);
  g_variant_dict_lookup (dict, "clipboard-text", "s", &clipboard_text);
  icon_data = g_variant_dict_lookup_value (dict, "icon", G_VARIANT_TYPE_ANY);

  if (icon_data)
    icon = g_icon_deserialize (icon_data);

  self = phosh_search_result_meta_new (id, title, desc, icon, clipboard_text);

  return self;
}
