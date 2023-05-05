/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "notification-list.h"

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PhoshNotifyScreenWakeupFlags:
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE: No wakeup
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY: Wakeup screen based on notification urgency
 * @PHOSH_NOTIFY_SCREEN_WAKUP_FLAG_CATEGORY: Wakeup screen based on notification category
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY: Wakeup screen on any notification
 *
 * What notification properties trigger screen wakeup
 */
typedef enum {
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE     = 0, /*< skip >*/
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY      = (1 << 0),
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY  = (1 << 1),
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_CATEGORY = (1 << 2),
} PhoshNotifyScreenWakeupFlags;

#define PHOSH_TYPE_NOTIFY_FEEDBACK (phosh_notify_feedback_get_type ())

G_DECLARE_FINAL_TYPE (PhoshNotifyFeedback, phosh_notify_feedback, PHOSH, NOTIFY_FEEDBACK, GObject)

PhoshNotifyFeedback *phosh_notify_feedback_new (PhoshNotificationList *list);
gboolean             phosh_notify_feedback_check_screen_wakeup (PhoshNotifyFeedback *self,
                                                                PhoshNotification   *notification);

G_END_DECLS
