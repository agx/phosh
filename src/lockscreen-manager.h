/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <gtk/gtk.h>

#define PHOSH_TYPE_LOCKSCREEN_MANAGER (phosh_lockscreen_manager_get_type())

G_DECLARE_FINAL_TYPE (PhoshLockscreenManager,
                      phosh_lockscreen_manager,
                      PHOSH,
                      LOCKSCREEN_MANAGER,
                      GObject)

PhoshLockscreenManager *phosh_lockscreen_manager_new (void);
void                    phosh_lockscreen_manager_set_locked  (PhoshLockscreenManager *self,
                                                              gboolean state);
gboolean                phosh_lockscreen_manager_get_locked  (PhoshLockscreenManager *self);
void                    phosh_lockscreen_manager_set_timeout (PhoshLockscreenManager *self,
                                                              gint timeout);
gint                    phosh_lockscreen_manager_get_timeout (PhoshLockscreenManager *self);
gint64                  phosh_lockscreen_manager_get_active_time (PhoshLockscreenManager *self);
