/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "launcher-entry-manager.h"
#include "session-manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_SHELL phosh_shell_get_type ()

G_DECLARE_FINAL_TYPE (PhoshShell, phosh_shell, PHOSH, SHELL, GObject)

PhoshShell             *phosh_shell_get_default (void);

/* Created by the shell on startup */
PhoshLauncherEntryManager *phosh_shell_get_launcher_entry_manager (PhoshShell *self);
PhoshSessionManager       *phosh_shell_get_session_manager (PhoshShell *self);


G_END_DECLS
