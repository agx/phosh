/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal.h"

#define PHOSH_TYPE_OSD_WINDOW (phosh_osd_window_get_type ())

G_DECLARE_FINAL_TYPE (PhoshOsdWindow, phosh_osd_window, PHOSH, OSD_WINDOW, PhoshSystemModal)

GtkWidget *phosh_osd_window_new (char *connector,
                                 char *label,
                                 char *icon_name,
                                 double level,
                                 double max_level);
