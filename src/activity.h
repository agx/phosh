/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gtk/gtk.h>
#include "thumbnail.h"

#define PHOSH_TYPE_ACTIVITY (phosh_activity_get_type())

G_DECLARE_FINAL_TYPE (PhoshActivity, phosh_activity, PHOSH, ACTIVITY, GtkEventBox)

GtkWidget  *phosh_activity_new        (const char *app_id,
                                       const char *title);
const char *phosh_activity_get_app_id (PhoshActivity   *self);
const char *phosh_activity_get_title  (PhoshActivity   *self);
void        phosh_activity_set_title  (PhoshActivity   *self,
                                       const char      *title);
void        phosh_activity_set_thumbnail (PhoshActivity *self,
                                          PhoshThumbnail *thumbnail);
