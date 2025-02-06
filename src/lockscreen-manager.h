/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "lockscreen.h"
#include <glib-object.h>

#define PHOSH_TYPE_LOCKSCREEN_MANAGER (phosh_lockscreen_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLockscreenManager,
                      phosh_lockscreen_manager,
                      PHOSH,
                      LOCKSCREEN_MANAGER,
                      GObject)

G_BEGIN_DECLS

void                    phosh_lockscreen_manager_set_locked  (PhoshLockscreenManager *self,
                                                              gboolean state);
gboolean                phosh_lockscreen_manager_get_locked  (PhoshLockscreenManager *self);
gboolean                phosh_lockscreen_manager_set_page  (PhoshLockscreenManager *self,
                                                            PhoshLockscreenPage     page);
PhoshLockscreenPage     phosh_lockscreen_manager_get_page  (PhoshLockscreenManager *self);
gint64                  phosh_lockscreen_manager_get_active_time (PhoshLockscreenManager *self);
PhoshLockscreen        *phosh_lockscreen_manager_get_lockscreen (PhoshLockscreenManager *self);

G_END_DECLS
