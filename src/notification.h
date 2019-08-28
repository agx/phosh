/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <layersurface.h>

G_BEGIN_DECLS

enum
{
  PHOSH_NOTIFICATION_URGENCY_LOW = 0,
  PHOSH_NOTIFICATION_URGENCY_NORMAL,
  PHOSH_NOTIFICATION_URGENCY_CRITICAL,
} PhoshNotificationUrgency;

#define PHOSH_TYPE_NOTIFICATION (phosh_notification_get_type())

G_DECLARE_FINAL_TYPE (PhoshNotification, phosh_notification, PHOSH, NOTIFICATION, PhoshLayerSurface)

PhoshNotification *phosh_notification_new (const char *app_name,
                                           const char *summary,
                                           const char *body,
                                           const char *app_icon);
void phosh_notification_set_summary (PhoshNotification *self, const gchar *summary);
void phosh_notification_set_app_name (PhoshNotification *self, const gchar *app_name);
void phosh_notification_set_app_icon (PhoshNotification *self, const gchar *app_icon);

G_END_DECLS
