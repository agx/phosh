/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notify-manager"

#include "config.h"

#include "notification.h"
#include "notify-manager.h"
#include "shell.h"

#define NOTIFICATION_DEFAULT_TIMEOUT 5000 /* ms */
#define NOTIFICATIONS_SPEC_VERSION "1.2"

/**
 * SECTION:phosh-notify-manager
 * @short_description: Provides the org.freedesktop.Notification DBus interface
 * @Title: PhoshNotifyManager
 * See https://developer.gnome.org/notification-spec/
 */

#define NOTIFY_DBUS_NAME "org.freedesktop.Notifications"

static void phosh_notify_manager_notify_iface_init (
  PhoshNotifyDbusNotificationsIface *iface);

typedef struct _PhoshNotifyManager
{
  PhoshNotifyDbusNotificationsSkeleton parent;

  int dbus_name_id;
  guint next_id;

  GHashTable *notifications;
} PhoshNotifyManager;

G_DEFINE_TYPE_WITH_CODE (PhoshNotifyManager,
                         phosh_notify_manager,
                         PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS,
                           phosh_notify_manager_notify_iface_init));

static void
phosh_notify_manager_set_property (GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_notify_manager_get_property (GObject *object,
                                         guint property_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
handle_close_notification (PhoshNotifyDbusNotifications *skeleton,
                           GDBusMethodInvocation        *invocation,
                           guint                         arg_id)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);
  g_debug ("DBus call CloseNotification %u", arg_id);

  if (!phosh_notify_manager_close_notification (self, arg_id,
                                                PHOSH_NOTIFY_MANAGER_REASON_CLOSED)) {
    return FALSE;
  }

  phosh_notify_dbus_notifications_complete_close_notification (
    skeleton, invocation);

  return TRUE;
}


static gboolean
handle_get_capabilities (PhoshNotifyDbusNotifications *skeleton,
                         GDBusMethodInvocation        *invocation)
{
  const gchar *const capabilities[] = {
    "body", "icon-static", NULL,
  };

  g_debug ("DBus call GetCapabilities");
  phosh_notify_dbus_notifications_complete_get_capabilities (
    skeleton, invocation, capabilities);
  return TRUE;
}


static gboolean
handle_get_server_information (PhoshNotifyDbusNotifications *skeleton,
                               GDBusMethodInvocation        *invocation)
{
  g_debug ("DBus call GetServerInformation");
  phosh_notify_dbus_notifications_complete_get_server_information (
    skeleton, invocation, "Phosh Notify Daemon", "Phosh", PHOSH_VERSION,
    NOTIFICATIONS_SPEC_VERSION);
  return TRUE;
}


static gboolean
on_notification_expired (gpointer data)
{
  guint id = GPOINTER_TO_UINT (data);
  PhoshNotifyManager *self = phosh_notify_manager_get_default ();

  g_debug ("Notification %u expired", id);
  phosh_notify_manager_close_notification (self, id,
                                           PHOSH_NOTIFY_MANAGER_REASON_EXPIRED);

  return G_SOURCE_REMOVE;
}

static void
on_notification_dismissed (PhoshNotifyManager *self, PhoshNotification *notification)
{
  gpointer data;

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  data = g_object_get_data (G_OBJECT (notification), "notify-id");
  g_return_if_fail (data);

  phosh_notify_manager_close_notification (self, GPOINTER_TO_UINT(data),
                                           PHOSH_NOTIFY_MANAGER_REASON_DISMISSED);
}


