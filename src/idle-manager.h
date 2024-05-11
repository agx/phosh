/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <glib-object.h>
#include "dbus/phosh-idle-dbus.h"

#define PHOSH_TYPE_IDLE_MANAGER                 (phosh_idle_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshIdleManager, phosh_idle_manager, PHOSH, IDLE_MANAGER, GObject)

PhoshIdleManager * phosh_idle_manager_get_default    (void);
void phosh_idle_manager_reset_timers (PhoshIdleManager *self);
