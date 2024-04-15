/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *         Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 *
 * ShellActionMode is from GNOME Shell's shell-action-modes.h
 * which is GPL-2.0-or-later and authored by
 * Florian Müllner <fmuellner@gnome.org>
 */
#pragma once

#include "dbus/phosh-gnome-shell-dbus.h"
#include <glib-object.h>

/**
 * ShellActionMode:
 * @PHOSH_SHELL_ACTION_MODE_NONE: block action
 * @PHOSH_SHELL_ACTION_MODE_NORMAL: allow action when in window mode,
 *     e.g. when the focus is in an application window
 * @PHOSH_SHELL_ACTION_MODE_OVERVIEW: allow action while the overview
 *     is active
 * @PHOSH_SHELL_ACTION_MODE_LOCK_SCREEN: allow action when the screen
 *     is locked, e.g. when the screen shield is shown
 * @PHOSH_SHELL_ACTION_MODE_UNLOCK_SCREEN: allow action in the unlock
 *     dialog
 * @PHOSH_SHELL_ACTION_MODE_LOGIN_SCREEN: allow action in the login screen
 * @PHOSH_SHELL_ACTION_MODE_SYSTEM_MODAL: allow action when a system modal
 *     dialog (e.g. authentication or session dialogs) is open
 * @PHOSH_SHELL_ACTION_MODE_LOOKING_GLASS: allow action in looking glass
 * @PHOSH_SHELL_ACTION_MODE_POPUP: allow action while a shell menu is open
 * @PHOSH_SHELL_ACTION_MODE_ALL: always allow action
 *
 * Controls in which GNOME Shell states an action (like keybindings and gestures)
 * should be handled.
*/
typedef enum {
  PHOSH_SHELL_ACTION_MODE_NONE          = 0,
  PHOSH_SHELL_ACTION_MODE_NORMAL        = 1 << 0,
  PHOSH_SHELL_ACTION_MODE_OVERVIEW      = 1 << 1,
  PHOSH_SHELL_ACTION_MODE_LOCK_SCREEN   = 1 << 2,
  PHOSH_SHELL_ACTION_MODE_UNLOCK_SCREEN = 1 << 3,
  PHOSH_SHELL_ACTION_MODE_LOGIN_SCREEN  = 1 << 4,
  PHOSH_SHELL_ACTION_MODE_SYSTEM_MODAL  = 1 << 5,
  PHOSH_SHELL_ACTION_MODE_LOOKING_GLASS = 1 << 6,
  PHOSH_SHELL_ACTION_MODE_POPUP         = 1 << 7,

  PHOSH_SHELL_ACTION_MODE_ALL = ~0,
} PhoshShellActionMode;

/**
 * ShellKeyBindingFlags:
 * @PHOSH_SHELL_KEY_BINDING_NONE: none
 * @HPOSH_SHELL_KEY_BINDING_PER_WINDOW: per-window
 * @PHOSH_SHELL_KEY_BINDING_BUILTIN: built-in
 * @PHOSH_SHELL_KEY_BINDING_IS_REVERSED: is reversed
 * @PHOSH_SHELL_KEY_BINDING_NON_MASKABLE: always active
 * @PHOSH_SHELL_KEY_BINDING_IGNORE_AUTOREPEAT: ignore autorepeat
 * @PHOSH_SHELL_KEY_BINDING_NO_AUTO_GRAB: not grabbed automatically
 */
typedef enum
{
  /* not implemented */
  PHOSH_SHELL_KEY_BINDING_NONE,
  PHOSH_SHELL_KEY_BINDING_PER_WINDOW   = 1 << 0,
  PHOSH_SHELL_KEY_BINDING_BUILTIN      = 1 << 1,
  PHOSH_SHELL_KEY_BINDING_IS_REVERSED  = 1 << 2,
  PHOSH_SHELL_KEY_BINDING_NON_MASKABLE = 1 << 3,
  PHOSH_SHELL_KEY_BINDING_NO_AUTO_GRAB = 1 << 5,
  /* implemented */
  PHOSH_SHELL_KEY_BINDING_IGNORE_AUTOREPEAT = 1 << 4,
} PhoshShellKeyBindingFlags;

G_BEGIN_DECLS

#define PHOSH_TYPE_GNOME_SHELL_MANAGER             (phosh_gnome_shell_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshGnomeShellManager, phosh_gnome_shell_manager, PHOSH, GNOME_SHELL_MANAGER,
                      PhoshDBusGnomeShellSkeleton)

PhoshGnomeShellManager *phosh_gnome_shell_manager_get_default      (void);

G_END_DECLS
