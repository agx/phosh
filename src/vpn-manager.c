/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#define G_LOG_DOMAIN "phosh-vpn-manager"

#include "phosh-config.h"

#include "vpn-manager.h"
#include "shell.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * PhoshVpnManager:
 *
 * Tracks the Vpn status and handle vpn credentials entry
 *
 * Manages vpn information and state
 */

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_CONNECTION,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshVpnManager {
  GObject             parent;

  gboolean            enabled;
  gboolean            present;

  const char         *icon_name;
  char               *last_uuid;

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
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);

  if (self->enabled != old_enabled)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
phosh_vpn_manager_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshVpnManager *self = PHOSH_VPN_MANAGER (object);

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
  case PROP_LAST_CONNECTION:
    g_value_set_string (value, self->last_uuid);
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


static void
update_connections (PhoshVpnManager *self)
{
  const GPtrArray *conns;
  gint last_ts = 0;
  g_autofree char *old_uuid = NULL;
  gboolean old_present;

  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));

  old_present = self->present;
  old_uuid = g_strdup (self->last_uuid);
  /* No need to bother as long as we have an active connection */
  if (self->active) {
    self->present = TRUE;
    goto out;
  }

  conns = nm_client_get_connections (self->nmclient);

  g_clear_pointer (&self->last_uuid, g_free);
  for (int i = 0; i < conns->len; i++) {
    NMConnection *conn = NM_CONNECTION (g_ptr_array_index (conns, i));
    NMSettingConnection *s_con = nm_connection_get_setting_connection (conn);
    gint64 ts;

    if (!nm_connection_is_type (conn, NM_SETTING_VPN_SETTING_NAME) &&
        !nm_connection_is_type (conn, NM_SETTING_WIREGUARD_SETTING_NAME))
      continue;

    ts = nm_setting_connection_get_timestamp (s_con);
    if (!last_ts || ts > last_ts) {
      last_ts = ts;

      g_free (self->last_uuid);
      self->last_uuid = g_strdup (nm_setting_connection_get_uuid (s_con));
    }
  }

  self->present = !!self->last_uuid;

 out:
  g_debug ("VPN present: %d, uuid: %s", self->present, self->last_uuid);

  if (self->present != old_present)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);

  if (g_strcmp0 (self->last_uuid, old_uuid))
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LAST_CONNECTION]);
}


static void
on_vpn_connection_activated (GObject      *source_object,
                             GAsyncResult *res,
                             gpointer      user_data)
{
  NMActiveConnection *active;
  g_autoptr (GError) err = NULL;

  g_return_if_fail (NM_IS_CLIENT (source_object));

  active = nm_client_activate_connection_finish (NM_CLIENT (source_object), res, &err);
  if (!active)
    g_warning ("Failed to activate connection: %s", err->message);
}


static void
on_vpn_connection_deactivated (GObject      *source_object,
                             GAsyncResult *res,
                             gpointer      user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;

  g_return_if_fail (NM_IS_CLIENT (source_object));

  success = nm_client_deactivate_connection_finish (NM_CLIENT (source_object), res, &err);
  if (!success)
    g_warning ("Failed to deactivate connection: %s", err->message);
}


/*
 * Connections changed
 *
 * Look if we have a configured VPN connection
 */
static void
on_nmclient_connections_changed (PhoshVpnManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  update_connections (self);
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
  NMActiveConnection *conn, *old_conn;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  conns = nm_client_get_active_connections (nmclient);
  old_conn = self->active;

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

  g_object_freeze_notify (G_OBJECT (self));

  if (old_conn != self->active)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LAST_CONNECTION]);

  /* Update the active connection state */
  update_state (self);
  if (!self->active) {
    update_connections (self);
  }

  g_object_thaw_notify (G_OBJECT (self));
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

  g_signal_connect_swapped (self->nmclient, "notify::connections",
                            G_CALLBACK (on_nmclient_connections_changed), self);
  on_nmclient_connections_changed (self, NULL, self->nmclient);

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
  g_clear_pointer (&self->last_uuid, g_free);

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
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshVpnManager:enabled
   *
   * Whether a VPN connection is enabled
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshVpnManager:present:
   *
   * Whether there is at least one VPN connection configured
   */
  props[PROP_PRESENT] =
    g_param_spec_string ("present", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshVpnManager:last-connection:
   *
   * The last activated connection
   */
  props[PROP_LAST_CONNECTION] =
    g_param_spec_string ("last-connection", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
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

gboolean
phosh_vpn_manager_get_present (PhoshVpnManager *self)
{
  g_return_val_if_fail (PHOSH_IS_VPN_MANAGER (self), FALSE);

  return self->present;
}

const char *
phosh_vpn_manager_get_last_connection (PhoshVpnManager *self)
{
  g_return_val_if_fail (PHOSH_IS_VPN_MANAGER (self), NULL);

  if (self->active)
    return nm_active_connection_get_id (self->active);

  if (self->last_uuid) {
    NMConnection *conn;

    conn = NM_CONNECTION (nm_client_get_connection_by_uuid (self->nmclient, self->last_uuid));
    if (conn)
      return nm_connection_get_id (conn);
  }

  return NULL;
}

void
phosh_vpn_manager_toggle_last_connection (PhoshVpnManager *self)
{
  NMConnection *conn;
  const char *name;

  g_return_if_fail (PHOSH_IS_VPN_MANAGER (self));

  if (self->active) {
    nm_client_deactivate_connection_async (self->nmclient,
                                           self->active,
                                           self->cancel,
                                           on_vpn_connection_deactivated,
                                           NULL);
    return;
  }

  if (!self->last_uuid)
    return;

  conn = NM_CONNECTION (nm_client_get_connection_by_uuid (self->nmclient, self->last_uuid));
  g_return_if_fail (NM_IS_CONNECTION (conn));

  name = nm_connection_get_id (conn);
  g_debug ("Activating connection %s", name);
  nm_client_activate_connection_async (self->nmclient, conn, NULL, NULL,
                                       self->cancel,
                                       on_vpn_connection_activated,
                                       NULL);
}
