/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-wwan-manager"

#include "phosh-wwan-iface.h"
#include "wwan-manager.h"
#include "util.h"

#include <NetworkManager.h>

enum {
  PROP_0,
  PROP_DATA_ENABLED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhoshWWanManager:
 *
 * Base class for #PhoshWWan interface implementations
 *
 * Common code for implementors of the #PhoshWWan interface covering
 * NetworkManager related bits.
 */
typedef struct _PhoshWWanManagerPrivate {
  NMClient           *nmclient;
  NMActiveConnection *active;
  GCancellable       *cancel;
} PhoshWWanManagerPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (PhoshWWanManager, phosh_wwan_manager, G_TYPE_OBJECT);


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

  if (g_strcmp0 (type, NM_SETTING_GSM_SETTING_NAME) == 0)
    return TRUE;

  if (g_strcmp0 (type, NM_SETTING_CDMA_SETTING_NAME) == 0)
    return TRUE;

  return FALSE;
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


gboolean
phosh_wwan_manager_get_data_enabled (PhoshWWanManager *self)
{
  PhoshWWanManagerPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MANAGER (self), FALSE);
  priv = phosh_wwan_manager_get_instance_private (self);

  return !!priv->active;
}
