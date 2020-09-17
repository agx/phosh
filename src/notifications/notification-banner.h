/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <layersurface.h>
#include "notification.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_NOTIFICATION_BANNER (phosh_notification_banner_get_type ())


G_DECLARE_FINAL_TYPE (PhoshNotificationBanner, phosh_notification_banner, PHOSH, NOTIFICATION_BANNER, PhoshLayerSurface)


GtkWidget         *phosh_notification_banner_new              (PhoshNotification       *notification);
PhoshNotification *phosh_notification_banner_get_notification (PhoshNotificationBanner *self);


G_END_DECLS
