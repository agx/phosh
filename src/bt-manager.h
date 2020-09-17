/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BT_MANAGER (phosh_bt_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshBtManager, phosh_bt_manager, PHOSH, BT_MANAGER, GObject)

PhoshBtManager    *phosh_bt_manager_new (void);
const gchar *phosh_bt_manager_get_icon_name (PhoshBtManager *self);
gboolean     phosh_bt_manager_get_enabled (PhoshBtManager *self);
gboolean     phosh_bt_manager_get_present (PhoshBtManager *self);

G_END_DECLS
