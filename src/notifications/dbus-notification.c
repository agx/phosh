/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-dbus-notification"

#include "phosh-config.h"
#include "dbus-notification.h"
#include "notify-manager.h"
#include "shell.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

/**
 * PhoshDBusNotification:
 *
 * A notifiction submitted via the DBus notification interface
 *
 * The #PhoshDBusNotification is a notification submitted via the
 * org.freedesktop.Notification interface.
 */

typedef struct _PhoshDBusNotification {
  PhoshNotification parent;

} PhoshDBusNotification;


G_DEFINE_TYPE (PhoshDBusNotification, phosh_dbus_notification, PHOSH_TYPE_NOTIFICATION)


static void
phosh_dbus_notification_do_action (PhoshNotification *notification, guint id, const char *action)
{
  PhoshNotifyManager *nm = phosh_notify_manager_get_default ();

  phosh_notify_dbus_notifications_emit_action_invoked (
    PHOSH_NOTIFY_DBUS_NOTIFICATIONS (nm), id, action);
}


static void
phosh_dbus_notification_class_init (PhoshDBusNotificationClass *klass)
{
  PhoshNotificationClass *notification_class = PHOSH_NOTIFICATION_CLASS (klass);

  notification_class->do_action = phosh_dbus_notification_do_action;
}


static void
phosh_dbus_notification_init (PhoshDBusNotification *self)
{
}


PhoshDBusNotification *
phosh_dbus_notification_new (guint                     id,
                             const char               *app_name,
                             GAppInfo                 *info,
                             const char               *summary,
                             const char               *body,
                             GIcon                    *icon,
                             GIcon                    *image,
                             PhoshNotificationUrgency  urgency,
                             GStrv                     actions,
                             gboolean                  transient,
                             gboolean                  resident,
                             const char               *category,
                             const char               *profile,
                             GDateTime                *timestamp)
{
  return g_object_new (PHOSH_TYPE_DBUS_NOTIFICATION,
                       "id", id,
                       "summary", summary,
                       "body", body,
                       "app-name", app_name,
                       "app-icon", icon,
                       /* Set info after fallback name and icon */
                       "app-info", info,
                       "image", image,
                       "urgency", urgency,
                       "actions", actions,
                       "transient", transient,
                       "resident", resident,
                       "category", category,
                       "profile", profile,
                       "timestamp", timestamp,
                       NULL);
}
