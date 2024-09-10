/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Anteater <nt8r@protonmail.com>
 */

#define G_LOG_DOMAIN "phosh-wwan-ofono"

#include "phosh-config.h"

#include "phosh-wwan-iface.h"
#include "phosh-wwan-ofono.h"
#include "phosh-wwan-ofono-dbus.h"
#include "util.h"

#define BUS_NAME "org.ofono"
#define OBJECT_PATH "/"

/**
 * PhoshWWanOfono:
 *
 * Implementation of the [iface@Phosh.WWan] interface for Ofono
 *
 * Since: 0.4.5
 */

enum {
  PHOSH_WWAN_OFONO_PROP_0,
  PHOSH_WWAN_OFONO_PROP_SIGNAL_QUALITY,
  PHOSH_WWAN_OFONO_PROP_ACCESS_TEC,
  PHOSH_WWAN_OFONO_PROP_UNLOCKED,
  PHOSH_WWAN_OFONO_PROP_SIM,
  PHOSH_WWAN_OFONO_PROP_PRESENT,
  PHOSH_WWAN_OFONO_PROP_ENABLED,
  PHOSH_WWAN_OFONO_PROP_OPERATOR,
  PHOSH_WWAN_OFONO_PROP_LAST_PROP,
};

typedef struct _PhoshWWanOfono {
  PhoshWWanManager                   parent;

  PhoshOfonoDBusNetworkRegistration *proxy_netreg;
  PhoshOfonoDBusSimManager          *proxy_sim;
  PhoshOfonoDBusManager             *proxy_manager;

  /* Signals we connect to */
  gulong                             manager_object_added_signal_id;
  gulong                             manager_object_removed_signal_id;
  gulong                             proxy_netreg_props_signal_id;
  gulong                             proxy_sim_props_signal_id;

  char                              *object_path;
  guint                              signal_quality;
  const char                        *access_tec;
  gboolean                           locked;
  gboolean                           sim;
  gboolean                           present;
  char                              *operator;
} PhoshWWanOfono;


static void phosh_wwan_ofono_interface_init (PhoshWWanInterface *iface);
G_DEFINE_TYPE_WITH_CODE (PhoshWWanOfono, phosh_wwan_ofono, PHOSH_TYPE_WWAN_MANAGER,
                         G_IMPLEMENT_INTERFACE (PHOSH_TYPE_WWAN,
                                                phosh_wwan_ofono_interface_init))

static void
phosh_wwan_ofono_update_signal_quality (PhoshWWanOfono *self, GVariant *v)
{
  g_return_if_fail (self);
  g_return_if_fail (v);

  self->signal_quality = g_variant_get_byte (v);
  g_object_notify (G_OBJECT (self), "signal-quality");
}


static const char *
phosh_wwan_ofono_user_friendly_access_tec (const char *access_tec)
{
  if (!access_tec)
    return NULL;

  if (g_strcmp0 (access_tec, "gsm") == 0)
    return "2G";
  if (g_strcmp0 (access_tec, "edge") == 0)
    return "2.75G";
  if (g_strcmp0 (access_tec, "umts") == 0)
    return "3G";
  if (g_strcmp0 (access_tec, "hspa") == 0)
    return "3.5G";
  if (g_strcmp0 (access_tec, "lte") == 0)
    return "4G";
  if (g_strcmp0 (access_tec, "nr") == 0)
    return "5G";

  return NULL;
}


static void
phosh_wwan_ofono_update_access_tec (PhoshWWanOfono *self, GVariant *v)
{
  const char *access_tec;

  g_return_if_fail (self);
  g_return_if_fail (v);

  access_tec = g_variant_get_string (v, NULL);
  self->access_tec = phosh_wwan_ofono_user_friendly_access_tec (access_tec);

  g_debug ("Access tec is %s", self->access_tec);
  g_object_notify (G_OBJECT (self), "access-tec");
}


