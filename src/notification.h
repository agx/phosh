/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <layersurface.h>

G_BEGIN_DECLS

typedef enum {
  PHOSH_NOTIFICATION_URGENCY_LOW = 0,
  PHOSH_NOTIFICATION_URGENCY_NORMAL,
  PHOSH_NOTIFICATION_URGENCY_CRITICAL,
} PhoshNotificationUrgency;

#define PHOSH_TYPE_NOTIFICATION (phosh_notification_get_type())

G_DECLARE_FINAL_TYPE (PhoshNotification, phosh_notification, PHOSH, NOTIFICATION, PhoshLayerSurface)

PhoshNotification *phosh_notification_new             (const char        *app_name,
                                                       GAppInfo          *info,
                                                       const char        *summary,
                                                       const char        *body,
                                                       GIcon             *icon,
                                                       GIcon             *image);
void               phosh_notification_set_summary     (PhoshNotification *self,
                                                       const gchar       *summary);
void               phosh_notification_set_body        (PhoshNotification *self,
                                                       const gchar       *body);
void               phosh_notification_set_app_name    (PhoshNotification *self,
                                                       const gchar       *app_name);
void               phosh_notification_set_app_icon    (PhoshNotification *self,
                                                       GIcon             *icon);
void               phosh_notification_set_app_info    (PhoshNotification *self,
                                                       GAppInfo          *info);
void               phosh_notification_set_image       (PhoshNotification *self,
                                                       GIcon             *icon);

G_END_DECLS
