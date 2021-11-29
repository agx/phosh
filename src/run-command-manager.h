/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#pragma once

#include <glib-object.h>

#define PHOSH_TYPE_RUN_COMMAND_MANAGER (phosh_run_command_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshRunCommandManager, phosh_run_command_manager, PHOSH, RUN_COMMAND_MANAGER, GObject);

PhoshRunCommandManager *phosh_run_command_manager_new (void);
