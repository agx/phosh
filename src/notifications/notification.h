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


#define PHOSH_NOTIFICATION_DEFAULT_ACTION "default"


#define PHOSH_TYPE_NOTIFICATION (phosh_notification_get_type ())

G_DECLARE_FINAL_TYPE (PhoshNotification, phosh_notification, PHOSH, NOTIFICATION, GObject)


PhoshNotification *phosh_notification_new          (const char        *app_name,
                                                    GAppInfo          *info,
                                                    const char        *summary,
                                                    const char        *body,
                                                    GIcon             *icon,
                                                    GIcon             *image,
                                                    GStrv              actions);
void               phosh_notification_set_id       (PhoshNotification *self,
                                                    guint              id);
guint              phosh_notification_get_id       (PhoshNotification *self);
void               phosh_notification_set_summary  (PhoshNotification *self,
                                                    const char        *summary);
const char        *phosh_notification_get_summary  (PhoshNotification *self);
void               phosh_notification_set_body     (PhoshNotification *self,
                                                    const char        *body);
const char        *phosh_notification_get_body     (PhoshNotification *self);
void               phosh_notification_set_app_name (PhoshNotification *self,
                                                    const char        *app_name);
const char        *phosh_notification_get_app_name (PhoshNotification *self);
void               phosh_notification_set_app_icon (PhoshNotification *self,
                                                    GIcon             *icon);
GIcon             *phosh_notification_get_app_icon (PhoshNotification *self);
void               phosh_notification_set_app_info (PhoshNotification *self,
                                                    GAppInfo          *info);
GAppInfo          *phosh_notification_get_app_info (PhoshNotification *self);
void               phosh_notification_set_image    (PhoshNotification *self,
                                                    GIcon             *icon);
GIcon             *phosh_notification_get_image    (PhoshNotification *self);
void               phosh_notification_set_actions  (PhoshNotification *self,
                                                    GStrv              actions);
const GStrv        phosh_notification_get_actions  (PhoshNotification *self);
void               phosh_notification_activate     (PhoshNotification *self,
                                                    const char        *action);

G_END_DECLS
