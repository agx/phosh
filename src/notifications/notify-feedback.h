/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "notification-list.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_NOTIFY_FEEDBACK (phosh_notify_feedback_get_type ())

G_DECLARE_FINAL_TYPE (PhoshNotifyFeedback, phosh_notify_feedback, PHOSH, NOTIFY_FEEDBACK, GObject)

PhoshNotifyFeedback *phosh_notify_feedback_new (PhoshNotificationList *list);

G_END_DECLS
