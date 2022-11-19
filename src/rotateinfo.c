/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Julian Sparber <julian.sparber@puri.sm>
 */

#define G_LOG_DOMAIN "phosh-rotateinfo"

#include "phosh-config.h"

#include "rotateinfo.h"
#include "shell.h"

/**
 * PhoshRotateInfo:
 *
 * A widget to display the rotate lock status
 *
 * A #PhoshStatusIcon to display the rotation lock status.
 * It can either display whether a rotation lock is currently active or
 * if the output is in portrait/landscape mode.
 */

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _PhoshRotateInfo {
  PhoshStatusIcon       parent;

  PhoshRotationManager *manager;
  gboolean              present;
  gboolean              enabled;
} PhoshRotateInfo;


G_DEFINE_TYPE (PhoshRotateInfo, phosh_rotate_info, PHOSH_TYPE_STATUS_ICON)


static void
phosh_rotation_info_check_enabled (PhoshRotateInfo *self)
{
  gboolean enabled = FALSE;

  if ((phosh_rotation_manager_get_mode (self->manager) == PHOSH_ROTATION_MANAGER_MODE_SENSOR) &&
      phosh_rotation_manager_get_orientation_locked (self->manager) == FALSE) {
    enabled = TRUE;
  }

  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
on_transform_changed (PhoshRotateInfo *self)
{
  PhoshMonitor *monitor = phosh_rotation_manager_get_monitor (self->manager);
  gboolean monitor_is_landscape;
  gboolean portrait;

  if (phosh_rotation_manager_get_mode (self->manager) != PHOSH_ROTATION_MANAGER_MODE_OFF)
    return;

  if (!monitor)
    return;

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
  monitor_is_landscape = ((double)monitor->width / (double)monitor->height) > 1.0;
  portrait = monitor_is_landscape ? !portrait : portrait;

  g_debug ("Portrait: %d, width: %d, height: %d", portrait, monitor->width, monitor->height);
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
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), locked ?
                              C_("automatic-screen-rotation-disabled", "Off") :
                              C_("automatic-screen-rotation-enabled", "On"));
  phosh_rotation_info_check_enabled (self);

  return;
}


static void
on_mode_or_monitor_changed (PhoshRotateInfo *self)
{
  PhoshRotationManagerMode mode = phosh_rotation_manager_get_mode (self->manager);
  gboolean present = !!phosh_rotation_manager_get_monitor (self->manager);

  g_debug ("Rotation manager mode: %d, has-builtin: %d", mode, present);
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

  phosh_rotation_info_check_enabled (self);
  if (self->present == present)
    return;

  self->present = present;
  g_debug ("Built-in monitor present: %d", present);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
}


static void
phosh_rotate_info_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshRotateInfo *self = PHOSH_ROTATE_INFO (object);

  switch (property_id) {
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_rotate_info_class_init (PhoshRotateInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_rotate_info_get_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-rotate-info");

  /**
   * PhoshRotateInfo:present:
   *
   * Whether a builtin display to rotate is present
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshRotateInfo:enabled:
   *
   * Whether automatic rotation is enabled
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_rotate_info_init (PhoshRotateInfo *self)
{
  self->manager = phosh_shell_get_rotation_manager (phosh_shell_get_default());

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "rotation-locked-symbolic");
  /* Translators: Automatic screen orientation is off (locked/disabled) */
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self),
                              C_("automatic-screen-rotation-disabled", "Off"));

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
                           G_CALLBACK (on_mode_or_monitor_changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager,
                           "notify::monitor",
                           G_CALLBACK (on_mode_or_monitor_changed),
                           self,
                           G_CONNECT_SWAPPED);

  on_mode_or_monitor_changed (self);
}
