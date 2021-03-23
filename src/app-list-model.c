/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Inspired by gliststore.c:
 *     Copyright 2015 Lars Uebernickel
 *     Copyright 2015 Ryan Lortie
 * https://gitlab.gnome.org/GNOME/glib/blob/713fec9dcb1ee49c4f64bbb6f483a5cd1db9966a/gio/gliststore.c
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "app-list-model.h"

#include <gio/gio.h>


typedef struct _PhoshAppListModelPrivate PhoshAppListModelPrivate;
struct _PhoshAppListModelPrivate {
  GAppInfoMonitor *monitor;

  GSequence *items;

  gulong debounce;

  /* cache */
  struct {
    gboolean       is_valid;
    guint          position;
    GSequenceIter *iter;
  } last;
};

static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshAppListModel, phosh_app_list_model, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (PhoshAppListModel)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))


static void
phosh_app_list_model_finalize (GObject *object)
{
  PhoshAppListModel *self = PHOSH_APP_LIST_MODEL (object);
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);

  g_clear_object (&priv->monitor);

  g_sequence_free (priv->items);

  G_OBJECT_CLASS (phosh_app_list_model_parent_class)->finalize (object);
}


static void
phosh_app_list_model_class_init (PhoshAppListModelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = phosh_app_list_model_finalize;
}


static GType
list_get_item_type (GListModel *list)
{
  return G_TYPE_APP_INFO;
}


static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshAppListModel *self = PHOSH_APP_LIST_MODEL (list);
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);
  GSequenceIter *it = NULL;

  if (priv->last.is_valid)
    {
      if (position < G_MAXUINT && priv->last.position == position + 1)
        it = g_sequence_iter_prev (priv->last.iter);
      else if (position > 0 && priv->last.position == position - 1)
        it = g_sequence_iter_next (priv->last.iter);
      else if (priv->last.position == position)
        it = priv->last.iter;
    }

  if (it == NULL)
    it = g_sequence_get_iter_at_pos (priv->items, position);

  priv->last.iter = it;
  priv->last.position = position;
  priv->last.is_valid = TRUE;

  if (g_sequence_iter_is_end (it))
    return NULL;
  else
    return g_object_ref (g_sequence_get (it));

}


static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshAppListModel *self = PHOSH_APP_LIST_MODEL (list);
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);

  return g_sequence_get_length (priv->items);
}


static void
list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_item = list_get_item;
  iface->get_n_items = list_get_n_items;
}


static gboolean
items_changed (gpointer data)
{
  PhoshAppListModel *self = PHOSH_APP_LIST_MODEL (data);
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);
  g_autolist(GAppInfo) new_apps = NULL;
  int removed;
  int added = 0;

  new_apps = g_app_info_get_all ();

  g_return_val_if_fail (new_apps != NULL, G_SOURCE_REMOVE);

  removed = g_sequence_get_length (priv->items);

  g_sequence_remove_range (g_sequence_get_begin_iter (priv->items),
                           g_sequence_get_end_iter (priv->items));

  for (GList *l = new_apps; l; l = g_list_next (l)) {
    if (!g_app_info_should_show (G_APP_INFO (l->data))) {
      continue;
    }
    g_sequence_append (priv->items, g_object_ref (l->data));
    added++;
  }

  priv->last.is_valid = FALSE;
  priv->last.iter = NULL;
  priv->last.position = 0;

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);

  priv->debounce = 0;

  return G_SOURCE_REMOVE;
}


static void
on_monitor_changed_cb (GAppInfoMonitor *monitor,
                       gpointer         data)
{
  PhoshAppListModel *self = PHOSH_APP_LIST_MODEL (data);
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);

  if (priv->debounce != 0) {
    g_source_remove (priv->debounce);
  }
  priv->debounce = g_timeout_add (500, items_changed, data);
  g_source_set_name_by_id (priv->debounce, "[phosh] debounce app changes");
}


static void
phosh_app_list_model_init (PhoshAppListModel *self)
{
  PhoshAppListModelPrivate *priv = phosh_app_list_model_get_instance_private (self);

  priv->debounce = 0;

  priv->last.is_valid = FALSE;

  priv->items = g_sequence_new ((GDestroyNotify) g_object_unref);
  priv->monitor = g_app_info_monitor_get ();
  g_signal_connect (priv->monitor, "changed", G_CALLBACK (on_monitor_changed_cb), self);

  on_monitor_changed_cb (priv->monitor, self);
}


/**
 * phosh_app_list_model_get_default:
 *
 * Return Value: (transfer none): The global #PhoshAppListModel singleton
 */
PhoshAppListModel *
phosh_app_list_model_get_default (void)
{
  static PhoshAppListModel *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_APP_LIST_MODEL, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}