static void
phosh_wwan_ofono_update_operator (PhoshWWanOfono *self, GVariant *v)
{
  const char *operator;

  g_return_if_fail (self);
  g_return_if_fail (v);

  operator = g_variant_get_string (v, NULL);

  if (g_strcmp0 (operator, self->operator)) {
    g_free (self->operator);
    self->operator = g_strdup (operator);

    g_debug("Operator is '%s'", self->operator);
    g_object_notify (G_OBJECT (self), "operator");
  }
}


static void
phosh_wwan_ofono_update_lock_status (PhoshWWanOfono *self, GVariant *v)
{
  const char *pin_required;

  g_return_if_fail (self);
  g_return_if_fail (v);

  /* Whether any kind of PIN is required */
  pin_required = g_variant_get_string (v, NULL);
  self->locked = !!g_strcmp0 (pin_required, "none");

  g_debug ("SIM is %slocked: (%s)", self->locked ? "" : "un", pin_required);
  g_object_notify (G_OBJECT (self), "unlocked");
}


static void
phosh_wwan_ofono_update_sim_status (PhoshWWanOfono *self, GVariant *v)
{
  g_return_if_fail (self);
  g_return_if_fail (v);

  self->sim = g_variant_get_boolean (v);

  g_debug ("SIM is %spresent", self->sim ? "" : "not ");
  g_object_notify (G_OBJECT (self), "sim");
}


static void
phosh_wwan_ofono_update_present (PhoshWWanOfono *self, gboolean present)
{
  g_return_if_fail (self);

  if (self->present != present) {
    g_debug ("Modem is %spresent", present ? "" : "not ");
    self->present = present;
    g_object_notify (G_OBJECT (self), "present");
  }
}


static void
phosh_wwan_ofono_dbus_netreg_update_prop (PhoshOfonoDBusNetworkRegistration *proxy,
                                          const char                        *property,
                                          GVariant                          *value,
                                          PhoshWWanOfono                    *self)
{
  g_debug ("WWAN netreg property %s changed", property);
  if (g_strcmp0 (property, "Strength") == 0)
    phosh_wwan_ofono_update_signal_quality (self, value);
  else if (g_strcmp0 (property, "Technology") == 0)
    phosh_wwan_ofono_update_access_tec (self, value);
  else if (g_strcmp0 (property, "Name") == 0)
    phosh_wwan_ofono_update_operator (self, value);
}


static void
phosh_wwan_ofono_dbus_netreg_prop_changed_cb (PhoshOfonoDBusNetworkRegistration *proxy,
                                              const char *property,
                                              GVariant *value,
                                              PhoshWWanOfono *self)
{
  g_autoptr (GVariant) inner = g_variant_get_variant (value);
  phosh_wwan_ofono_dbus_netreg_update_prop (proxy, property, inner, self);
}


static void
phosh_wwan_ofono_dbus_sim_update_prop (PhoshOfonoDBusSimManager *proxy,
                                       const char               *property,
                                       GVariant                 *value,
                                       PhoshWWanOfono           *self)
{
  g_debug ("WWAN SIM property %s changed", property);
  if (g_strcmp0 (property, "Present") == 0)
    phosh_wwan_ofono_update_sim_status (self, value);
  else if (g_strcmp0 (property, "PinRequired") == 0)
    phosh_wwan_ofono_update_lock_status (self, value);
  else if (g_strcmp0 (property, "ServiceProviderName") == 0)
    phosh_wwan_ofono_update_operator (self, value);
}


static void
phosh_wwan_ofono_dbus_sim_prop_changed_cb (PhoshOfonoDBusSimManager *proxy,
                                           const char               *property,
                                           GVariant                 *value,
                                           PhoshWWanOfono           *self)
{
  g_autoptr (GVariant) inner = g_variant_get_variant(value);
  phosh_wwan_ofono_dbus_sim_update_prop (proxy, property, inner, self);
}


static void
phosh_wwan_ofono_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  /* All props are ro */
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}


