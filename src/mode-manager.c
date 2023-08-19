/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mode-manager"

#include "phosh-config.h"

#include "mode-manager.h"
#include "shell.h"
#include "util.h"
#include "dbus/hostname1-dbus.h"

#define BUS_NAME "org.freedesktop.hostname1"
#define OBJECT_PATH "/org/freedesktop/hostname1"

#define PHOC_KEY_MAXIMIZE "auto-maximize"
#define A11Y_KEY_OSK "screen-keyboard-enabled"
#define WM_KEY_LAYOUT "button-layout"

/**
 * PhoshModeManager:
 *
 * Determines the device mode
 *
 * #PhoshModeManager tracks the device mode and attached hardware.
 */

enum {
  PROP_0,
  PROP_DEVICE_TYPE,
  PROP_HW_FLAGS,
  PROP_MIMICRY,

  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshModeManager {
  PhoshManager                 parent;

  PhoshModeDeviceType          device_type;
  PhoshModeDeviceType          mimicry;
  PhoshModeHwFlags             hw_flags;

  PhoshMonitorManager         *monitor_manager;

  PhoshDBusHostname1          *proxy;
  GCancellable                *cancel;
  gchar                       *chassis;
  PhoshWaylandSeatCapabilities wl_caps;

  /* Tablet mode */
  gboolean                            is_tablet_mode;
  struct zphoc_tablet_mode_switch_v1 *tablet_mode_switch;
};
G_DEFINE_TYPE (PhoshModeManager, phosh_mode_manager, PHOSH_TYPE_MANAGER);


static void
phosh_mode_manager_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (object);

  switch (property_id) {
  case PROP_HW_FLAGS:
    g_value_set_flags (value, self->hw_flags);
    break;
  case PROP_DEVICE_TYPE:
    g_value_set_enum (value, self->device_type);
    break;
  case PROP_MIMICRY:
    g_value_set_enum (value, self->mimicry);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
has_external_display (PhoshModeManager *self)
{
  int n_monitors;
  PhoshMonitor *primary;
  PhoshShell *shell = phosh_shell_get_default ();

  n_monitors = phosh_monitor_manager_get_num_monitors (self->monitor_manager);

  /* We assume only one display can be built in */
  if (n_monitors > 1)
    return TRUE;

  primary = phosh_shell_get_primary_monitor (shell);
  if (primary == NULL)
    return FALSE;

  /* Single monitor is builtin */
  if (phosh_shell_get_builtin_monitor (shell) == primary)
    return FALSE;

  /* Single monitor is virtual (e.g. in automatic tests) */
  if (primary->conn_type == PHOSH_MONITOR_CONNECTOR_TYPE_VIRTUAL)
    return FALSE;

  /* primary display is not built-in */
  return TRUE;
}


static void
update_props (PhoshModeManager *self)
{
  PhoshModeDeviceType device_type, mimicry;
  PhoshModeHwFlags hw;

  /* Self->Chassis type */
  hw = PHOSH_MODE_HW_NONE;
  if (g_strcmp0 (self->chassis, "handset") == 0) {
    device_type = PHOSH_MODE_DEVICE_TYPE_PHONE;
  } else if (g_strcmp0 (self->chassis, "laptop") == 0) {
    device_type = PHOSH_MODE_DEVICE_TYPE_LAPTOP;
    hw |= PHOSH_MODE_HW_KEYBOARD;
  } else if (g_strcmp0 (self->chassis, "desktop") == 0) {
    device_type = PHOSH_MODE_DEVICE_TYPE_DESKTOP;
    hw |= PHOSH_MODE_HW_KEYBOARD;
  } else if (g_strcmp0 (self->chassis, "convertible") == 0) {
    device_type = PHOSH_MODE_DEVICE_TYPE_CONVERTIBLE;
  } else if (g_strcmp0 (self->chassis, "tablet") == 0) {
    device_type = PHOSH_MODE_DEVICE_TYPE_TABLET;
  } else {
    device_type = PHOSH_MODE_DEVICE_TYPE_UNKNOWN;
  }
  mimicry = device_type;

  /* Additional hardware */
  if (has_external_display (self))
    hw |= PHOSH_MODE_HW_EXT_DISPLAY;

  if (self->wl_caps & PHOSH_WAYLAND_SEAT_CAPABILITY_POINTER)
    hw |= PHOSH_MODE_HW_POINTER;

  /* Mimicries */
  if (device_type == PHOSH_MODE_DEVICE_TYPE_PHONE &&
      (hw & PHOSH_MODE_DOCKED_PHONE_MASK) == PHOSH_MODE_DOCKED_PHONE_MASK) {
    mimicry = PHOSH_MODE_DEVICE_TYPE_DESKTOP;
  } else if (device_type == PHOSH_MODE_DEVICE_TYPE_TABLET &&
      (hw & PHOSH_MODE_DOCKED_TABLET_MASK) == PHOSH_MODE_DOCKED_TABLET_MASK) {
    mimicry = PHOSH_MODE_DEVICE_TYPE_DESKTOP;
  } else if (device_type == PHOSH_MODE_DEVICE_TYPE_CONVERTIBLE) {
    if (self->is_tablet_mode < 0 || self->is_tablet_mode == FALSE)
      mimicry = PHOSH_MODE_DEVICE_TYPE_LAPTOP;
    else
      mimicry = PHOSH_MODE_DEVICE_TYPE_TABLET;
  }

  g_object_freeze_notify (G_OBJECT (self));

  if (device_type != self->device_type) {
    g_autofree char *name = g_enum_to_string (PHOSH_TYPE_MODE_DEVICE_TYPE, device_type);

    self->device_type = device_type;
    g_debug ("Device type is %s", name);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DEVICE_TYPE]);
  }

