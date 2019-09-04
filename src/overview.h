/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <gtk/gtk.h>

#define PHOSH_TYPE_OVERVIEW (phosh_overview_get_type())

G_DECLARE_FINAL_TYPE (PhoshOverview, phosh_overview, PHOSH, OVERVIEW, GtkBox)

void phosh_overview_reset (PhoshOverview *self);
GtkWidget * phosh_overview_new (void);
