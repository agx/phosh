/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

#define PHOSH_TYPE_POWER_MENU_MANAGER (phosh_power_menu_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshPowerMenuManager, phosh_power_menu_manager, PHOSH, POWER_MENU_MANAGER, GObject);

PhoshPowerMenuManager *phosh_power_menu_manager_new (void);
