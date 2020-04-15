/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-osk-manager"

#include "lockscreen-manager.h"
#include "osk-manager.h"
#include "phosh-osk0-dbus.h"
#include "shell.h"

#include <gio/gio.h>

#define VIRTBOARD_DBUS_NAME      "sm.puri.OSK0"
#define VIRTBOARD_DBUS_OBJECT    "/sm/puri/OSK0"

/**
 * SECTION:phosh-osk-manager
 * @short_description: A manager that handles the OSK
 * @Title: PhoshOsk
 *
 * The #PhoshOskManager is responsible for handling the on screen keyboard
 */
enum {
  PHOSH_OSK_MANAGER_PROP_0,
  PHOSH_OSK_MANAGER_PROP_AVAILABLE,
  PHOSH_OSK_MANAGER_PROP_VISIBLE,
  PHOSH_OSK_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_OSK_MANAGER_PROP_LAST_PROP];

struct _PhoshOskManager
{
  GObject parent;

  /* Currently the only impl. We can use an interface once we support
   * different OSK types */
  PhoshOsk0SmPuriOSK0 *proxy;
  PhoshLockscreenManager *lockscreen_manger;
  gboolean visible;
  gboolean available;
};
G_DEFINE_TYPE (PhoshOskManager, phosh_osk_manager, G_TYPE_OBJECT)


static void
on_osk_show (GObject             *source_object,
             GAsyncResult        *res,
             gpointer             user_data)
{
  GVariant *variant;
  GError *err = NULL;

  variant = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &err);
  if (!variant) {
    g_warning ("Unable to toggle OSK: %s", err->message);
    g_clear_error (&err);
    return;
  }
  g_variant_unref (variant);
}


static void
dbus_props_changed_cb (PhoshOsk0SmPuriOSK0 *proxy,
                       GVariant *changed_properties,
                       GStrv invaliated,
                       gpointer *data)
{
  PhoshOskManager *self = PHOSH_OSK_MANAGER (data);
  char *property;
  GVariantIter i;
  GVariant *value;

  g_variant_iter_init (&i, changed_properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, &value)) {
    g_debug ("OSK property '%s' changed", property);
    if (strcmp (property, "Visible") == 0) {
      phosh_osk_manager_set_visible (self, g_variant_get_boolean (value));
    }
    g_variant_unref (value);
  }
}


static void
dbus_name_owner_changed_cb (PhoshOskManager *self, gpointer data)
{
  g_autofree char *name_owner = NULL;

  g_return_if_fail (PHOSH_IS_OSK_MANAGER (self));

  name_owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (self->proxy));
  g_debug ("OSK bus '%s' owned by %s", VIRTBOARD_DBUS_NAME, name_owner ? name_owner : "nobody");

  self->available = name_owner ? TRUE : FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_OSK_MANAGER_PROP_AVAILABLE]);
}


static void
on_availability_changed (PhoshOskManager *self, GParamSpec *pspec, gpointer unused)
{
  g_return_if_fail (PHOSH_IS_OSK_MANAGER (self));

  /* When there's no OSK we always want the manager to be unpressed */
  if (!self->available)
    phosh_osk_manager_set_visible (self, FALSE);
}


static void
on_lockscreen_manager_locked (PhoshOskManager *self, GParamSpec *pspec, gpointer unused)
{
  g_return_if_fail (PHOSH_IS_OSK_MANAGER (self));
  phosh_osk_manager_set_visible (self, FALSE);
}


