/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Stubs so we don't need to run the shell */

#include <glib.h>
#include "phosh-wayland.h"
#include "shell.h"

PhoshToplevelManager *toplevel_manager = NULL;
GObject *shell = NULL;

PhoshShell *
phosh_shell_get_default (void)
{
  if (shell == NULL)
    shell = g_object_new (G_TYPE_OBJECT, NULL);
  return (PhoshShell*)(shell);
}

void
phosh_shell_get_usable_area (PhoshShell *self, int *x, int *y, int *width, int *height)
{
  if (x)
    *x = 16;
  if (y)
    *y = 32;
  if (width)
    *width = 64;
  if (height)
    *height = 128;
  return;
}

PhoshMonitor *
phosh_shell_get_primary_monitor (PhoshShell *self)
{
  return NULL;
}

PhoshToplevelManager*
phosh_shell_get_toplevel_manager (PhoshShell *self)
{
  return g_object_new (PHOSH_TYPE_TOPLEVEL_MANAGER, NULL);
}

GdkAppLaunchContext*
phosh_shell_get_app_launch_context (PhoshShell *self)
{
  return gdk_display_get_app_launch_context (gdk_display_get_default ());
}

void
phosh_shell_set_state (PhoshShell *self, PhoshShellStateFlags state, gboolean set)
{
}

PhoshShellStateFlags
phosh_shell_get_state (PhoshShell *self)
{
  return 0;
}

gboolean
phosh_shell_get_locked (PhoshShell *self)
{
  return FALSE;
}

gboolean
phosh_shell_get_docked (PhoshShell *self)
{
  return FALSE;
}

PhoshAppTracker *
phosh_shell_get_app_tracker (PhoshShell *self)
{
  return NULL;
}

PhoshLockscreenManager *
phosh_shell_get_lockscreen_manager (PhoshShell *self)
{
  return NULL;
}

gboolean
phosh_shell_get_blanked (PhoshShell *self)
{
  return FALSE;
}

gboolean
phosh_shell_activate_action (PhoshShell *self, const char *action, GVariant *parameter)
{
  return TRUE;
}
