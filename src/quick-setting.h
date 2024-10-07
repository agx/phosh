/*
 * Copyright (C) 2020 Purism SPC
 *               2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "status-page.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_QUICK_SETTING phosh_quick_setting_get_type ()
G_DECLARE_DERIVABLE_TYPE (PhoshQuickSetting, phosh_quick_setting, PHOSH, QUICK_SETTING, GtkBox)

struct _PhoshQuickSettingClass {
  GtkBoxClass parent_class;
};

GtkWidget       *phosh_quick_setting_new (PhoshStatusPage *status_page);
void             phosh_quick_setting_set_active (PhoshQuickSetting *self, gboolean active);
gboolean         phosh_quick_setting_get_active (PhoshQuickSetting *self);
void             phosh_quick_setting_set_can_show_status (PhoshQuickSetting *self, gboolean can_show_status);
gboolean         phosh_quick_setting_get_can_show_status (PhoshQuickSetting *self);
void             phosh_quick_setting_set_showing_status (PhoshQuickSetting *self, gboolean showing_status);
gboolean         phosh_quick_setting_get_showing_status (PhoshQuickSetting *self);
void             phosh_quick_setting_set_status_page (PhoshQuickSetting *self, PhoshStatusPage *status_page);
PhoshStatusPage *phosh_quick_setting_get_status_page (PhoshQuickSetting *self);

G_END_DECLS
