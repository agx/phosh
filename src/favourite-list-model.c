/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 * 
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-favourite-list-model"

#define FAVOURITES_KEY "favorites"

#include "favourite-list-model.h"

#include <gio/gio.h>


typedef struct _PhoshFavouriteListModelPrivate PhoshFavouriteListModelPrivate;
struct _PhoshFavouriteListModelPrivate {
  // The complete list as stored in @settings
  GStrv items_inc_missing;

  // The sanitised list
  GStrv items;
  // Cached length of @items
  guint len;

  GSettings *settings;
};

static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshFavouriteListModel, phosh_favourite_list_model, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (PhoshFavouriteListModel)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))

static void
phosh_favourite_list_model_finalize (GObject *object)
{
  PhoshFavouriteListModel *self = PHOSH_FAVOURITE_LIST_MODEL (object);
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (self);

  g_clear_object (&priv->settings);

  g_clear_pointer (&priv->items_inc_missing, g_strfreev);
  g_clear_pointer (&priv->items, g_strfreev);

  G_OBJECT_CLASS (phosh_favourite_list_model_parent_class)->finalize (object);
}

static void
phosh_favourite_list_model_class_init (PhoshFavouriteListModelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = phosh_favourite_list_model_finalize;
}

static GType
list_get_item_type (GListModel *list)
{
  return G_TYPE_APP_INFO;
}

static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshFavouriteListModel *self = PHOSH_FAVOURITE_LIST_MODEL (list);
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (self);

  if (position >= priv->len) {
    return NULL;
  }

  return g_desktop_app_info_new (priv->items[position]);
}

static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshFavouriteListModel *self = PHOSH_FAVOURITE_LIST_MODEL (list);
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (self);

  return priv->len;
}

static void
list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_item = list_get_item;
  iface->get_n_items = list_get_n_items;
}

static void
favorites_changed (GSettings               *settings,
                   const gchar             *key,
                   PhoshFavouriteListModel *self)
{
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (self);
  int removed;
  int added = 0;
  int new_length = 0;
  int i = 0;

  // Clear the old items
  removed = priv->len;

  g_clear_pointer (&priv->items_inc_missing, g_strfreev);
  g_clear_pointer (&priv->items, g_strfreev);

  // Get the new list
  priv->items_inc_missing = g_settings_get_strv (settings, key);
  new_length = g_strv_length (priv->items_inc_missing);

  priv->items = g_new (char *, new_length + 1);

  while (priv->items_inc_missing[i]) {
    g_autoptr (GDesktopAppInfo) info = NULL;
    
    // We don't actually care about this value, just that it isn't NULL
    info = g_desktop_app_info_new (priv->items_inc_missing[i]);

    if (G_LIKELY (info != NULL)) {
      priv->items[added] = g_strdup (priv->items_inc_missing[i]);
      added++;
    } else {
      g_debug ("Missing favourite %s, skipping", priv->items_inc_missing[i]);
    }

    i++;
  }
  priv->items[added] = NULL;

  priv->len = added;

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
}


static void
phosh_favourite_list_model_init (PhoshFavouriteListModel *self)
{
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (self);

  priv->items_inc_missing = NULL;
  priv->items = NULL;
  priv->len = 0;

  priv->settings = g_settings_new ("sm.puri.phosh");
  g_signal_connect (priv->settings, "changed::" FAVOURITES_KEY,
                    G_CALLBACK (favorites_changed), self);
  favorites_changed (priv->settings, FAVOURITES_KEY, self);
}

/**
 * phosh_favourite_list_model_get_default:
 *
 * Returns: (transfer none): The global #PhoshFavouriteListModel singleton
 */
PhoshFavouriteListModel *
phosh_favourite_list_model_get_default (void)
{
  static PhoshFavouriteListModel *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_FAVOURITE_LIST_MODEL, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}

