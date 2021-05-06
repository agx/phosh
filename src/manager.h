/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MANAGER (phosh_manager_get_type())

G_DECLARE_DERIVABLE_TYPE (PhoshManager, phosh_manager, PHOSH, MANAGER, GObject)

/**
 * PhoshManagerClass:
 * @parent_class: The parent class
 * @idle_init: a callback to be invoked once on idle
 */
struct _PhoshManagerClass
{
  GObjectClass parent_class;

  void (*idle_init) (PhoshManager *self);
};

G_END_DECLS
