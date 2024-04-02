/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_LAUNCHER_ITEM (phosh_launcher_item_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLauncherItem, phosh_launcher_item, PHOSH, LAUNCHER_ITEM, GObject)

PhoshLauncherItem *phosh_launcher_item_new (GDesktopAppInfo *app_info);
gboolean           phosh_launcher_item_get_progress_visible (PhoshLauncherItem *self);
void               phosh_launcher_item_set_progress_visible (PhoshLauncherItem *self, gboolean visible);
double             phosh_launcher_item_get_progress (PhoshLauncherItem *self);
void               phosh_launcher_item_set_progress (PhoshLauncherItem *self, double progress);
gboolean           phosh_launcher_item_get_count_visible (PhoshLauncherItem *self);
void               phosh_launcher_item_set_count_visible (PhoshLauncherItem *self, gboolean visible);
gint64             phosh_launcher_item_get_count    (PhoshLauncherItem *self);
void               phosh_launcher_item_set_count    (PhoshLauncherItem *self, gint64 count);
GDesktopAppInfo   *phosh_launcher_item_get_app_info (PhoshLauncherItem *self);

G_END_DECLS
