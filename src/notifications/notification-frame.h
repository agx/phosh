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


#define PHOSH_TYPE_NOTIFICATION_FRAME (phosh_notification_frame_get_type ())


G_DECLARE_FINAL_TYPE (PhoshNotificationFrame, phosh_notification_frame, PHOSH, NOTIFICATION_FRAME, GtkEventBox)


GtkWidget         *phosh_notification_frame_new               (gboolean show_body,
                                                               const char * const *action_filters);
void               phosh_notification_frame_bind_notification (PhoshNotificationFrame *self,
                                                               PhoshNotification      *notification);
void               phosh_notification_frame_bind_model        (PhoshNotificationFrame *self,
                                                               GListModel             *model);
const char* const *phosh_notification_frame_get_action_filter_keys (PhoshNotificationFrame *self);

G_END_DECLS