static void
phosh_wwan_ofono_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshWWanOfono *self = PHOSH_WWAN_OFONO (object);

  switch (property_id) {
  case PHOSH_WWAN_OFONO_PROP_SIGNAL_QUALITY:
    g_value_set_uint (value, self->signal_quality);
    break;
  case PHOSH_WWAN_OFONO_PROP_ACCESS_TEC:
    g_value_set_string (value, self->access_tec);
    break;
  case PHOSH_WWAN_OFONO_PROP_UNLOCKED:
    g_value_set_boolean (value, !self->locked);
    break;
  case PHOSH_WWAN_OFONO_PROP_SIM:
    g_value_set_boolean (value, self->sim);
    break;
  case PHOSH_WWAN_OFONO_PROP_OPERATOR:
    g_value_set_string (value, self->operator);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_wwan_ofono_destroy_modem (PhoshWWanOfono *self)
{
  g_debug ("destroying modem '%p'", self);

  if (self->proxy_netreg) {
    g_clear_signal_handler (&self->proxy_netreg_props_signal_id,
                            self->proxy_netreg);
    g_clear_object (&self->proxy_netreg);
  }

  if (self->proxy_sim) {
    g_clear_signal_handler (&self->proxy_sim_props_signal_id, self->proxy_sim);
    g_clear_object (&self->proxy_sim);
  }

  g_clear_pointer (&self->object_path, g_free);

  phosh_wwan_ofono_update_present (self, FALSE);

  self->signal_quality = 0;
  g_object_notify (G_OBJECT (self), "signal-quality");

  self->access_tec = NULL;
  g_object_notify (G_OBJECT (self), "access-tec");

  self->locked = TRUE;
  g_object_notify (G_OBJECT (self), "unlocked");

  self->sim = FALSE;
  g_object_notify (G_OBJECT (self), "sim");

  g_clear_pointer (&self->operator, g_free);
  g_object_notify (G_OBJECT (self), "operator");
}


static void
phosh_wwan_ofono_on_sim_get_properties_finish (GObject        *source_object,
                                               GAsyncResult   *res,
                                               PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GVariant) properties = NULL;
  g_autoptr (GVariant) value = NULL;
  char *property;
  GVariantIter i;

  if (!phosh_ofono_dbus_sim_manager_call_get_properties_finish (
    self->proxy_sim,
    &properties,
    res,
    &err)) {
    g_warning ("Failed to get sim proxy properties for %s: %s",
      self->object_path, err->message);
    g_object_unref (self);
    return;
  }

  g_variant_iter_init (&i, properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, &value, NULL)) {
    phosh_wwan_ofono_dbus_sim_update_prop(self->proxy_sim, property, value, self);
    g_clear_pointer (&value, g_variant_unref);
  }

  g_object_unref (self);
}


static void
phosh_wwan_ofono_on_proxy_sim_new_for_bus_finish (GObject        *source_object,
                                                  GAsyncResult   *res,
                                                  PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;

  self->proxy_sim = phosh_ofono_dbus_sim_manager_proxy_new_for_bus_finish (
    res,
    &err);

  g_debug("proxy_sim finish '%p'", self->proxy_sim);

  if (!self->proxy_sim) {
    g_warning ("Failed to get sim proxy for %s: %s",
      self->object_path, err->message);
    g_object_unref (self);
    return;
  }

  phosh_ofono_dbus_sim_manager_call_get_properties (
    self->proxy_sim,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_sim_get_properties_finish,
    self);

  self->proxy_sim_props_signal_id = g_signal_connect (self->proxy_sim,
                                                     "property-changed",
                                                     G_CALLBACK (phosh_wwan_ofono_dbus_sim_prop_changed_cb),
                                                     self);
}


static void
phosh_wwan_ofono_on_netreg_get_properties_finish (GObject        *source_object,
                                                  GAsyncResult   *res,
                                                  PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GVariant) properties = NULL;
  g_autoptr (GVariant) value = NULL;
  char *property;
  GVariantIter i;

  if (!phosh_ofono_dbus_network_registration_call_get_properties_finish (
    self->proxy_netreg,
    &properties,
    res,
    &err)) {
    g_warning ("Failed to get netreg proxy properties for %s: %s",
      self->object_path, err->message);
    g_object_unref (self);
    return;
  }

  g_variant_iter_init (&i, properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, &value, NULL)) {
    phosh_wwan_ofono_dbus_netreg_update_prop(self->proxy_netreg, property, value, self);
    g_clear_pointer (&value, g_variant_unref);
  }

  g_object_unref (self);
}


