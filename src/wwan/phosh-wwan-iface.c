/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */
/* Modem abstraction */

#define G_LOG_DOMAIN "phosh-wwan-iface"

#include "phosh-wwan-iface.h"

G_DEFINE_INTERFACE (PhoshWWan, phosh_wwan, G_TYPE_OBJECT)

void
phosh_wwan_default_init (PhoshWWanInterface *iface)
{
  g_object_interface_install_property (
    iface,
    g_param_spec_int ("signal-quality",
                      "Signal quality",
                      "Signal quality in percent",
                      0, 100, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));

  g_object_interface_install_property (
    iface,
    g_param_spec_string ("access-tec",
                         "Access technology",
                         "Network access technology",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));

  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("unlocked",
                          "Modem unlocked",
                          "Modem is unlocked",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));

  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("sim",
                          "Modem sim present",
                          "Modem has a sim card inserted",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));

  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("present",
                          "Modem present",
                          "Whether there is a modem present",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  }


guint
phosh_wwan_get_signal_quality (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), 0);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->get_signal_quality != NULL, 0);
  return iface->get_signal_quality (self);
}


const char*
phosh_wwan_get_access_tec (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), NULL);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->get_access_tec != NULL, NULL);
  return iface->get_access_tec (self);

}

gboolean
phosh_wwan_is_unlocked (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), FALSE);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->is_unlocked != NULL, FALSE);
  return iface->is_unlocked (self);
}

gboolean
phosh_wwan_has_sim (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), FALSE);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->has_sim != NULL, FALSE);
  return iface->has_sim (self);
}

gboolean
phosh_wwan_is_present (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), FALSE);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->is_present != NULL, FALSE);
  return iface->is_present (self);
}
