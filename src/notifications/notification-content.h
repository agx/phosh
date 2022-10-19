/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

#include "notification.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_NOTIFICATION_CONTENT (phosh_notification_content_get_type ())


G_DECLARE_FINAL_TYPE (PhoshNotificationContent, phosh_notification_content, PHOSH, NOTIFICATION_CONTENT, GtkListBoxRow)


GtkWidget         *phosh_notification_content_new              (PhoshNotification        *notification,
                                                                gboolean                  show_body,
                                                                const char * const       *action_filters);
PhoshNotification *phosh_notification_content_get_notification (PhoshNotificationContent *self);


G_END_DECLS
