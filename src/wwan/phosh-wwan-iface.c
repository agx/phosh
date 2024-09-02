/*
 * Copyright (C) 2018-2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwan-iface"

#include "phosh-wwan-iface.h"
#include "wwan-manager.h"

/**
 * PhoshWWan:
 *
 * Implementations of the `PhoshWWan` interface handle modem
 * interaction such as getting mobile network information and signal
 * strength.
 *
 * Since: 0.0.1
 */

G_DEFINE_INTERFACE (PhoshWWan, phosh_wwan, PHOSH_TYPE_WWAN_MANAGER)

void
phosh_wwan_default_init (PhoshWWanInterface *iface)
{
  /**
   * PhoshWWan:signal-quality:
   *
   * The signal quality of the current modem
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_int ("signal-quality", "", "",
                      0, 100, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:access-tec:
   *
   * The access-tec of the current modem
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_string ("access-tec", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:unlocked:
   *
   * Whether the current modem is unlocked
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("unlocked", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:sim:
   *
   * Whether the current modem has a sim card inserted
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("sim", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:present:
   *
   * Whether a modem is prsent
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:enabled:
   *
   * Whether a modem is enabled
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY));
  /**
   * PhoshWWan:operator:
   *
   * The name of the operator of the current modem
   */
  g_object_interface_install_property (
    iface,
    g_param_spec_string ("operator", "", "",
                         NULL,
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


gboolean
phosh_wwan_is_enabled (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), FALSE);

  iface = PHOSH_WWAN_GET_IFACE (self);
  g_return_val_if_fail (iface->is_enabled != NULL, FALSE);
  return iface->is_enabled (self);
}


const char *
phosh_wwan_get_operator (PhoshWWan *self)
{
  PhoshWWanInterface *iface;

  g_return_val_if_fail (PHOSH_IS_WWAN (self), NULL);

  iface = PHOSH_WWAN_GET_IFACE (self);
  return iface->get_operator (self);
}


void
phosh_wwan_set_enabled (PhoshWWan *self, gboolean enabled)
{
  PhoshWWanManager *manager;

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));

  manager = PHOSH_WWAN_MANAGER (self);
  phosh_wwan_manager_set_enabled (manager, enabled);
}

/**
 * phosh_wwan_set_data_enabled:
 * @self: The wwan interface
 * @enabled: Whether to enable or disable the mobile data connection
 *
 * Connect to or disconnect from mobile data.
 */
void
phosh_wwan_set_data_enabled (PhoshWWan *self, gboolean enabled)
{
  PhoshWWanManager *manager;

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));

  manager = PHOSH_WWAN_MANAGER (self);
  phosh_wwan_manager_set_data_enabled (manager, enabled);
}

/**
 * phosh_wwan_has_data:
 * @self: The wwan interface
 *
 * Gets whether there's a data connection that could possibly be enabled.
 * It doesn't take into account whether the connection is enabled or not.
 *
 * Returns: `TRUE` if there's a activatable data connection.
 */
gboolean
phosh_wwan_has_data (PhoshWWan *self)
{
  PhoshWWanManager *manager;

  g_return_val_if_fail (PHOSH_IS_WWAN_MANAGER (self), FALSE);

  manager = PHOSH_WWAN_MANAGER (self);
  return phosh_wwan_manager_has_data (manager);
}
