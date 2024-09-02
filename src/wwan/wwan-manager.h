/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN_MANAGER (phosh_wwan_manager_get_type())

G_DECLARE_DERIVABLE_TYPE (PhoshWWanManager, phosh_wwan_manager, PHOSH, WWAN_MANAGER, GObject)

struct _PhoshWWanManagerClass {
  GObjectClass parent_class;
};

PhoshWWanManager  *phosh_wwan_manager_new (void);
void               phosh_wwan_manager_set_enabled (PhoshWWanManager *self, gboolean enabled);
gboolean           phosh_wwan_manager_get_data_enabled (PhoshWWanManager *self);
void               phosh_wwan_manager_set_data_enabled (PhoshWWanManager *self, gboolean enabled);
gboolean           phosh_wwan_manager_has_data (PhoshWWanManager *self);

G_END_DECLS
