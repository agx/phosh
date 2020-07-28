/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
  PHOSH_NOTIFICATION_URGENCY_LOW = 0,
  PHOSH_NOTIFICATION_URGENCY_NORMAL,
  PHOSH_NOTIFICATION_URGENCY_CRITICAL,
} PhoshNotificationUrgency;


typedef enum {
  PHOSH_NOTIFICATION_REASON_EXPIRED = 1,
  PHOSH_NOTIFICATION_REASON_DISMISSED = 2,
  PHOSH_NOTIFICATION_REASON_CLOSED = 3,
  PHOSH_NOTIFICATION_REASON_UNDEFINED = 4,
} PhoshNotificationReason;


#define PHOSH_NOTIFICATION_DEFAULT_ACTION "default"


#define PHOSH_TYPE_NOTIFICATION (phosh_notification_get_type ())

G_DECLARE_FINAL_TYPE (PhoshNotification, phosh_notification, PHOSH, NOTIFICATION, GObject)


PhoshNotification        *phosh_notification_new           (guint                     id,
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
                                                            GDateTime                *timestamp);
void                      phosh_notification_set_id        (PhoshNotification        *self,
                                                            guint                     id);
guint                     phosh_notification_get_id        (PhoshNotification        *self);
void                      phosh_notification_set_summary   (PhoshNotification        *self,
                                                            const char               *summary);
const char               *phosh_notification_get_summary   (PhoshNotification        *self);
void                      phosh_notification_set_body      (PhoshNotification        *self,
                                                            const char               *body);
const char               *phosh_notification_get_body      (PhoshNotification        *self);
void                      phosh_notification_set_app_name  (PhoshNotification        *self,
                                                            const char               *app_name);
const char               *phosh_notification_get_app_name  (PhoshNotification        *self);
GDateTime                *phosh_notification_get_timestamp (PhoshNotification        *self);
void                      phosh_notification_set_timestamp (PhoshNotification        *self,
                                                            GDateTime                *timestamp);
void                      phosh_notification_set_app_icon  (PhoshNotification        *self,
                                                            GIcon                    *icon);
GIcon                    *phosh_notification_get_app_icon  (PhoshNotification        *self);
void                      phosh_notification_set_app_info  (PhoshNotification        *self,
                                                            GAppInfo                 *info);
GAppInfo                 *phosh_notification_get_app_info  (PhoshNotification        *self);
void                      phosh_notification_set_image     (PhoshNotification        *self,
                                                            GIcon                    *icon);
GIcon                    *phosh_notification_get_image     (PhoshNotification        *self);
void                      phosh_notification_set_urgency   (PhoshNotification        *self,
                                                            PhoshNotificationUrgency  urgency);
PhoshNotificationUrgency  phosh_notification_get_urgency   (PhoshNotification        *self);
void                      phosh_notification_set_actions   (PhoshNotification        *self,
                                                            GStrv                     actions);
const GStrv               phosh_notification_get_actions   (PhoshNotification        *self);
void                      phosh_notification_set_transient (PhoshNotification        *self,
                                                            gboolean                  transient);
gboolean                  phosh_notification_get_transient (PhoshNotification        *self);
void                      phosh_notification_set_resident  (PhoshNotification        *self,
                                                            gboolean                  resident);
gboolean                  phosh_notification_get_resident  (PhoshNotification        *self);
void                      phosh_notification_set_category  (PhoshNotification        *self,
                                                            const char               *category);
const char               *phosh_notification_get_category  (PhoshNotification        *self);
void                      phosh_notification_activate      (PhoshNotification        *self,
                                                            const char               *action);
void                      phosh_notification_expires       (PhoshNotification        *self,
                                                            int                       timeout);
void                      phosh_notification_close         (PhoshNotification        *self,
                                                            PhoshNotificationReason   reason);

G_END_DECLS
