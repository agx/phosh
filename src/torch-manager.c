/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-torch-manager"

#include "config.h"

#include "torch-manager.h"
#include "shell.h"
#include "dbus/upower-torch-dbus.h"

#define BUS_NAME "org.freedesktop.UPower"
#define OBJECT_PATH "/org/freedesktop/UPower/Torch"

#define TORCH_DISABLED_ICON "torch-disabled-symbolic"
#define TORCH_ENABLED_ICON  "torch-enabled-symbolic"

/**
 * SECTION:torch-manager
 * @short_description: Interacts with torch via UPower
 * @Title: PhoshTorchManager
 *
 * #PhoshTorchManager tracks the torch status and
 * allows to set the brightness.
 */

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_BRIGHTNESS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshTorchManager {
  GObject               parent;

  /* Whether upower sees a torch device */
  gboolean              present;
  const char           *icon_name;
  int                   brightness;
  int                   max_brightness;
  int                   last_brightness;

  PhoshUPowerDBusTorch *proxy;
};
G_DEFINE_TYPE (PhoshTorchManager, phosh_torch_manager, G_TYPE_OBJECT);


static void
on_brightness_set (PhoshUPowerDBusTorch *proxy,
                   GAsyncResult         *res,
                   PhoshTorchManager    *self)
{
  g_autoptr (GError) err = NULL;

  if (!phosh_upower_dbus_torch_call_set_brightness_finish (proxy, res, &err)) {
    g_warning ("Failed to set torch brigthness: %s", err->message);
  }
  g_object_unref (self);
}


static void
set_brightness (PhoshTorchManager *self, int brightness)
{
  phosh_upower_dbus_torch_call_set_brightness (self->proxy, brightness,
                                               NULL,
                                               (GAsyncReadyCallback) on_brightness_set,
                                               g_object_ref (self));
}

static void
phosh_torch_manager_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshTorchManager *self = PHOSH_TORCH_MANAGER (object);

  switch (property_id) {
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, !!self->brightness);
    break;
  case PROP_BRIGHTNESS:
    g_value_set_int (value, self->brightness);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_torch_brightness_changed (PhoshTorchManager    *self,
                             GParamSpec           *pspec,
                             PhoshUPowerDBusTorch *proxy)
{
  int brightness;
  const char *icon_name;

  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (PHOSH_UPOWER_DBUS_IS_TORCH (proxy));

  brightness = phosh_upower_dbus_torch_get_brightness (proxy);
  if (brightness == self->brightness)
    return;

  g_debug ("Torch brightness: %d", brightness);

  g_object_freeze_notify (G_OBJECT (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BRIGHTNESS]);

  if (!!self->brightness != !!brightness)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
  self->brightness = brightness;

  icon_name = self->brightness ? TORCH_ENABLED_ICON : TORCH_DISABLED_ICON;
  if (icon_name != self->icon_name) {
    self->icon_name = icon_name;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }

  g_object_thaw_notify (G_OBJECT (self));
}


static void
on_get_max_brightness_finished (PhoshUPowerDBusTorch *proxy,
                                GAsyncResult         *res,
                                PhoshTorchManager    *self)
{
  g_autoptr (GError) err = NULL;
  gboolean present = TRUE;
  int value;

  if (phosh_upower_dbus_torch_call_get_max_brightness_finish (proxy, &value, res, &err)) {
    self->max_brightness = value;
  } else {
    g_debug ("Failed to get torch brightness: %s", err->message);
    present = FALSE;
  }

  if (self->present != present) {
    self->present = present;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
  }

  g_debug ("Torch present: %d", self->present);
  g_object_unref (self);
}


static void
on_name_owner_changed (PhoshTorchManager    *self,
                       GParamSpec           *pspec,
                       PhoshUPowerDBusTorch *proxy)
{
  g_autofree char *owner = NULL;
  gboolean present;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (G_IS_DBUS_PROXY (proxy));

  owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (proxy));
  present = owner ? TRUE : FALSE;
  /* Name owner went away */
  if (!present && self->present) {
    self->present = present;
    g_debug ("Torch present: %d", self->present);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
    return;
  }

  /* Check max brightness as indicator if we can drive that DBus interface */
  phosh_upower_dbus_torch_call_get_max_brightness (
    proxy, NULL, (GAsyncReadyCallback)on_get_max_brightness_finished,
    g_object_ref (self));
}