static gboolean
handle_notify (PhoshNotifyDbusNotifications *skeleton,
               GDBusMethodInvocation        *invocation,
               const char                   *app_name,
               guint                         replaces_id,
               const gchar                  *app_icon,
               const gchar                  *summary,
               const gchar                  *body,
               const gchar * const          *actions,
               GVariant                     *hints,
               gint                          expire_timeout)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);
  PhoshNotification *notification = NULL;
  GVariant *item;
  GVariantIter iter;
  guint id;
  g_autofree gchar *image_path = NULL;
  g_autofree gchar *desktop_id = NULL;

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call Notify: %s (%u): %s (%s), %s, %d", app_name, replaces_id, summary, body, app_icon, expire_timeout);

  g_variant_iter_init (&iter, hints);
  while ((item = g_variant_iter_next_value (&iter))) {
    g_autofree gchar *key = NULL;
    g_autoptr(GVariant) value = NULL;

    g_variant_get (item, "{sv}", &key, &value);

    if (g_strcmp0 (key, "urgency") == 0) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_BYTE) &&
          (g_variant_get_byte(value) == PHOSH_NOTIFICATION_URGENCY_CRITICAL)) {
        expire_timeout = 0;
      }
    } else if ((g_strcmp0 (key, "image-data") == 0) ||
               (g_strcmp0 (key, "image_data") == 0)) {
      /* TBD */
    }
    else if ((g_strcmp0 (key, "icon-data") == 0) ||
             (g_strcmp0 (key, "icon_data") == 0)) {
      /* TBD */
    }
    else if ((g_strcmp0 (key, "image-path") == 0) ||
             (g_strcmp0 (key, "image_path") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        image_path = g_variant_dup_string (value, NULL);
    }
    else if ((g_strcmp0 (key, "desktop_entry") == 0) ||
             (g_strcmp0 (key, "desktop-entry") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        desktop_id = g_variant_dup_string (value, NULL);
    }
  }

  if (expire_timeout == -1)
    expire_timeout = NOTIFICATION_DEFAULT_TIMEOUT;

  if (replaces_id)
    notification = g_hash_table_lookup (self->notifications, GUINT_TO_POINTER (replaces_id));

  if (notification) {
    id = replaces_id;
    g_object_set (notification,
                  "app_name", app_name,
                  "summary", summary,
                  "body", body,
                  "app_icon", image_path ?: app_icon,
                  NULL);
  } else {
    id = self->next_id++;

    notification = g_object_ref_sink (phosh_notification_new (app_name, summary,
                                                              body, image_path ?: app_icon));
    g_hash_table_insert (self->notifications,
                         GUINT_TO_POINTER (id),
                         notification);
    g_object_set_data (G_OBJECT (notification), "notify-id", GUINT_TO_POINTER(id));

    if (expire_timeout) {
      g_timeout_add_seconds (expire_timeout / 1000,
                             (GSourceFunc)on_notification_expired,
                             GUINT_TO_POINTER (id));
    }

    g_signal_connect_object (notification,
                             "dismissed",
                             G_CALLBACK (on_notification_dismissed),
                             self,
                             G_CONNECT_SWAPPED);

    gtk_widget_show (GTK_WIDGET (notification));
  }

  phosh_notify_dbus_notifications_complete_notify (
    skeleton, invocation, id);

  return TRUE;
}


static void
phosh_notify_manager_notify_iface_init (PhoshNotifyDbusNotificationsIface *iface)
{
  iface->handle_close_notification = handle_close_notification;
  iface->handle_get_capabilities = handle_get_capabilities;
  iface->handle_get_server_information = handle_get_server_information;
  iface->handle_notify = handle_notify;
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
}


static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
  g_debug ("Lost or failed to acquire name %s", name);
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshNotifyManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/freedesktop/Notifications",
                                    NULL);
}


static void
phosh_notify_manager_dispose (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);

  g_clear_pointer (&self->notifications, g_hash_table_destroy);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->dispose (object);
}


static void
phosh_notify_manager_constructed (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       NOTIFY_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);
}


static void
phosh_notify_manager_class_init (PhoshNotifyManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_notify_manager_constructed;
  object_class->dispose = phosh_notify_manager_dispose;
  object_class->set_property = phosh_notify_manager_set_property;
  object_class->get_property = phosh_notify_manager_get_property;
}


static void
phosh_notify_manager_init (PhoshNotifyManager *self)
{
  self->notifications = g_hash_table_new_full (g_direct_hash,
                                               g_direct_equal,
                                               NULL,
                                               (GDestroyNotify)gtk_widget_destroy);
  self->next_id = 1;
}


PhoshNotifyManager *
phosh_notify_manager_get_default (void)
{
  static PhoshNotifyManager *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_NOTIFY_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }

  return instance;
}

/**
 * phosh_notify_manager_close_notification:
 *
 * Close a notification due to the give reason
 * Returns: %TRUE if the notification was closed, %FALSE if it didn't exist.
 */
gboolean
phosh_notify_manager_close_notification (PhoshNotifyManager *self, guint id,
                                         PhoshNotifyManagerReason reason)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);
  g_return_val_if_fail (id > 0, FALSE);

  if (!g_hash_table_remove (self->notifications, GUINT_TO_POINTER (id)))
    return FALSE;

  g_debug ("Emitting NotificationClosed: %d, %d", id, reason);

  phosh_notify_dbus_notifications_emit_notification_closed (
    PHOSH_NOTIFY_DBUS_NOTIFICATIONS (self), id, reason);

  return TRUE;
}