static void
phosh_osk_manager_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshOskManager *self = PHOSH_OSK_MANAGER (object);

  switch (property_id) {
  case PHOSH_OSK_MANAGER_PROP_VISIBLE:
    phosh_osk_manager_set_visible (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_osk_manager_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshOskManager *self = PHOSH_OSK_MANAGER (object);

  switch (property_id) {
  case PHOSH_OSK_MANAGER_PROP_VISIBLE:
    g_value_set_boolean (value, self->visible);
    break;
  case PHOSH_OSK_MANAGER_PROP_AVAILABLE:
    g_value_set_boolean (value, self->available);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_osk_manager_constructed (GObject *object)
{
  PhoshOskManager *self = PHOSH_OSK_MANAGER (object);
  PhoshShell *shell;
  GError *err = NULL;

  G_OBJECT_CLASS (phosh_osk_manager_parent_class)->constructed (object);

  self->proxy = phosh_osk0_sm_puri_osk0_proxy_new_for_bus_sync(
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
    VIRTBOARD_DBUS_NAME,
    VIRTBOARD_DBUS_OBJECT,
    NULL,
    &err);

  if (self->proxy == NULL) {
    g_warning ("Failed to register with osk: %s", err->message);
    g_clear_error (&err);
    g_return_if_fail (self->proxy);
  }

  g_signal_connect (
    self->proxy,
    "g-properties-changed",
    G_CALLBACK (dbus_props_changed_cb),
    self);

  g_signal_connect_swapped (
    self->proxy,
    "notify::g-name-owner",
    G_CALLBACK (dbus_name_owner_changed_cb),
    self);
  dbus_name_owner_changed_cb (self, NULL);

  g_signal_connect (self,
                    "notify::available",
                    G_CALLBACK (on_availability_changed),
                    NULL);

  shell = phosh_shell_get_default();
  self->lockscreen_manger = g_object_ref(phosh_shell_get_lockscreen_manager(shell));

  g_signal_connect_swapped (self->lockscreen_manger,
                            "notify::locked",
                            G_CALLBACK (on_lockscreen_manager_locked),
                            self);

  phosh_osk_manager_set_visible (self, phosh_osk0_sm_puri_osk0_get_visible (self->proxy));
}


static void
phosh_osk_manager_dispose (GObject *object)
{
  PhoshOskManager *self = PHOSH_OSK_MANAGER (object);

  g_clear_object (&self->proxy);
  g_clear_object (&self->lockscreen_manger);
  G_OBJECT_CLASS (phosh_osk_manager_parent_class)->dispose (object);
}


static void
phosh_osk_manager_class_init (PhoshOskManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_osk_manager_constructed;
  object_class->dispose = phosh_osk_manager_dispose;

  object_class->set_property = phosh_osk_manager_set_property;
  object_class->get_property = phosh_osk_manager_get_property;

  props[PHOSH_OSK_MANAGER_PROP_AVAILABLE] =
    g_param_spec_boolean ("available",
                          "available",
                          "Whether an OSK is available",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  props[PHOSH_OSK_MANAGER_PROP_VISIBLE] =
    g_param_spec_boolean ("visible",
                          "visible",
                          "Whether the OSK is currently visible",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PHOSH_OSK_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_osk_manager_init (PhoshOskManager *self)
{
}


PhoshOskManager *
phosh_osk_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_OSK_MANAGER, NULL);
}


gboolean
phosh_osk_manager_get_available (PhoshOskManager *self)
{
  g_return_val_if_fail (PHOSH_IS_OSK_MANAGER (self), FALSE);

  return self->available;
}


gboolean
phosh_osk_manager_get_visible (PhoshOskManager *self)
{
  g_return_val_if_fail (PHOSH_IS_OSK_MANAGER (self), FALSE);

  return self->visible;
}


void
phosh_osk_manager_set_visible (PhoshOskManager *self, gboolean visible)
{
  g_return_if_fail (PHOSH_IS_OSK_MANAGER (self));

  g_debug ("Setting osk to %svisible", visible ? "" : "not ");
  if (self->visible == visible)
    return;

  if (self->proxy) {
    phosh_osk0_sm_puri_osk0_call_set_visible (
      self->proxy,
      visible,
      NULL,
      (GAsyncReadyCallback) on_osk_show,
      NULL);
  }
  self->visible = visible;

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_OSK_MANAGER_PROP_VISIBLE]);
}