  if (mimicry != self->mimicry) {
    g_autofree char *name = g_enum_to_string (PHOSH_TYPE_MODE_DEVICE_TYPE, mimicry);

    self->mimicry = mimicry;
    g_debug ("Mimicry is %s", name);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MIMICRY]);
  }

  if (hw != self->hw_flags) {
    g_autofree char *names = g_flags_to_string (PHOSH_TYPE_MODE_HW_FLAGS, hw);
    self->hw_flags = hw;
    g_debug ("HW flags %s", names);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HW_FLAGS]);
  }

  g_object_thaw_notify (G_OBJECT (self));
}


static void
on_n_monitors_changed (PhoshModeManager *self, GParamSpec *pspec, PhoshMonitorManager *manager)
{
  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (manager));

  update_props (self);
}


static void
on_chassis_changed (PhoshModeManager   *self,
                    GParamSpec         *pspec,
                    PhoshDBusHostname1 *proxy)
{
  const gchar *chassis;

  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_HOSTNAME1 (proxy));

  chassis = phosh_dbus_hostname1_get_chassis (self->proxy);

  if (!chassis)
    return;

  g_debug ("Chassis: %s", chassis);
  g_free (self->chassis);
  self->chassis = g_strdup (chassis);
  update_props (self);
}


static void
tablet_mode_switch_disabled (void *data, struct zphoc_tablet_mode_switch_v1 *zphoc_tablet_mode_switch_v1)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (data);

  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));

  g_debug ("Tablet mode disabled");

  self->is_tablet_mode = FALSE;
  update_props (self);
}

static void
tablet_mode_switch_enabled (void *data, struct zphoc_tablet_mode_switch_v1 *zphoc_tablet_mode_switch_v1)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (data);

  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));

  g_debug ("Tablet mode enabled");

  self->is_tablet_mode = TRUE;
  update_props (self);
}


const struct zphoc_tablet_mode_switch_v1_listener tablet_mode_switch_listener = {
  .disabled = tablet_mode_switch_disabled,
  .enabled = tablet_mode_switch_enabled,
};


static void
register_tablet_mode_switch (PhoshModeManager *self)
{
  if (self->tablet_mode_switch)
    return;

  self->tablet_mode_switch = zphoc_device_state_v1_get_tablet_mode_switch (
    phosh_wayland_get_zphoc_device_state_v1 (phosh_wayland_get_default ()));
  zphoc_tablet_mode_switch_v1_add_listener (self->tablet_mode_switch, &tablet_mode_switch_listener, self);
}


static void
on_seat_capabilities_changed (PhoshModeManager *self,
                              GParamSpec       *pspec,
                              PhoshWayland     *wl)
{
  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));
  g_return_if_fail (PHOSH_IS_WAYLAND (wl));

  self->wl_caps = phosh_wayland_get_seat_capabilities (wl);

  if (self->wl_caps & PHOSH_WAYLAND_SEAT_CAPABILITY_TABLET_MODE_SWITCH)
    register_tablet_mode_switch (self);

  update_props (self);
}


