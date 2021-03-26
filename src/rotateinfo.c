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
 * @short_description: A widget to display the rotate lock status
 * @Title: PhoshRotateInfo
 *
 * A #PhoshStatusIcon to display the rotation lock status.
 * It can either display whether a rotation lock is currently active or
 * if the output is in portrait/landscape mode.
 */

typedef struct _PhoshRotateInfo {
  PhoshStatusIcon     parent;

  PhoshRotationManager *manager;
} PhoshRotateInfo;


G_DEFINE_TYPE (PhoshRotateInfo, phosh_rotate_info, PHOSH_TYPE_STATUS_ICON)

static void
on_transform_changed (PhoshRotateInfo *self)
{
  PhoshMonitor *monitor;
  gboolean monitor_is_landscape;
  gboolean portrait;

  if (phosh_rotation_manager_get_mode (self->manager) != PHOSH_ROTATION_MANAGER_MODE_OFF) {
    return;
  }

  switch (phosh_rotation_manager_get_transform (self->manager)) {
  case PHOSH_MONITOR_TRANSFORM_NORMAL:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED:
  case PHOSH_MONITOR_TRANSFORM_180:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_180:
    portrait = TRUE;
    break;
  case PHOSH_MONITOR_TRANSFORM_90:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_90:
  case PHOSH_MONITOR_TRANSFORM_270:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_270:
    portrait = FALSE;
    break;
  default:
    g_warn_if_reached();
    portrait = TRUE;
  }

  /* If we have a landscape monitor (tv, laptop) flip the rotation */
  monitor = phosh_rotation_manager_get_monitor (self->manager);
  monitor_is_landscape = ((double)monitor->width / (double)monitor->height) > 1.0;
  portrait = monitor_is_landscape ? !portrait : portrait;

  g_debug ("Potrait: %d, width: %d, height: %d", portrait, monitor->width, monitor->height);
  if (portrait) {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "screen-rotation-portrait-symbolic");
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Portrait"));
  } else {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "screen-rotation-landscape-symbolic");
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Landscape"));
  }
}


static void
on_orientation_lock_changed (PhoshRotateInfo *self)
{
  gboolean locked = phosh_rotation_manager_get_orientation_locked (self->manager);
  const char *icon_name;

  if (phosh_rotation_manager_get_mode (self->manager) != PHOSH_ROTATION_MANAGER_MODE_SENSOR)
    return;

  g_debug ("Orientation locked: %d", locked);

  icon_name = locked ? "rotation-locked-symbolic" : "rotation-allowed-symbolic";
  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
  /* Translators: Automatic screen orientation is either on (enabled) or off (locked/disabled) */
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), locked ? _("Off") : _("On"));

  return;
}


static void
on_mode_changed (PhoshRotateInfo *self)
{
  PhoshRotationManagerMode mode = phosh_rotation_manager_get_mode (self->manager);

  g_debug ("Rotation manager mode: %d", mode);
  switch (mode) {
  case PHOSH_ROTATION_MANAGER_MODE_OFF:
    on_transform_changed (self);
    break;
  case PHOSH_ROTATION_MANAGER_MODE_SENSOR:
    on_orientation_lock_changed (self);
    break;
  default:
    g_assert_not_reached ();
  }
}


static void
phosh_rotate_info_class_init (PhoshRotateInfoClass *klass)
{
}


static void
phosh_rotate_info_init (PhoshRotateInfo *self)
{
  self->manager = phosh_shell_get_rotation_manager (phosh_shell_get_default());

  /* We don't use property bindings since we flip info/icon based on rotation and lock */
  g_signal_connect_object (self->manager,
                           "notify::transform",
                           G_CALLBACK (on_transform_changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager,
                           "notify::orientation-locked",
                           G_CALLBACK (on_orientation_lock_changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager,
                           "notify::mode",
                           G_CALLBACK (on_mode_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_mode_changed (self);
}
