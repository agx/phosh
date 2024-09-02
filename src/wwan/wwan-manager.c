/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwan-manager"

#include "phosh-config.h"

#include "phosh-wwan-iface.h"
#include "wwan-manager.h"
#include "util.h"

#include <NetworkManager.h>

#define is_type_wwan_connection(s)                                      \
  (g_strcmp0 ((s), NM_SETTING_GSM_SETTING_NAME) == 0 ||                 \
   g_strcmp0 ((s), NM_SETTING_CDMA_SETTING_NAME) == 0)

enum {
  PROP_0,
  PROP_DATA_ENABLED,
  PROP_HAS_DATA,
  PROP_LAST_CONNECTION,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhoshWWanManager:
 *
 * Base class for `PhoshWWan` interface implementations
 *
 * Common code for implementors of the #PhoshWWan interface covering
 * NetworkManager related bits for the mobile data connection.
 */
typedef struct _PhoshWWanManagerPrivate {
  NMClient           *nmclient;
  NMActiveConnection *active;
  GCancellable       *cancel;

  gboolean            has_data;
  char               *last_uuid;
} PhoshWWanManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshWWanManager, phosh_wwan_manager, G_TYPE_OBJECT);


static void
on_wwan_connection_activated (GObject      *object,
                              GAsyncResult *result,
                              gpointer      data)
{
  NMClient *client = NM_CLIENT (object);
  g_autoptr (GError) err = NULL;
  NMActiveConnection *conn;

  conn = nm_client_activate_connection_finish (client, result, &err);
  if (conn != NULL)
    g_debug ("Connecting to cellular network using %s", nm_active_connection_get_uuid (conn));
  else
    g_warning ("Failed to connect to cellular network: %s", err->message);
}


static void
phosh_wwan_manager_activate_last_connection (PhoshWWanManager *self)
{
  PhoshWWanManagerPrivate *priv = phosh_wwan_manager_get_instance_private (self);
  NMConnection *conn;
  const char *name;

  g_return_if_fail (priv->last_uuid);

  conn = NM_CONNECTION (nm_client_get_connection_by_uuid (priv->nmclient, priv->last_uuid));
  g_return_if_fail (NM_IS_CONNECTION (conn));

  name = nm_connection_get_id (conn);
  g_debug ("Activating connection %s", name);
  nm_client_activate_connection_async (priv->nmclient, conn, NULL, NULL,
                                       priv->cancel,
                                       on_wwan_connection_activated,
                                       NULL);
}


static void
on_data_connection_deactivated (GObject      *object,
                                GAsyncResult *result,
                                gpointer      data)
{
  NMClient *client = NM_CLIENT (object);
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = nm_client_deactivate_connection_finish (client, result, &err);
  if (success)
    g_debug ("Disconnected from cellular network");
  else
    g_warning ("Failed to disconnect from cellular network: %s", err->message);
}


static void
phosh_wwan_manager_deactivate_all_connections (PhoshWWanManager *self)
{
  PhoshWWanManagerPrivate *priv = phosh_wwan_manager_get_instance_private (self);
  const GPtrArray *conns;

  conns = nm_client_get_active_connections (priv->nmclient);
  /* Disconnect all mobile data connections */
  for (int i = 0; i < conns->len; i++) {
    NMActiveConnection *conn = NM_ACTIVE_CONNECTION (g_ptr_array_index (conns, i));
    const char *type;

    type = nm_active_connection_get_connection_type (conn);
    if (!is_type_wwan_connection (type))
      continue;

    nm_client_deactivate_connection_async (priv->nmclient,
                                           conn,
                                           priv->cancel,
                                           on_data_connection_deactivated,
                                           NULL);
  }
}


