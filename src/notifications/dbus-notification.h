/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <notifications/notification.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_DBUS_NOTIFICATION     phosh_dbus_notification_get_type ()

G_DECLARE_FINAL_TYPE (PhoshDBusNotification, phosh_dbus_notification,
                      PHOSH, DBUS_NOTIFICATION, PhoshNotification)

PhoshDBusNotification    *phosh_dbus_notification_new      (guint                     id,
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
                                                            GDateTime                *timestamp);

G_END_DECLS
