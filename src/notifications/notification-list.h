/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <glib-object.h>

#include "notification.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_NOTIFICATION_LIST (phosh_notification_list_get_type ())


G_DECLARE_FINAL_TYPE (PhoshNotificationList, phosh_notification_list, PHOSH, NOTIFICATION_LIST, GObject)


PhoshNotificationList *phosh_notification_list_new       (void);
void                   phosh_notification_list_add       (PhoshNotificationList *self,
                                                          const char            *source_id,
                                                          PhoshNotification     *notification);
PhoshNotification     *phosh_notification_list_get_by_id (PhoshNotificationList *self,
                                                          guint                  id);

G_END_DECLS