static void
phosh_wwan_manager_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshWWanManager *self = PHOSH_WWAN_MANAGER (object);
  PhoshWWanManagerPrivate *priv = phosh_wwan_manager_get_instance_private (self);

  switch (property_id) {
  case PROP_DATA_ENABLED:
    g_value_set_boolean (value, !!priv->active);
    break;
  case PROP_HAS_DATA:
    g_value_set_boolean (value, priv->has_data);
    break;
  case PROP_LAST_CONNECTION:
    g_value_set_string (value, priv->last_uuid);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
is_wwan_connection (NMActiveConnection *conn)
{
  const char *type;

  type = nm_active_connection_get_connection_type (conn);

  return is_type_wwan_connection (type);
}


static void
update_connections (PhoshWWanManager *self)
{
  const GPtrArray *conns;
  gint last_ts = 0;
  g_autofree char *old_uuid = NULL;
  gboolean old_has_data;
  PhoshWWanManagerPrivate *priv;

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));
  priv = phosh_wwan_manager_get_instance_private (self);

  old_has_data = priv->has_data;
  old_uuid = g_strdup (priv->last_uuid);
  /* No need to bother as long as we have an active connection */
  if (priv->active) {
    priv->has_data = TRUE;
    goto out;
  }

  conns = nm_client_get_connections (priv->nmclient);

  g_clear_pointer (&priv->last_uuid, g_free);
  for (int i = 0; i < conns->len; i++) {
    NMConnection *conn = NM_CONNECTION (g_ptr_array_index (conns, i));
    NMSettingConnection *s_con = nm_connection_get_setting_connection (conn);
    const char *type;
    gint64 ts;

    type = nm_connection_get_connection_type (conn);
    if (!is_type_wwan_connection (type))
      continue;

    ts = nm_setting_connection_get_timestamp (s_con);
    if (!last_ts || ts > last_ts) {
      last_ts = ts;

      g_free (priv->last_uuid);
      priv->last_uuid = g_strdup (nm_setting_connection_get_uuid (s_con));
    }
  }

  priv->has_data = !!priv->last_uuid;

 out:
  g_debug ("WWAN data connection present: %d, uuid: %s", priv->has_data, priv->last_uuid);

  if (priv->has_data != old_has_data)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HAS_DATA]);

  if (g_strcmp0 (priv->last_uuid, old_uuid))
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LAST_CONNECTION]);
}

/*
 * Connections changed
 *
 * Check if we have a configured WWAN connection
 */
static void
on_nmclient_connections_changed (PhoshWWanManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  update_connections (self);
}

/*
 * Active connections changed
 *
 * Look if we have a connection using a wwan device and listen
 * for changes on that connection.
 */
static void
on_nmclient_active_connections_changed (PhoshWWanManager *self,
                                        GParamSpec       *pspec,
                                        NMClient         *nmclient)
{
  const GPtrArray *conns;
  NMActiveConnection *old_conn;
  PhoshWWanManagerPrivate *priv;

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));
  priv = phosh_wwan_manager_get_instance_private (self);

  old_conn = priv->active;
  conns = nm_client_get_active_connections (nmclient);

  g_clear_object (&priv->active);
  for (int i = 0; i < conns->len; i++) {
    NMActiveConnection *conn = g_ptr_array_index (conns, i);

    /* We only care about wwan connections */
    if (!is_wwan_connection (conn))
      continue;

    g_set_object (&priv->active, conn);
    /* We pick one connection here and select another one once this one goes away. */
    break;
  }

  g_debug ("Mobile data connection: %d", !!priv->active);
  if (!!old_conn != !!priv->active)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DATA_ENABLED]);

  if (!priv->active)
    update_connections (self);
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) err = NULL;
  PhoshWWanManager *self;
  PhoshWWanManagerPrivate *priv;
  NMClient *nmclient;

  nmclient = nm_client_new_finish (res, &err);
  if (nmclient == NULL) {
    phosh_async_error_warn (err, "Failed to init NM");
    return;
  }

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (data));
  self = PHOSH_WWAN_MANAGER (data);
  priv = phosh_wwan_manager_get_instance_private (self);
  priv->nmclient = nmclient;

  g_signal_connect_swapped (priv->nmclient, "notify::active-connections",
                            G_CALLBACK (on_nmclient_active_connections_changed), self);
  on_nmclient_active_connections_changed (self, NULL, priv->nmclient);

  g_signal_connect_swapped (priv->nmclient, "notify::connections",
                            G_CALLBACK (on_nmclient_connections_changed), self);
  on_nmclient_connections_changed (self, NULL, priv->nmclient);

  g_debug ("WWan manager initialized");
}


