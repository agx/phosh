/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#define G_LOG_DOMAIN "phosh-vpn-manager"

#include "config.h"

#include "vpn-manager.h"
#include "shell.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * SECTION:vpn-manager
 * @short_description: Tracks the Vpn status and handle vpn credentials entry
 * @Title: PhoshVpnManager
 *
 * Manages vpn information and state
 */

enum {
  PHOSH_VPN_MANAGER_PROP_0,
  PHOSH_VPN_MANAGER_PROP_ICON_NAME,
  PHOSH_VPN_MANAGER_PROP_ENABLED,
  PHOSH_VPN_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_VPN_MANAGER_PROP_LAST_PROP];

struct _PhoshVpnManager {
  GObject             parent;

  gboolean            enabled;

  const char         *icon_name;

  NMClient           *nmclient;
  GCancellable       *cancel;

  NMActiveConnection *active;
};
G_DEFINE_TYPE (PhoshVpnManager, phosh_vpn_manager, G_TYPE_OBJECT);


static void
update_state (PhoshVpnManager *self)
{
  NMActiveConnectionState state;
  const char *old_icon_name;
  gboolean old_enabled, is_vpn, is_wg;
  const char *type;

  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));

  old_icon_name = self->icon_name;
  old_enabled = self->enabled;

  if (!self->active) {
    self->icon_name = "network-vpn-disabled-symbolic";
    self->enabled = FALSE;
    goto out;
  }

  type = nm_active_connection_get_connection_type (self->active);
  is_vpn = nm_active_connection_get_vpn (self->active);
  is_wg = !g_strcmp0 (type, NM_SETTING_WIREGUARD_SETTING_NAME);

  g_return_if_fail (is_wg || is_vpn);

  state = nm_active_connection_get_state (self->active);

  switch (state) {
  case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
    self->icon_name = "network-vpn-acquiring-symbolic";
    self->enabled = TRUE;
    break;
  case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
    self->icon_name = "network-vpn-symbolic";
    self->enabled = TRUE;
    break;
  case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
  case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
  case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
  default:
    self->icon_name = "network-vpn-disabled-symbolic";
    self->enabled = FALSE;
  }

out:
  g_debug ("Enabled: %d, icon: %s", self->enabled, self->icon_name);

  if (g_strcmp0 (self->icon_name, old_icon_name) != 0)
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_VPN_MANAGER_PROP_ICON_NAME]);

  if (self->enabled != old_enabled)
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_VPN_MANAGER_PROP_ENABLED]);
}


static void
phosh_vpn_manager_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshVpnManager *self = PHOSH_VPN_MANAGER (object);

  switch (property_id) {
  case PHOSH_VPN_MANAGER_PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PHOSH_VPN_MANAGER_PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_nm_active_connection_vpn_state_changed (PhoshVpnManager              *self,
                                           NMActiveConnectionState       state,
                                           NMActiveConnectionStateReason reason,
                                           NMActiveConnection           *active)
{
  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));
  g_return_if_fail (NM_IS_ACTIVE_CONNECTION (active));

  g_debug ("Active VPN connection '%s' state changed: %d", nm_active_connection_get_id (active), state);

  update_state (self);
}


static void
on_nm_active_wg_connection_state_changed (PhoshVpnManager              *self,
                                          NMActiveConnectionState       state,
                                          NMActiveConnectionStateReason reason,
                                          NMActiveConnection           *active)
{
  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));
  g_return_if_fail (NM_IS_ACTIVE_CONNECTION (active));

  g_debug ("Active wireguard connection '%s' state changed: %d", nm_active_connection_get_id (active), state);

  update_state (self);
}


/*
 * Active connections changed
 *
 * Look if we have a connection using a vpn device and listen
 * for changes on that connection.
 */
