/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-wwan-manager"

#include "wwan-manager.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * SECTION:wwan-manager
 * @short_description: Base class for #PhoshWWan interface implementations
 * @Title: PhoshWWanManager
 *
 * Common code for implementors of the #PhoshWWan interface covering
 * NetworkManager related bits.
 */

typedef struct _PhoshWWanManagerPrivate PhoshWWanManagerPrivate;
struct _PhoshWWanManagerPrivate
{
  NMClient     *nmclient;
  GCancellable *cancel;
};
G_DEFINE_TYPE_WITH_PRIVATE (PhoshWWanManager, phosh_wwan_manager, G_TYPE_OBJECT);

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

  g_return_if_fail (NM_IS_CLIENT (priv->nmclient));
  g_debug("WWan manager initialized");
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

  if (priv->nmclient) {
    g_signal_handlers_disconnect_by_data (priv->nmclient, self);
    g_clear_object (&priv->nmclient);
  }

  G_OBJECT_CLASS (phosh_wwan_manager_parent_class)->dispose (object);
}

static void
phosh_wwan_manager_class_init (PhoshWWanManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->constructed = phosh_wwan_manager_constructed;
  object_class->dispose = phosh_wwan_manager_dispose;
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

  nm_client_dbus_set_property (
    priv->nmclient, NM_DBUS_PATH, NM_DBUS_INTERFACE,
    "WwanEnabled", g_variant_new_boolean (enabled),
    DEFAULT_TIMEOUT_MSEC, NULL, NULL, NULL
  );
}
