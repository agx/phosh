/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "launcher-item.h"

#include <gtk/gtk.h>
#include <handy.h>

#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_LAUNCHER_ROW (phosh_launcher_row_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLauncherRow, phosh_launcher_row, PHOSH, LAUNCHER_ROW, HdyActionRow)

GtkWidget         *phosh_launcher_row_new      (PhoshLauncherItem *item);
PhoshLauncherItem *phosh_launcher_row_get_item (PhoshLauncherRow *self);

G_END_DECLS
