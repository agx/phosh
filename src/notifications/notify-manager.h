/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "notify-manager.h"
#include "dbus/notify-dbus.h"
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  PHOSH_NOTIFY_MANAGER_REASON_EXPIRED = 1,
  PHOSH_NOTIFY_MANAGER_REASON_DISMISSED = 2,
  PHOSH_NOTIFY_MANAGER_REASON_CLOSED = 3,
  PHOSH_NOTIFY_MANAGER_REASON_UNDEFINED = 4,
} PhoshNotifyManagerReason;

#define PHOSH_TYPE_NOTIFY_MANAGER             (phosh_notify_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshNotifyManager, phosh_notify_manager, PHOSH, NOTIFY_MANAGER,
                      PhoshNotifyDbusNotificationsSkeleton)

PhoshNotifyManager * phosh_notify_manager_get_default        (void);
gboolean             phosh_notify_manager_close_notification (PhoshNotifyManager *self,
                                                              guint id,
                                                              PhoshNotifyManagerReason  reason);
gboolean             phosh_notify_manager_action_invoked     (PhoshNotifyManager       *self,
                                                              guint                     id,
                                                              const char               *action);

G_END_DECLS
