/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_QUICK_SETTING (phosh_quick_setting_get_type())

G_DECLARE_DERIVABLE_TYPE (PhoshQuickSetting, phosh_quick_setting, PHOSH, QUICK_SETTING, GtkButton)

struct _PhoshQuickSettingClass
{
  GtkButtonClass parent_class;
};

GtkWidget       *phosh_quick_setting_new (void);
void             phosh_quick_setting_set_status_icon (PhoshQuickSetting *self,
                                                      PhoshStatusIcon   *widget);
PhoshStatusIcon *phosh_quick_setting_get_status_icon (PhoshQuickSetting *self);
void             phosh_quick_setting_set_active (PhoshQuickSetting *self, gboolean active);
gboolean         phosh_quick_setting_get_active (PhoshQuickSetting *self);
void             phosh_quick_setting_set_present (PhoshQuickSetting *self, gboolean present);
gboolean         phosh_quick_setting_get_present (PhoshQuickSetting *self);
void             phosh_quick_setting_set_has_status (PhoshQuickSetting *self, gboolean has_status);
gboolean         phosh_quick_setting_get_has_status (PhoshQuickSetting *self);
void             phosh_quick_setting_open_settings_panel (const char *panel);

G_END_DECLS
