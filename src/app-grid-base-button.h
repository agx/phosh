/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_APP_GRID_BASE_BUTTON (phosh_app_grid_base_button_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshAppGridBaseButton, phosh_app_grid_base_button, PHOSH, APP_GRID_BASE_BUTTON, GtkFlowBoxChild)

struct _PhoshAppGridBaseButtonClass
{
  GtkFlowBoxChildClass parent_class;
};

void        phosh_app_grid_base_button_set_label (PhoshAppGridBaseButton *self, const char *label);
const char *phosh_app_grid_base_button_get_label (PhoshAppGridBaseButton *self);

G_END_DECLS
