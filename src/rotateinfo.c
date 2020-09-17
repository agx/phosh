/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Julian Sparber <julian.sparber@puri.sm>
 */

#define G_LOG_DOMAIN "phosh-rotateinfo"

#include "config.h"

#include "rotateinfo.h"
#include "shell.h"

/**
 * SECTION:rotateinfo
 * @short_description: A widget to display the rotate status
 * @Title: PhoshRotateInfo
 *
 * Rotate Info widget
 */

typedef struct _PhoshRotateInfo {
  PhoshStatusIcon parent;
} PhoshRotateInfo;


G_DEFINE_TYPE (PhoshRotateInfo, phosh_rotate_info, PHOSH_TYPE_STATUS_ICON)


static void
set_state (PhoshRotateInfo *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  /* TODO: switch to builtin monitor once we support wlr-output-management */
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (shell);
  gboolean monitor_is_landscape;
  gboolean portrait = !phosh_shell_get_rotation (shell);

  /* If we have a landscape monitor (tv, laptop) flip the rotation */
  monitor_is_landscape = ((double)monitor->width / (double)monitor->height) > 1.0;
  portrait = monitor_is_landscape ? !portrait : portrait;

  g_debug ("Potrait: %d, width: %d, height: %d", portrait, monitor->width , monitor->height);
  if (portrait) {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "screen-rotation-portrait-symbolic");
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Portrait"));
  } else {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "screen-rotation-landscape-symbolic");
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Landscape"));
  }
}


static void
phosh_rotate_info_finalize (GObject *object)
{
  PhoshRotateInfo *self = PHOSH_ROTATE_INFO(object);

  g_signal_handlers_disconnect_by_data (phosh_shell_get_default (), self);

  G_OBJECT_CLASS (phosh_rotate_info_parent_class)->finalize (object);
}


static void
phosh_rotate_info_class_init (PhoshRotateInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = phosh_rotate_info_finalize;
}


static void
phosh_rotate_info_init (PhoshRotateInfo *self)
{
  g_signal_connect_swapped (phosh_shell_get_default (),
                            "notify::rotation",
                            G_CALLBACK (set_state),
                            self);
  set_state (self);
}


GtkWidget *
phosh_rotate_info_new (void)
{
  return g_object_new (PHOSH_TYPE_ROTATE_INFO, NULL);
}
