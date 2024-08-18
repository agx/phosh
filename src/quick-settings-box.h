/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_QUICK_SETTINGS_BOX phosh_quick_settings_box_get_type ()
G_DECLARE_FINAL_TYPE (PhoshQuickSettingsBox, phosh_quick_settings_box, PHOSH, QUICK_SETTINGS_BOX, GtkContainer)

GtkWidget *phosh_quick_settings_box_new (guint max_columns, guint spacing);
void       phosh_quick_settings_box_set_max_columns (PhoshQuickSettingsBox *self, guint max_columns);
guint      phosh_quick_settings_box_get_max_columns (PhoshQuickSettingsBox *self);
void       phosh_quick_settings_box_set_spacing (PhoshQuickSettingsBox *self, guint spacing);
guint      phosh_quick_settings_box_get_spacing (PhoshQuickSettingsBox *self);
void       phosh_quick_settings_box_set_can_show_status (PhoshQuickSettingsBox *self, gboolean can_show_status);
gboolean   phosh_quick_settings_box_get_can_show_status (PhoshQuickSettingsBox *self);

G_END_DECLS