static void
phosh_wwan_ofono_on_proxy_netreg_new_for_bus_finish (GObject        *source_object,
                                                     GAsyncResult   *res,
                                                     PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;

  self->proxy_netreg = phosh_ofono_dbus_network_registration_proxy_new_for_bus_finish (
    res,
    &err);

  if (!self->proxy_netreg) {
    g_warning ("Failed to get netreg proxy for %s: %s",
      self->object_path, err->message);
    g_object_unref (self);
    return;
  }

  phosh_ofono_dbus_network_registration_call_get_properties (
    self->proxy_netreg,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_netreg_get_properties_finish,
    self);

  self->proxy_netreg_props_signal_id = g_signal_connect (self->proxy_netreg,
                                                         "property-changed",
                                                         G_CALLBACK (phosh_wwan_ofono_dbus_netreg_prop_changed_cb),
                                                         self);
}


static void
phosh_wwan_ofono_init_modem (PhoshWWanOfono *self, const char *object_path)
{
  g_return_if_fail (object_path);

  self->object_path = g_strdup (object_path);
  self->locked = FALSE;

  phosh_ofono_dbus_network_registration_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    BUS_NAME,
    object_path,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_proxy_netreg_new_for_bus_finish,
    g_object_ref (self));

  phosh_ofono_dbus_sim_manager_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    BUS_NAME,
    object_path,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_proxy_sim_new_for_bus_finish,
    g_object_ref (self));

  phosh_wwan_ofono_update_present (self, TRUE);
}


static void
phosh_wwan_ofono_modem_added_cb (PhoshWWanOfono        *self,
                                 const char            *modem_object_path,
                                 GVariant              *modem_properties,
                                 PhoshOfonoDBusManager *proxy_manager)
{
  g_debug ("Modem added at path: %s", modem_object_path);
  if (self->object_path == NULL) {
    g_debug ("Tracking modem at: %s", modem_object_path);
    phosh_wwan_ofono_init_modem (self, modem_object_path);
  }
}


static void
phosh_wwan_ofono_modem_removed_cb (PhoshWWanOfono        *self,
                                   const char            *modem_object_path,
                                   PhoshOfonoDBusManager *proxy_manager)
{
  g_debug ("Modem removed at path: %s", modem_object_path);
  if (!g_strcmp0 (modem_object_path, self->object_path)) {
    g_debug ("Dropping modem at: %s", modem_object_path);
    phosh_wwan_ofono_destroy_modem (self);
  }
}


static void
phosh_wwan_ofono_on_get_modems_finish (GObject        *source_object,
                                       GAsyncResult   *res,
                                       PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GVariant) modems = NULL;
  GVariantIter i;
  const char *modem_object_path = NULL;

  if (!phosh_ofono_dbus_manager_call_get_modems_finish (
    self->proxy_manager,
    &modems,
    res,
    &err)) {
      g_warning ("GetModems call failed: %s", err->message);
      return;
  }

  g_variant_iter_init (&i, modems);
  if (g_variant_iter_next (&i, "(&oa{sv})", &modem_object_path, NULL)) {
    /* Look at the first modem */
    g_debug ("modem path: %s", modem_object_path);
    phosh_wwan_ofono_init_modem (self, modem_object_path);
  } else {
    g_debug ("No modem found");
  }
}


static void
phosh_wwan_ofono_on_ofono_manager_created (GObject        *source_object,
                                           GAsyncResult   *res,
                                           PhoshWWanOfono *self)
{
  g_autoptr (GError) err = NULL;

  g_debug ("manager created for %p", source_object);
  self->proxy_manager = phosh_ofono_dbus_manager_proxy_new_for_bus_finish (
    res,
    &err);

  if (!self->proxy_manager) {
    g_warning ("Failed to connect to ofono: %s", err->message);
    return;
  }

  self->manager_object_added_signal_id =
    g_signal_connect_swapped (self->proxy_manager,
                              "modem-added",
                              G_CALLBACK (phosh_wwan_ofono_modem_added_cb),
                              self);

  self->manager_object_removed_signal_id =
    g_signal_connect_swapped (self->proxy_manager,
                              "modem-removed",
                              G_CALLBACK (phosh_wwan_ofono_modem_removed_cb),
                              self);

  phosh_ofono_dbus_manager_call_get_modems (
    self->proxy_manager,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_get_modems_finish,
    self);
}


