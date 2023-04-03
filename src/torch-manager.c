/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-torch-manager"

#include "phosh-config.h"

#include <gudev/gudev.h>

#include "torch-manager.h"
#include "shell.h"
#include "util.h"
#include "dbus/login1-session-dbus.h"

#define BUS_NAME "org.freedesktop.login1"
#define OBJECT_PATH "/org/freedesktop/login1/session/auto"

#define TORCH_SUBSYSTEM "leds"

#define TORCH_DISABLED_ICON "torch-disabled-symbolic"
#define TORCH_ENABLED_ICON  "torch-enabled-symbolic"

/**
 * PhoshTorchManager:
 *
 * Interacts with torch via UPower
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
  PhoshManager           parent;

  /* Whether we found a torch device */
  gboolean               present;
  const char            *icon_name;
  int                    brightness;
  int                    max_brightness;
  int                    last_brightness;

  GUdevClient           *udev_client;
  GUdevDevice           *udev_device;

  PhoshDBusLoginSession *proxy;
  GCancellable          *cancel;
};
G_DEFINE_TYPE (PhoshTorchManager, phosh_torch_manager, PHOSH_TYPE_MANAGER);


static void
apply_brightness (PhoshTorchManager *self)
{
  const char *icon_name;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (G_UDEV_IS_DEVICE (self->udev_device));

  g_object_freeze_notify (G_OBJECT (self));

  self->brightness = g_udev_device_get_sysfs_attr_as_int_uncached (self->udev_device,
                                                                   "brightness");
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BRIGHTNESS]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);

  icon_name = self->brightness ? TORCH_ENABLED_ICON : TORCH_DISABLED_ICON;
  if (icon_name != self->icon_name) {
    self->icon_name = icon_name;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }

  g_object_thaw_notify (G_OBJECT (self));
}

static void
on_brightness_set (PhoshDBusLoginSession *proxy,
                   GAsyncResult                       *res,
                   PhoshTorchManager                  *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_LOGIN_SESSION (proxy));

  if (!phosh_dbus_login_session_call_set_brightness_finish (proxy, res, &err)) {
    g_warning ("Failed to set torch brightness: %s", err->message);
    return;
  }

  apply_brightness (self);
}


static void
set_brightness (PhoshTorchManager *self, int brightness)
{
  g_return_if_fail (G_UDEV_IS_DEVICE (self->udev_device));

  if (self->brightness == brightness)
    return;

  g_debug("Setting brightness to %d", brightness);

  phosh_dbus_login_session_call_set_brightness (self->proxy,
                                                TORCH_SUBSYSTEM,
                                                g_udev_device_get_name (self->udev_device),
                                                (guint) brightness,
                                                NULL,
                                                (GAsyncReadyCallback) on_brightness_set,
                                                self);
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
get_first_torch_device (gpointer data, gpointer manager)
{
  PhoshTorchManager *self = PHOSH_TORCH_MANAGER (manager);
  GUdevDevice *device = G_UDEV_DEVICE (data);

  /* Keep only the first torch device found */
  if (!self->udev_device)
    self->udev_device = g_object_ref (device);
}

static gboolean
find_torch_device (PhoshTorchManager *self)
{
  g_autoptr (GUdevEnumerator) udev_enumerator = NULL;
  g_autolist (GUdevDevice) device_list = NULL;

  self->udev_device = NULL;

  udev_enumerator = g_udev_enumerator_new (self->udev_client);
  g_udev_enumerator_add_match_subsystem (udev_enumerator, TORCH_SUBSYSTEM);
  g_udev_enumerator_add_match_name (udev_enumerator, "*:torch");
  g_udev_enumerator_add_match_name (udev_enumerator, "*:flash");

  device_list = g_udev_enumerator_execute (udev_enumerator);
  if (!device_list) {
    g_debug ("Failed to find a torch device");
    return FALSE;
  }

  g_list_foreach (device_list, get_first_torch_device, self);

  if (!self->udev_device) {
    g_warning ("Failed to find a usable torch device");
    return FALSE;
  }

  self->max_brightness = g_udev_device_get_sysfs_attr_as_int (self->udev_device,
                                                              "max_brightness");
  g_debug("Found torch device '%s' with max brightness %d",
          g_udev_device_get_name (self->udev_device), self->max_brightness);
  return TRUE;
}


static void
on_proxy_new_for_bus_finish (GObject           *source_object,
                             GAsyncResult      *res,
                             PhoshTorchManager *self)
{
  g_autoptr (GError) err = NULL;
  PhoshDBusLoginSession *proxy;

  proxy = phosh_dbus_login_session_proxy_new_for_bus_finish (res, &err);
  if (!proxy) {
    phosh_async_error_warn (err, "Failed to get login1 session proxy");
    return;
  }

  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self));
  self->proxy = proxy;

  self->present = find_torch_device (self);
  if (self->present) {
    g_object_freeze_notify (G_OBJECT (self));

    apply_brightness (self);

    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
    g_object_thaw_notify (G_OBJECT (self));
  }
}


static void
phosh_torch_manager_idle_init (PhoshManager *manager)
{
  PhoshTorchManager *self = PHOSH_TORCH_MANAGER (manager);
  const char * const subsystems[] = { TORCH_SUBSYSTEM, NULL };

  self->udev_client = g_udev_client_new (subsystems);
  g_return_if_fail (self->udev_client != NULL);

  self->cancel = g_cancellable_new ();
  phosh_dbus_login_session_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              BUS_NAME,
                                              OBJECT_PATH,
                                              self->cancel,
                                              (GAsyncReadyCallback) on_proxy_new_for_bus_finish,
                                              self);
}


static void
phosh_torch_manager_dispose (GObject *object)
{
  PhoshTorchManager *self = PHOSH_TORCH_MANAGER(object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->proxy);

  g_clear_object (&self->udev_client);
  g_clear_object (&self->udev_device);

  G_OBJECT_CLASS (phosh_torch_manager_parent_class)->dispose (object);
}


static void
phosh_torch_manager_class_init (PhoshTorchManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->get_property = phosh_torch_manager_get_property;
  object_class->dispose = phosh_torch_manager_dispose;

  manager_class->idle_init = phosh_torch_manager_idle_init;

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
  g_return_if_fail (PHOSH_DBUS_IS_LOGIN_SESSION (self->proxy));

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
