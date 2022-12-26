/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-bt-manager"

#include "phosh-config.h"

#include "bt-manager.h"
#include "shell.h"
#include "dbus/gsd-rfkill-dbus.h"
#include "util.h"

#define BUS_NAME "org.gnome.SettingsDaemon.Rfkill"
#define OBJECT_PATH "/org/gnome/SettingsDaemon/Rfkill"


/**
 * PhoshBtManager:
 *
 * Tracks the Bluetooth status
 *
 * #PhoshBtManager tracks the Bluetooth status that
 * is whether the adapter is present and enabled.
 */

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_ENABLED,
  PROP_PRESENT,
  /* TODO: keep track of connected devices for quick-settings */
  /* PROP_N_DEVICES */
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshBtManager {
  PhoshManager           manager;

  /* Whether bt radio is on */
  gboolean               enabled;
  /* Whether we have a bt device is present */
  gboolean               present;
  const char            *icon_name;

  PhoshRfkillDBusRfkill *proxy;
};
G_DEFINE_TYPE (PhoshBtManager, phosh_bt_manager, PHOSH_TYPE_MANAGER);


static void
phosh_bt_manager_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (object);

  switch (property_id) {
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_bt_airplane_mode_changed (PhoshBtManager        *self,
                             GParamSpec            *pspec,
                             PhoshRfkillDBusRfkill *proxy)
{
  gboolean enabled;
  const char *icon_name;

  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));
  g_return_if_fail (PHOSH_RFKILL_DBUS_IS_RFKILL (proxy));

  enabled = !phosh_rfkill_dbus_rfkill_get_bluetooth_airplane_mode (proxy) && self->present;

  if (enabled == self->enabled)
    return;

  self->enabled = enabled;

  g_debug ("BT enabled: %d", self->enabled);
  if (enabled)
    icon_name = "bluetooth-active-symbolic";
  else
    icon_name = "bluetooth-disabled-symbolic";

  if (icon_name != self->icon_name) {
    self->icon_name = icon_name;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
on_bt_has_airplane_mode_changed (PhoshBtManager        *self,
                                 GParamSpec            *pspec,
                                 PhoshRfkillDBusRfkill *proxy)
{
  gboolean present;

  present = phosh_rfkill_dbus_rfkill_get_bluetooth_has_airplane_mode (proxy);

  if (present == self->present)
    return;

  /* Having a BT adapter that supports airplane mode seems to be the best
     indicator for having a device at all */
  self->present = present;
  g_debug ("BT present: %d", self->present);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);

  /* Sync up in case `enabled` got flipped first */
  on_bt_airplane_mode_changed (self, NULL, proxy);
}


static void
on_proxy_new_for_bus_finish (GObject        *source_object,
                             GAsyncResult   *res,
                             PhoshBtManager *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));

  self->proxy = phosh_rfkill_dbus_rfkill_proxy_new_for_bus_finish (res, &err);

  if (!self->proxy) {
    phosh_dbus_service_error_warn (err, "Failed to get gsd rfkill proxy");
    goto out;
  }

  g_object_connect (self->proxy,
                    "swapped_object_signal::notify::bluetooth-airplane-mode",
                    G_CALLBACK (on_bt_airplane_mode_changed),
                    self,
                    "swapped_object_signal::notify::bluetooth-has-airplane-mode",
                    G_CALLBACK (on_bt_has_airplane_mode_changed),
                    self,
                    NULL);
  on_bt_airplane_mode_changed (self, NULL, self->proxy);
  on_bt_has_airplane_mode_changed (self, NULL, self->proxy);

  g_debug ("BT manager initialized");
out:
  g_object_unref (self);
}


static void
phosh_bt_manager_idle_init (PhoshManager *manager)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (manager);

  phosh_rfkill_dbus_rfkill_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              BUS_NAME,
                                              OBJECT_PATH,
                                              NULL,
                                              (GAsyncReadyCallback) on_proxy_new_for_bus_finish,
                                              g_object_ref (self));
}


static void
phosh_bt_manager_dispose (GObject *object)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (object);

  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_bt_manager_parent_class)->dispose (object);
}


static void
phosh_bt_manager_class_init (PhoshBtManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->dispose = phosh_bt_manager_dispose;

  object_class->get_property = phosh_bt_manager_get_property;

  manager_class->idle_init = phosh_bt_manager_idle_init;

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The bt icon name",
                         "bluetooth-disabled-symbolic",
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether bluetooth hardware is enabled",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether bluettoh hardware is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_bt_manager_init (PhoshBtManager *self)
{
  self->icon_name = "bluetooth-disabled-symbolic";
}


PhoshBtManager *
phosh_bt_manager_new (void)
{
  return PHOSH_BT_MANAGER (g_object_new (PHOSH_TYPE_BT_MANAGER, NULL));
}


const char *
phosh_bt_manager_get_icon_name (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), NULL);

  return self->icon_name;
}


gboolean
phosh_bt_manager_get_enabled (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), FALSE);

  return self->enabled;
}


void
phosh_bt_manager_set_enabled (PhoshBtManager *self, gboolean enabled)
{
  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));

  if (!self->present)
    return;

  if (enabled == self->enabled)
    return;

  g_return_if_fail (self->proxy);

  self->enabled = enabled;
  phosh_rfkill_dbus_rfkill_set_bluetooth_airplane_mode (self->proxy, !enabled);
}


gboolean
phosh_bt_manager_get_present (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), FALSE);

  return self->present;
}
