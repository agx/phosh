/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "call.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CALL_NOTIFICATION (phosh_call_notification_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCallNotification, phosh_call_notification, PHOSH, CALL_NOTIFICATION,
                      GtkListBoxRow)

PhoshCallNotification *phosh_call_notification_new               (PhoshCall *call);

G_END_DECLS
