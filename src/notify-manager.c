/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notify-manager"

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

  return FALSE;
}


static gboolean
handle_get_capabilities (PhoshNotifyDbusNotifications *skeleton,
                         GDBusMethodInvocation        *invocation)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call GetCapabilities");

  return FALSE;
}


static gboolean
handle_get_server_information (PhoshNotifyDbusNotifications *skeleton,
                               GDBusMethodInvocation        *invocation)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call GetServerInformation");

  return FALSE;
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

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call Notify: %s: %s", app_name, summary);

  return FALSE;
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
