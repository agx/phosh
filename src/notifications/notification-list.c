/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-list"

#include "phosh-config.h"
#include "notification-source.h"
#include "notification-list.h"

/**
 * PhoshNotificationList:
 *
 * A list containing one or more #PhoshNotificationSource
 *
 * #PhoshNotificationList maps between #PhoshNotificationSource objects and their
 * notifications creating and removing sources on the fly.
 */


struct _PhoshNotificationList {
  GObject     parent;

  /* Ordered list of notification sources */
  GSequence  *source_list;

  /* cache for fast get_item */
  struct {
    gboolean       is_valid;
    guint          position;
    GSequenceIter *iter;
  } last;

  /* Map of source name -> iter in source_list */
  GHashTable *source_map;

  /* Map of id -> notification */
  GHashTable *notifications;
};
typedef struct _PhoshNotificationList PhoshNotificationList;


static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshNotificationList, phosh_notification_list, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))


static void
phosh_notification_list_finalize (GObject *object)
{
  PhoshNotificationList *self = PHOSH_NOTIFICATION_LIST (object);

  g_clear_pointer (&self->source_list, g_sequence_free);
  g_clear_pointer (&self->source_map, g_hash_table_unref);

  g_clear_pointer (&self->notifications, g_hash_table_unref);

  G_OBJECT_CLASS (phosh_notification_list_parent_class)->finalize (object);
}


static void
phosh_notification_list_class_init (PhoshNotificationListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_notification_list_finalize;
}


static GType
list_get_item_type (GListModel *list)
{
  return PHOSH_TYPE_NOTIFICATION_SOURCE;
}


static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshNotificationList *self = PHOSH_NOTIFICATION_LIST (list);
  GSequenceIter *it = NULL;

  if (self->last.is_valid) {
    if (position < G_MAXUINT && self->last.position == position + 1)
      it = g_sequence_iter_prev (self->last.iter);
    else if (position > 0 && self->last.position == position - 1)
      it = g_sequence_iter_next (self->last.iter);
    else if (self->last.position == position)
      it = self->last.iter;
  }

  if (it == NULL)
    it = g_sequence_get_iter_at_pos (self->source_list, position);

  self->last.iter = it;
  self->last.position = position;
  self->last.is_valid = TRUE;

  if (g_sequence_iter_is_end (it))
    return NULL;
  else
    return g_object_ref (g_sequence_get (it));
}


static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshNotificationList *self = PHOSH_NOTIFICATION_LIST (list);

  return g_sequence_get_length (self->source_list);
}


static void
list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_item = list_get_item;
  iface->get_n_items = list_get_n_items;
}


static void
phosh_notification_list_init (PhoshNotificationList *self)
{
  /* The source list owns its elements */
  self->source_list = g_sequence_new ((GDestroyNotify) g_object_unref);

  /* The source map owns its keys, but values belong to source_list */
  self->source_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* The notifications are owened but the source they are in */
  self->notifications = g_hash_table_new_full (g_direct_hash,
                                               g_direct_equal,
                                               NULL,
                                               NULL);
}


/**
 * phosh_notification_list_new:
 *
 * Create an empty #PhoshNotificationList, generally used via
 * phosh_notify_manager_get_list()
 *
 * Returns: the new #PhoshNotificationList
 */
PhoshNotificationList *
phosh_notification_list_new (void)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_LIST, NULL);
}


/**
 * empty:
 * @source: the #PhoshNotificationSource
 * @self: the #PhoshNotificationList that owns @source
 *
 * The @source now contains no items, remove it from @self
 */
static void
empty (PhoshNotificationSource *source,
       PhoshNotificationList   *self)
{
  int i = 0;
  GSequenceIter *iter;
  const char *source_id;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_LIST (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION_SOURCE (source));

  source_id = phosh_notification_source_get_name (source);

  iter = g_hash_table_lookup (self->source_map, source_id);

  i = g_sequence_iter_get_position (iter);

  g_sequence_remove (iter);

  g_hash_table_remove (self->source_map, source_id);

  self->last.is_valid = FALSE;
  self->last.position = 0;
  self->last.iter = NULL;

  g_list_model_items_changed (G_LIST_MODEL (self), i, 1, 0);
}


