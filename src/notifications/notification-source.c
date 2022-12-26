/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-source"

#include "phosh-config.h"
#include "notification-source.h"

/**
 * PhoshNotificationSource:
 *
 * A #GListModel containing one or more notifications
 *
 * A #PhoshNotificationSource groups notifications. A source has a name
 * which is usually the app_id of the sending application.
 */


typedef struct _PhoshNotificationSource {
  GObject     parent;

  GListStore *list;

  char       *name;
} PhoshNotificationSource;


static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshNotificationSource, phosh_notification_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))


enum {
  PROP_0,
  PROP_NAME,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


enum {
  SIGNAL_EMPTY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
phosh_notification_source_finalize (GObject *object)
{
  PhoshNotificationSource *self = PHOSH_NOTIFICATION_SOURCE (object);

  g_clear_object (&self->list);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (phosh_notification_source_parent_class)->finalize (object);
}


static void
phosh_notification_source_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                       GParamSpec   *pspec)
{
  PhoshNotificationSource *self = PHOSH_NOTIFICATION_SOURCE (object);

  switch (property_id) {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_source_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  PhoshNotificationSource *self = PHOSH_NOTIFICATION_SOURCE (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_source_class_init (PhoshNotificationSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_notification_source_finalize;
  object_class->set_property = phosh_notification_source_set_property;
  object_class->get_property = phosh_notification_source_get_property;

  props[PROP_NAME] =
    g_param_spec_string (
      "name",
      "Name",
      "Source name (ID)",
      NULL,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  signals[SIGNAL_EMPTY] = g_signal_new ("empty",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        NULL,
                                        G_TYPE_NONE,
                                        0);
}


static GType
list_get_item_type (GListModel *list)
{
  return PHOSH_TYPE_NOTIFICATION;
}


static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshNotificationSource *self = PHOSH_NOTIFICATION_SOURCE (list);

  return g_list_model_get_item (G_LIST_MODEL (self->list), position);
}


static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshNotificationSource *self = PHOSH_NOTIFICATION_SOURCE (list);

  return g_list_model_get_n_items (G_LIST_MODEL (self->list));
}


static void
list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_item = list_get_item;
  iface->get_n_items = list_get_n_items;
}


static void
items_changed (GListModel              *list,
               guint                    position,
               guint                    removed,
               guint                    added,
               PhoshNotificationSource *self)
{
  g_autoptr (PhoshNotification) item = NULL;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_SOURCE (self));

  g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);

  item = g_list_model_get_item (G_LIST_MODEL (self->list), 0);

  if (item == NULL) {
    g_signal_emit (self, signals[SIGNAL_EMPTY], 0);
  }
}


static void
phosh_notification_source_init (PhoshNotificationSource *self)
{
  self->list = g_list_store_new (PHOSH_TYPE_NOTIFICATION);

  g_signal_connect (self->list,
                    "items-changed",
                    G_CALLBACK (items_changed),
                    self);
}


PhoshNotificationSource *
phosh_notification_source_new (const char *name)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_SOURCE,
                       "name", name,
                       NULL);
}


static void
closed (PhoshNotificationSource *self,
        PhoshNotificationReason  reason,
        PhoshNotification       *notification)
{
  int i = 0;
  gpointer item = NULL;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_SOURCE (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  while ((item = g_list_model_get_item (G_LIST_MODEL (self->list), i))) {
    if (item == notification) {
      g_list_store_remove (self->list, i);
      g_clear_object (&item);
      found = TRUE;
      g_clear_object (&item);
      break;
    }
    g_clear_object (&item);
    i++;
    g_clear_object (&item);
  }

  if (!found) {
    g_critical ("Can't remove unknown notification %p", notification);
  }
}


void
phosh_notification_source_add (PhoshNotificationSource *self,
                               PhoshNotification       *notification)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION_SOURCE (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  g_list_store_insert (self->list, 0, notification);

  g_signal_connect_object (notification,
                           "closed",
                           G_CALLBACK (closed),
                           self,
                           G_CONNECT_SWAPPED);
}


const char *
phosh_notification_source_get_name (PhoshNotificationSource *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_SOURCE (self), NULL);

  return self->name;
}
