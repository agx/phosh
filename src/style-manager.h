/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_STYLE_MANAGER (phosh_style_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshStyleManager, phosh_style_manager, PHOSH, STYLE_MANAGER, GObject)

PhoshStyleManager *phosh_style_manager_new (void);

const char        *phosh_style_manager_get_stylesheet (const char *theme_name);

G_END_DECLS
