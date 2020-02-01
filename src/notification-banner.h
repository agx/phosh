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

#define PHOSH_TYPE_NOTIFICATION_BANNER (phosh_notification_banner_get_type())

G_DECLARE_FINAL_TYPE (PhoshNotificationBanner, phosh_notification_banner, PHOSH, NOTIFICATION_BANNER, PhoshLayerSurface)

PhoshNotificationBanner *phosh_notification_banner_new             (const char              *app_name,
                                                                    GAppInfo                *info,
                                                                    const char              *summary,
                                                                    const char              *body,
                                                                    GIcon                   *icon,
                                                                    GIcon                   *image,
                                                                    GStrv                    actions);
void               phosh_notification_banner_set_summary           (PhoshNotificationBanner *self,
                                                                    const gchar             *summary);
void               phosh_notification_banner_set_body              (PhoshNotificationBanner *self,
                                                                    const gchar             *body);
void               phosh_notification_banner_set_app_name          (PhoshNotificationBanner *self,
                                                                    const gchar             *app_name);
void               phosh_notification_banner_set_app_icon          (PhoshNotificationBanner *self,
                                                                    GIcon                   *icon);
void               phosh_notification_banner_set_app_info          (PhoshNotificationBanner *self,
                                                                    GAppInfo                *info);
void               phosh_notification_banner_set_image             (PhoshNotificationBanner *self,
                                                                    GIcon                   *icon);
void               phosh_notification_banner_set_actions           (PhoshNotificationBanner *self,
                                                                    GStrv                    actions);

G_END_DECLS
