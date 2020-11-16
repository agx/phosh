/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Alexander Mikhaylenko <alexm@gnome.org>
 */
#pragma once

#include <gtk/gtk.h>

#define PHOSH_TYPE_SWIPE_AWAY_BIN (phosh_swipe_away_bin_get_type())

G_DECLARE_FINAL_TYPE (PhoshSwipeAwayBin, phosh_swipe_away_bin, PHOSH, SWIPE_AWAY_BIN, GtkEventBox)

void phosh_swipe_away_bin_remove (PhoshSwipeAwayBin *self);
void phosh_swipe_away_bin_undo   (PhoshSwipeAwayBin *self);