/**
 * phosh_favourite_list_model_app_is_favourite:
 * @self: (nullable): the #PhoshFavouriteListModel, use %NULL for the default
 * @app: a #GAppInfo to lookup
 *
 * Returns: %TRUE if @app if currently favourited, otherwise %FALSE
 */
gboolean
phosh_favourite_list_model_app_is_favourite (PhoshFavouriteListModel *self,
                                             GAppInfo                *app)
{
  PhoshFavouriteListModel *list = self != NULL ? self : phosh_favourite_list_model_get_default ();
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (list);
  const char *id;

  g_return_val_if_fail (G_IS_APP_INFO (app), FALSE);

  id = g_app_info_get_id (app);

  if (G_UNLIKELY (id == NULL)) {
    return FALSE;
  }

  if (g_strv_contains ((const gchar* const*) priv->items_inc_missing, id)) {
    return TRUE;
  }

  return FALSE;
}

void
phosh_favourite_list_model_add_app (PhoshFavouriteListModel *self,
                                    GAppInfo                *app)
{
  PhoshFavouriteListModel *list = self != NULL ? self : phosh_favourite_list_model_get_default ();
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (list);
  const char *id;
  int old_length = 0;
  g_auto(GStrv) new_favourites = NULL;

  g_return_if_fail (G_IS_APP_INFO (app));

  id = g_app_info_get_id (app);

  if (G_UNLIKELY (id == NULL)) {
    g_critical ("Can't add `%p`, doesn't have an id", app);

    return;
  }

  old_length = g_strv_length (priv->items_inc_missing);

  new_favourites = g_new (char *, old_length + 2);

  for (int i = 0; i < old_length; i++) {
    // Avoid having the same favourite twice
    if (G_UNLIKELY (g_strcmp0 (priv->items_inc_missing[i], id) == 0)) {
      g_warning ("%s is already a favourite", id);

      return;
    }
    new_favourites[i] = g_strdup (priv->items_inc_missing[i]);
  }
  // Add the new id
  new_favourites[old_length] = g_strdup (id);
  new_favourites[old_length + 1] = NULL;

  // Indirectly calls favorites_changed which updates the model
  g_settings_set_strv (priv->settings,
                       FAVOURITES_KEY,
                       (const char *const *) new_favourites);
}

void
phosh_favourite_list_model_remove_app (PhoshFavouriteListModel *self,
                                       GAppInfo                *app)
{
  PhoshFavouriteListModel *list = self != NULL ? self : phosh_favourite_list_model_get_default ();
  PhoshFavouriteListModelPrivate *priv = phosh_favourite_list_model_get_instance_private (list);
  const char *id;
  int old_length = 0;
  int new_idx = 0;
  int old_idx = 0;
  g_auto(GStrv) new_favourites = NULL;

  g_return_if_fail (G_IS_APP_INFO (app));

  id = g_app_info_get_id (app);

  if (G_UNLIKELY (id == NULL)) {
    g_critical ("Can't remove `%p`, doesn't have an id", app);

    return;
  }

  old_length = g_strv_length (priv->items_inc_missing);

  new_favourites = g_new (char *, old_length + 1);

  while (priv->items_inc_missing[old_idx]) {
    // Skip over the favourite being removed
    if (G_LIKELY (g_strcmp0 (priv->items_inc_missing[old_idx], id) != 0)) {
      new_favourites[new_idx] = g_strdup (priv->items_inc_missing[old_idx]);
      new_idx++;
    }
    old_idx++;
  }
  new_favourites[new_idx] = NULL;

  // If we actually removed id then old_idx should be ahead of new_idx
  if (G_UNLIKELY (old_idx <= new_idx)) {
    g_warning ("%s wasn't a favourite", id);

    return;
  }

  // Indirectly calls favorites_changed which updates the model
  g_settings_set_strv (priv->settings,
                       FAVOURITES_KEY,
                       (const char *const *) new_favourites);
}