static void
phosh_wwan_ofono_constructed (GObject *object)
{
  PhoshWWanOfono *self = PHOSH_WWAN_OFONO (object);

  G_OBJECT_CLASS (phosh_wwan_ofono_parent_class)->constructed (object);

  phosh_ofono_dbus_manager_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    BUS_NAME,
    OBJECT_PATH,
    NULL,
    (GAsyncReadyCallback)phosh_wwan_ofono_on_ofono_manager_created,
    self);
}


static void
phosh_wwan_ofono_dispose (GObject *object)
{
  PhoshWWanOfono *self = PHOSH_WWAN_OFONO (object);
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_wwan_ofono_parent_class);

  phosh_wwan_ofono_destroy_modem (self);
  if (self->proxy_manager) {
    g_clear_signal_handler (&self->manager_object_added_signal_id,
                            self->proxy_manager);
    g_clear_signal_handler (&self->manager_object_removed_signal_id,
                            self->proxy_manager);

    g_clear_object (&self->proxy_manager);
  }
  g_clear_pointer (&self->object_path, g_free);

  parent_class->dispose (object);
}


static void
phosh_wwan_ofono_class_init (PhoshWWanOfonoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_wwan_ofono_constructed;
  object_class->dispose = phosh_wwan_ofono_dispose;
  object_class->set_property = phosh_wwan_ofono_set_property;
  object_class->get_property = phosh_wwan_ofono_get_property;

  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_SIGNAL_QUALITY,
                                    "signal-quality");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_ACCESS_TEC,
                                    "access-tec");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_UNLOCKED,
                                    "unlocked");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_SIM,
                                    "sim");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_PRESENT,
                                    "present");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_ENABLED,
                                    "enabled");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_OFONO_PROP_OPERATOR,
                                    "operator");
}


static guint
phosh_wwan_ofono_get_signal_quality (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), 0);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return self->signal_quality;
}


static const char *
phosh_wwan_ofono_get_access_tec (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), NULL);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return self->access_tec;
}


static gboolean
phosh_wwan_ofono_is_unlocked (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), FALSE);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return !self->locked;
}


static gboolean
phosh_wwan_ofono_has_sim (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), FALSE);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return self->sim;
}


static gboolean
phosh_wwan_ofono_is_present (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), FALSE);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return self->present;
}


static gboolean
phosh_wwan_ofono_is_enabled (PhoshWWan *phosh_wwan)
{
  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), FALSE);

  return TRUE;
}


static const char *
phosh_wwan_ofono_get_operator (PhoshWWan *phosh_wwan)
{
  PhoshWWanOfono *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_OFONO (phosh_wwan), NULL);

  self = PHOSH_WWAN_OFONO (phosh_wwan);

  return self->operator;
}


static void
phosh_wwan_ofono_interface_init (PhoshWWanInterface *iface)
{
  iface->get_signal_quality = phosh_wwan_ofono_get_signal_quality;
  iface->get_access_tec = phosh_wwan_ofono_get_access_tec;
  iface->is_unlocked = phosh_wwan_ofono_is_unlocked;
  iface->has_sim = phosh_wwan_ofono_has_sim;
  iface->is_present = phosh_wwan_ofono_is_present;
  iface->is_enabled = phosh_wwan_ofono_is_enabled;
  iface->get_operator = phosh_wwan_ofono_get_operator;
}


static void
phosh_wwan_ofono_init (PhoshWWanOfono *self)
{
}


PhoshWWanOfono *
phosh_wwan_ofono_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_OFONO, NULL);
}
