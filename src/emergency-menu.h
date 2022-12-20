/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#pragma once

#include "system-modal-dialog.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_EMERGENCY_MENU (phosh_emergency_menu_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEmergencyMenu, phosh_emergency_menu, PHOSH, EMERGENCY_MENU, PhoshSystemModalDialog)

PhoshEmergencyMenu *phosh_emergency_menu_new (void);

G_END_DECLS