static void
phosh_wwan_manager_constructed (GObject *object)
{
  PhoshWWanManager *self = PHOSH_WWAN_MANAGER (object);
  PhoshWWanManagerPrivate *priv = phosh_wwan_manager_get_instance_private (self);

  G_OBJECT_CLASS (phosh_wwan_manager_parent_class)->constructed (object);

  priv->cancel = g_cancellable_new ();
  nm_client_new_async (priv->cancel, on_nm_client_ready, self);
}

static void
phosh_wwan_manager_dispose (GObject *object)
{
  PhoshWWanManager *self = PHOSH_WWAN_MANAGER (object);
  PhoshWWanManagerPrivate *priv = phosh_wwan_manager_get_instance_private (self);

  g_cancellable_cancel (priv->cancel);
  g_clear_object (&priv->cancel);

  g_clear_object (&priv->active);

  if (priv->nmclient) {
    g_signal_handlers_disconnect_by_data (priv->nmclient, self);
    g_clear_object (&priv->nmclient);
  }

  g_clear_pointer (&priv->last_uuid, g_free);

  G_OBJECT_CLASS (phosh_wwan_manager_parent_class)->dispose (object);
}

static void
phosh_wwan_manager_class_init (PhoshWWanManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->constructed = phosh_wwan_manager_constructed;
  object_class->dispose = phosh_wwan_manager_dispose;
  object_class->get_property = phosh_wwan_manager_get_property;

  /**
   * PhoshWwanManager:data-enabled:
   *
   * Whether data is enabled over WWAN.
   */
  props[PROP_DATA_ENABLED] =
    g_param_spec_boolean ("data-enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshWwanManager:has-data:
   *
   * Whether there's a data connection that could be activated.
   */
  props[PROP_HAS_DATA] =
    g_param_spec_boolean ("has-data", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshWWanManager:last-connection:
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
phosh_wwan_manager_init (PhoshWWanManager *self)
{
}

PhoshWWanManager *
phosh_wwan_manager_new (void)
{
  return PHOSH_WWAN_MANAGER (g_object_new (PHOSH_TYPE_WWAN_MANAGER, NULL));
}

void
phosh_wwan_manager_set_enabled (PhoshWWanManager *self, gboolean enabled)
{
  PhoshWWanManagerPrivate *priv;
  int DEFAULT_TIMEOUT_MSEC = -1;

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));
  priv = phosh_wwan_manager_get_instance_private (self);
  g_return_if_fail (NM_IS_CLIENT (priv->nmclient));

  nm_client_dbus_set_property (priv->nmclient, NM_DBUS_PATH, NM_DBUS_INTERFACE,
                               "WwanEnabled", g_variant_new_boolean (enabled),
                               DEFAULT_TIMEOUT_MSEC, NULL, NULL, NULL);
}

/**
 * phosh_wwan_manager_get_data_enabled:
 * @self: The wwan manager
 *
 * Get whether the mobile data connection is enabled.
 *
 * Returns: `TRUE` if a mobile data connection is enabled otherwise `FALSE`.
 */
gboolean
phosh_wwan_manager_get_data_enabled (PhoshWWanManager *self)
{
  PhoshWWanManagerPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MANAGER (self), FALSE);
  priv = phosh_wwan_manager_get_instance_private (self);

  return !!priv->active;
}

/**
 * phosh_wwan_manager_set_data_enabled:
 * @self: The wwan manager
 * @enabled: Whether to enable or disable the mobile data connection
 *
 * Connect to or disconnect from mobile data. When `enabled` is `TRUE`
 * connects the last active mobile data connection.
 */
void
phosh_wwan_manager_set_data_enabled (PhoshWWanManager *self, gboolean enabled)
{
  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));

  if (enabled)
    phosh_wwan_manager_activate_last_connection (self);
  else
    phosh_wwan_manager_deactivate_all_connections (self);
}

/**
 * phosh_wwan_manager_has_data:
 * @self: The wwan manager
 *
 * Gets whether there's a data connection that could possibly be enabled.
 * It doesn't take into account whether the connection is enabled or not.
 * See [method@WWanManager.get_data_enabled].
 *
 * Returns: `TRUE` if there's a activatable data connection.
 */
gboolean
phosh_wwan_manager_has_data (PhoshWWanManager *self)
{
  PhoshWWanManagerPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MANAGER (self), FALSE);
  priv = phosh_wwan_manager_get_instance_private (self);

  return priv->has_data;
}