static void
on_proxy_new_for_bus_finish (GObject          *source_object,
                             GAsyncResult     *res,
                             PhoshModeManager *self)
{
  g_autoptr (GError) err = NULL;
  PhoshWayland *wl;
  PhoshDBusHostname1 *proxy;

  proxy = phosh_dbus_hostname1_proxy_new_for_bus_finish (res, &err);
  if (proxy == NULL) {
    phosh_async_error_warn (err, "Failed to get hostname1 proxy");
    return;
  }
  g_return_if_fail (PHOSH_IS_MODE_MANAGER (self));
  self->proxy = proxy;

  g_debug ("Hostname1 interface initialized");
  g_signal_connect_object (self->proxy,
                           "notify::chassis",
                           G_CALLBACK (on_chassis_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_chassis_changed (self, NULL, self->proxy);

  wl = phosh_wayland_get_default ();
  g_signal_connect_object (wl,
                           "notify::seat-capabilities",
                           G_CALLBACK (on_seat_capabilities_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_seat_capabilities_changed (self, NULL, wl);

  g_signal_connect_object (self->monitor_manager,
                           "notify::n-monitors",
                           G_CALLBACK (on_n_monitors_changed),
                           self,
                           G_CONNECT_SWAPPED);
  /* n_monitors is always updated in update_props () */
}


static void
phosh_mode_manager_idle_init (PhoshManager *manager)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (manager);

  phosh_dbus_hostname1_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          BUS_NAME,
                                          OBJECT_PATH,
                                          self->cancel,
                                          (GAsyncReadyCallback) on_proxy_new_for_bus_finish,
                                          self);
}

static void
phosh_mode_manager_constructed (GObject *object)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (object);

  G_OBJECT_CLASS (phosh_mode_manager_parent_class)->constructed (object);

  self->monitor_manager = phosh_shell_get_monitor_manager (phosh_shell_get_default ());
}


static void
phosh_mode_manager_dispose (GObject *object)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (object);

  g_clear_pointer (&self->tablet_mode_switch, zphoc_tablet_mode_switch_v1_destroy);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_mode_manager_parent_class)->dispose (object);
}


static void
phosh_mode_manager_finalize (GObject *object)
{
  PhoshModeManager *self = PHOSH_MODE_MANAGER (object);

  g_clear_pointer (&self->chassis, g_free);

  G_OBJECT_CLASS (phosh_mode_manager_parent_class)->finalize (object);
}


static void
phosh_mode_manager_class_init (PhoshModeManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->constructed = phosh_mode_manager_constructed;
  object_class->dispose = phosh_mode_manager_dispose;
  object_class->finalize = phosh_mode_manager_finalize;
  object_class->get_property = phosh_mode_manager_get_property;

  manager_class->idle_init = phosh_mode_manager_idle_init;

  props[PROP_DEVICE_TYPE] =
    g_param_spec_enum ("device-type",
                       "Device Type",
                       "The device type",
                       PHOSH_TYPE_MODE_DEVICE_TYPE,
                       PHOSH_MODE_DEVICE_TYPE_PHONE,
                       G_PARAM_READABLE |
                       G_PARAM_EXPLICIT_NOTIFY |
                       G_PARAM_STATIC_STRINGS);

  props[PROP_HW_FLAGS] =
    g_param_spec_flags ("hw-flags",
                        "Hardware flags",
                        "Flags for available hardware",
                        PHOSH_TYPE_MODE_HW_FLAGS,
                        PHOSH_MODE_HW_NONE,
                        G_PARAM_READABLE |
                        G_PARAM_EXPLICIT_NOTIFY |
                        G_PARAM_STATIC_STRINGS);

  /**
   * PhoshMode:device-mimicry:
   *
   * What this device plus external hardware should be handled
   * like. E.g. a phone with keyboard and mouse and 2nd screen looks
   * much like a desktop. A touch laptop with removable keyboard can
   * look like a tablet.
   */
  props[PROP_MIMICRY] =
    g_param_spec_enum ("mimicry",
                       "Device Mimicry",
                       "The device mimicry",
                       PHOSH_TYPE_MODE_DEVICE_TYPE,
                       PHOSH_MODE_DEVICE_TYPE_PHONE,
                       G_PARAM_READABLE |
                       G_PARAM_EXPLICIT_NOTIFY |
                       G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_mode_manager_init (PhoshModeManager *self)
{
  self->hw_flags = PHOSH_MODE_HW_NONE;
  self->device_type = PHOSH_MODE_DEVICE_TYPE_UNKNOWN;
  self->mimicry = PHOSH_MODE_DEVICE_TYPE_UNKNOWN;
  self->cancel = g_cancellable_new ();
  self->is_tablet_mode = -1;
}


PhoshModeManager *
phosh_mode_manager_new (void)
{
  return PHOSH_MODE_MANAGER (g_object_new (PHOSH_TYPE_MODE_MANAGER, NULL));
}


PhoshModeDeviceType
phosh_mode_manager_get_device_type (PhoshModeManager *self)
{
  g_return_val_if_fail (PHOSH_IS_MODE_MANAGER (self), PHOSH_MODE_DEVICE_TYPE_UNKNOWN);

  return self->device_type;
}


PhoshModeDeviceType
phosh_mode_manager_get_mimicry (PhoshModeManager *self)
{
  g_return_val_if_fail (PHOSH_IS_MODE_MANAGER (self), PHOSH_MODE_DEVICE_TYPE_UNKNOWN);

  return self->mimicry;
}
