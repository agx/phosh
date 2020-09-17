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


#define PHOSH_TYPE_NOTIFY_MANAGER             (phosh_notify_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshNotifyManager, phosh_notify_manager, PHOSH, NOTIFY_MANAGER,
                      PhoshNotifyDbusNotificationsSkeleton)

PhoshNotifyManager    *phosh_notify_manager_get_default      (void);
PhoshNotificationList *phosh_notify_manager_get_list         (PhoshNotifyManager *self);
gboolean               phosh_notify_manager_get_show_banners (PhoshNotifyManager *self);


G_END_DECLS
