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


#define PHOSH_TYPE_NOTIFICATION_SOURCE (phosh_notification_source_get_type ())


G_DECLARE_FINAL_TYPE (PhoshNotificationSource, phosh_notification_source, PHOSH, NOTIFICATION_SOURCE, GObject)


PhoshNotificationSource *phosh_notification_source_new      (const char              *name);
void                     phosh_notification_source_add      (PhoshNotificationSource *self,
                                                             PhoshNotification       *notification);
const char              *phosh_notification_source_get_name (PhoshNotificationSource *self);

G_END_DECLS