static void
on_nmclient_active_connections_changed (PhoshVpnManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  const GPtrArray *conns;
  NMActiveConnection *conn;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  conns = nm_client_get_active_connections (nmclient);

  for (int i = 0; i < conns->len; i++) {
    gboolean is_vpn, is_wg;
    const char *type;

    conn = g_ptr_array_index (conns, i);

    type = nm_active_connection_get_connection_type (conn);
    is_vpn = nm_active_connection_get_vpn (conn);
    is_wg = !g_strcmp0 (type, NM_SETTING_WIREGUARD_SETTING_NAME);

    /* We only care about vpn and wireguard connections */
    if (!is_wg && !is_vpn)
        continue;

    found = TRUE;

    /* Is this still the same connection? */
    if (conn == self->active)
      break;

    g_debug ("New active VPN connection %p type '%s'", conn, type);
    if (self->active)
      g_signal_handlers_disconnect_by_data (self->active, self);
    g_set_object (&self->active, conn);

    if (is_vpn) {
      g_signal_connect_swapped (self->active, "vpn-state-changed",
                                G_CALLBACK (on_nm_active_connection_vpn_state_changed), self);
    } else if (is_wg) {
      g_signal_connect_swapped (self->active, "state-changed",
                                G_CALLBACK (on_nm_active_wg_connection_state_changed), self);
    }
    /* We pick one connection here and select another one once this one goes away. */
    /* TODO: can we sensibly infer some kind of priorities by looking at routing */
    break;
  }

  /* Clean up if there's no active vpn connection */
  if (!found) {
    if (self->active)
      g_signal_handlers_disconnect_by_data (self->active, self);
    g_clear_object (&self->active);
  }
  update_state (self);
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) err = NULL;
  PhoshVpnManager *self;
  NMClient *client;

  client = nm_client_new_finish (res, &err);
  if (client == NULL) {
    g_message ("Failed to init NM: %s", err->message);
    return;
  }

  self = PHOSH_VPN_MANAGER (data);
  self->nmclient = client;

  g_signal_connect_swapped (self->nmclient, "notify::active-connections",
                            G_CALLBACK (on_nmclient_active_connections_changed), self);
  on_nmclient_active_connections_changed (self, NULL, self->nmclient);

  g_debug ("Vpn manager initialized");
}


static void
phosh_vpn_manager_constructed (GObject *object)
{
  PhoshVpnManager *self = PHOSH_VPN_MANAGER (object);

  G_OBJECT_CLASS (phosh_vpn_manager_parent_class)->constructed (object);

  self->cancel = g_cancellable_new ();
  nm_client_new_async (self->cancel, on_nm_client_ready, self);
}


static void
phosh_vpn_manager_dispose (GObject *object)
{
  PhoshVpnManager *self = PHOSH_VPN_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  if (self->nmclient) {
    g_signal_handlers_disconnect_by_data (self->nmclient, self);
    g_clear_object (&self->nmclient);
  }

  if (self->active) {
    g_signal_handlers_disconnect_by_data (self->active, self);
    g_clear_object (&self->active);
  }

  G_OBJECT_CLASS (phosh_vpn_manager_parent_class)->dispose (object);
}


static void
phosh_vpn_manager_class_init (PhoshVpnManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_vpn_manager_constructed;
  object_class->dispose = phosh_vpn_manager_dispose;

  object_class->get_property = phosh_vpn_manager_get_property;

  /**
   * PhoshVpnManager:icon-name:
   *
   * The icon name to represent the current VPN status
   */
  props[PHOSH_VPN_MANAGER_PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshVpnManager:enabled
   *
   * Whether a VPN connection is enabled
   */
  props[PHOSH_VPN_MANAGER_PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PHOSH_VPN_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_vpn_manager_init (PhoshVpnManager *self)
{
  self->icon_name = "network-vpn-disabled-symbolic";
}


PhoshVpnManager *
phosh_vpn_manager_new (void)
{
  return PHOSH_VPN_MANAGER (g_object_new (PHOSH_TYPE_VPN_MANAGER, NULL));
}


const char *
phosh_vpn_manager_get_icon_name (PhoshVpnManager *self)
{
  g_return_val_if_fail (PHOSH_IS_VPN_MANAGER (self), NULL);

  return self->icon_name;
}


gboolean
phosh_vpn_manager_get_enabled (PhoshVpnManager *self)
{
  g_return_val_if_fail (PHOSH_IS_VPN_MANAGER (self), FALSE);

  return self->enabled;
}
