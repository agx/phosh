/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <notifications/notification.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MOUNT_NOTIFICATION     phosh_mount_notification_get_type ()

G_DECLARE_FINAL_TYPE (PhoshMountNotification, phosh_mount_notification,
                      PHOSH, MOUNT_NOTIFICATION, PhoshNotification)

PhoshMountNotification *phosh_mount_notification_new_from_mount (guint id, GMount *mount);

G_END_DECLS