static void
on_proxy_new_for_bus_finish (GObject           *source_object,
                             GAsyncResult      *res,
                             PhoshTorchManager *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));

  self->proxy = phosh_upower_dbus_torch_proxy_new_for_bus_finish (res, &err);

  if (!self->proxy) {
    g_warning ("Failed to get upower torch proxy: %s", err->message);
    goto out;
  }

  g_signal_connect_object (self->proxy,
                           "notify::g-name-owner",
                           G_CALLBACK (on_name_owner_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_name_owner_changed (self, NULL, self->proxy);
  g_signal_connect_object (self->proxy,
                           "notify::brightness",
                           G_CALLBACK (on_torch_brightness_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_torch_brightness_changed (self, NULL, self->proxy);

  g_debug ("Torch manager initialized");
out:
  g_object_unref (self);
}


static gboolean
on_idle (PhoshTorchManager *self)
{
  phosh_upower_dbus_torch_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                             G_DBUS_PROXY_FLAGS_NONE,
                                             BUS_NAME,
                                             OBJECT_PATH,
                                             NULL,
                                             (GAsyncReadyCallback) on_proxy_new_for_bus_finish,
                                             g_object_ref (self));
  return G_SOURCE_REMOVE;
}


static void
phosh_torch_manager_dispose (GObject *object)
{
  PhoshTorchManager *self = PHOSH_TORCH_MANAGER(object);

  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_torch_manager_parent_class)->dispose (object);
}


static void
phosh_torch_manager_class_init (PhoshTorchManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_torch_manager_get_property;
  object_class->dispose = phosh_torch_manager_dispose;

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The torch icon name",
                         "torch-disabled-symbolic",
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether torch is enabled",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether a torch led is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_BRIGHTNESS] =
    g_param_spec_int ("brightness",
                      "Brightness",
                      "The torch brightness",
                      0,
                      G_MAXINT,
                      0,
                      G_PARAM_READABLE |
                      G_PARAM_EXPLICIT_NOTIFY |
                      G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_torch_manager_init (PhoshTorchManager *self)
{
  self->icon_name = TORCH_DISABLED_ICON;
  self->max_brightness = 1;
  /* Perform DBus setup when idle */
  g_idle_add ((GSourceFunc)on_idle, self);
}


PhoshTorchManager *
phosh_torch_manager_new (void)
{
  return PHOSH_TORCH_MANAGER (g_object_new (PHOSH_TYPE_TORCH_MANAGER, NULL));
}


const char *
phosh_torch_manager_get_icon_name (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), NULL);

  return self->icon_name;
}


gboolean
phosh_torch_manager_get_enabled (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), FALSE);

  return !!self->brightness;
}


gboolean
phosh_torch_manager_get_present (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), FALSE);

  return self->present;
}


int
phosh_torch_manager_get_brightness (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), 0);

  return self->brightness;
}

/**
 * phosh_torch_manager_get_scaled_brightness:
 * @self: The #PhoshTorchManager
 *
 * Gets the current brightness as fraction between 0 (off) and 1 (maximum brightness)
 */
double
phosh_torch_manager_get_scaled_brightness (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), 0);

  return (double)self->brightness / (double)self->max_brightness;
}

/**
 * phosh_torch_manager_set_scaled_brightness:
 * @self: The #PhoshTorchManager
 * @frac: The brightness as fraction
 *
 * Sets the current brightness as fraction between 0 (off) and 1 (maximum brightness)
 */
void
phosh_torch_manager_set_scaled_brightness (PhoshTorchManager *self, double frac)
{
  int brightness;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (frac >= 0.0 && frac <= 1.0);

  brightness = round (frac * self->max_brightness);
  if (brightness > self->max_brightness)
    brightness = self->max_brightness;

  set_brightness (self, brightness);
}


int
phosh_torch_manager_get_max_brightness (PhoshTorchManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TORCH_MANAGER (self), 0);

  return self->max_brightness;
}


void
phosh_torch_manager_toggle (PhoshTorchManager *self)
{
  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (PHOSH_UPOWER_DBUS_IS_TORCH (self->proxy));

  if (self->brightness) {
    g_debug ("Disabling torch");
    self->last_brightness = self->brightness;
    set_brightness (self, 0);
  } else {
    if (self->last_brightness == 0) {
      self->last_brightness = self->max_brightness;
      g_debug ("Last brightness: %d", self->last_brightness);
    }
    g_debug ("Setting torch brightness to %d", self->last_brightness);
    set_brightness (self, self->last_brightness);
  }
}
