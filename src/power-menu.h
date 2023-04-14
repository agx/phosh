/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "system-modal-dialog.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_POWER_MENU (phosh_power_menu_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPowerMenu, phosh_power_menu, PHOSH, POWER_MENU, PhoshSystemModalDialog)

PhoshPowerMenu *phosh_power_menu_new (PhoshMonitor *monitor);

G_END_DECLS