/**
 * closed:
 * @notification: the #PhoshNotification
 * @reason: the #PhoshNotificationReason @notification closed
 * @self: the #PhoshNotificationList the @notification is (indirectly) in
 *
 * @notification has been closed, remove it from the notification map
 *
 * NOTE: We don't remove it from it's #PhoshNotificationSource, that's left
 * for the source to handle
 */
static void
closed (PhoshNotification       *notification,
        PhoshNotificationReason  reason,
        PhoshNotificationList   *self)
{
  guint id;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_LIST (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  id = phosh_notification_get_id (notification);

  g_hash_table_remove (self->notifications, GUINT_TO_POINTER (id));
}


/**
 * phosh_notification_list_add:
 * @self: the #PhoshNotificationList
 * @source_id: id of the #PhoshNotificationSource @notification belongs to
 *             (may not currently exist)
 * @notification: the new #PhoshNotification
 *
 * Registers a new notification with @self adding to (or creating) the relevant
 * #PhoshNotificationSource
 */
void
phosh_notification_list_add (PhoshNotificationList *self,
                             const char            *source_id,
                             PhoshNotification     *notification)
{
  PhoshNotificationSource *source;
  GSequenceIter *source_iter;
  guint id;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_LIST (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  id = phosh_notification_get_id (notification);

  g_hash_table_insert (self->notifications,
                       GUINT_TO_POINTER (id),
                       notification);

  /* Lookup an existing entry for source id */
  source_iter = g_hash_table_lookup (self->source_map, source_id);

  if (source_iter == NULL) {
    /* Source doesn't currently exist, generate it */
    source = phosh_notification_source_new (source_id);
    g_signal_connect (source, "empty", G_CALLBACK (empty), self);

    /* Add to the start, iter remains valid as long as the item exist */
    source_iter = g_sequence_prepend (self->source_list, source);

    g_hash_table_insert (self->source_map, g_strdup (source_id), source_iter);

    self->last.is_valid = FALSE;
    self->last.iter = NULL;
    self->last.position = 0;

    /* We added an item to the start */
    g_list_model_items_changed (G_LIST_MODEL (self), 0, 0, 1);
  } else if (!g_sequence_iter_is_begin (source_iter)) {
    int old_pos;

    source = g_sequence_get (source_iter);

    old_pos = g_sequence_iter_get_position (source_iter);

    /* Move the source to the top of the list */
    g_sequence_move (source_iter,
                     g_sequence_get_begin_iter (self->source_list));

    self->last.is_valid = FALSE;
    self->last.iter = NULL;
    self->last.position = 0;

    /* We "removed" an item */
    g_list_model_items_changed (G_LIST_MODEL (self), old_pos, 1, 0);
    /* And "added" it to the start */
    g_list_model_items_changed (G_LIST_MODEL (self), 0, 0, 1);
  } else {
    source = g_sequence_get (source_iter);
  }

  phosh_notification_source_add (source, notification);

  g_signal_connect (notification, "closed", G_CALLBACK (closed), self);
}


/**
 * phosh_notification_list_get_by_id:
 * @self: the #PhoshNotificationList
 * @id: the #PhoshNotification:id to lookup
 *
 * Find a #PhoshNotification in @self by it's @id
 *
 * Returns:(nullable)(transfer none): the #PhoshNotification or %NULL
 */
PhoshNotification *
phosh_notification_list_get_by_id (PhoshNotificationList *self,
                                   guint                  id)
{
  PhoshNotification *notification = NULL;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_LIST (self), NULL);

  notification = g_hash_table_lookup (self->notifications,
                                      GUINT_TO_POINTER (id));

  return notification;
}
