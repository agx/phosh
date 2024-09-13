/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_QUICK_SETTINGS phosh_quick_settings_get_type ()
G_DECLARE_FINAL_TYPE (PhoshQuickSettings, phosh_quick_settings, PHOSH, QUICK_SETTINGS, GtkBin)

GtkWidget *phosh_quick_settings_new (void);

G_END_DECLS
