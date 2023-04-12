/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "notify-manager.h"
#include "notification-list.h"
#include "dbus/notify-dbus.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define NOTIFICATIONS_SCHEMA_ID "org.gnome.desktop.notifications"
#define NOTIFICATIONS_KEY_SHOW_BANNERS "show-banners"
#define NOTIFICATIONS_KEY_APP_CHILDREN "application-children"

#define NOTIFICATIONS_APP_SCHEMA_ID NOTIFICATIONS_SCHEMA_ID ".application"
#define NOTIFICATIONS_APP_PREFIX "/org/gnome/desktop/notifications/application"
#define NOTIFICATIONS_APP_KEY_SHOW_BANNERS "show-banners"
#define NOTIFICATIONS_APP_KEY_APP_ID "application-id"

#define PHOSH_TYPE_NOTIFY_MANAGER             (phosh_notify_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshNotifyManager, phosh_notify_manager, PHOSH, NOTIFY_MANAGER,
                      PhoshNotifyDBusNotificationsSkeleton)

PhoshNotifyManager    *phosh_notify_manager_get_default      (void);
PhoshNotificationList *phosh_notify_manager_get_list         (PhoshNotifyManager *self);
gboolean               phosh_notify_manager_get_show_banners (PhoshNotifyManager *self);
guint                  phosh_notify_manager_get_notification_id (PhoshNotifyManager *self);
void                   phosh_notify_manager_add_notification (PhoshNotifyManager *self,
                                                              const gchar *source_id,
                                                              int expire_timeout,
                                                              PhoshNotification *notification);
gboolean               phosh_notify_manager_close_notification_by_id (PhoshNotifyManager      *self,
                                                                      int                      id,
                                                                      PhoshNotificationReason  reason);
void                   phosh_notify_manager_close_all_notifications  (PhoshNotifyManager      *self,
                                                                      PhoshNotificationReason  reaseon);
gboolean               phosh_notify_manager_get_show_notification_banner (PhoshNotifyManager *self,
                         PhoshNotification  *notification);

guint                  phosh_notify_manager_add_shell_notification (PhoshNotifyManager *self,
                                                                    const char         *summary,
                                                                    const char         *body,
                                                                    const char         *icon);

G_END_DECLS
