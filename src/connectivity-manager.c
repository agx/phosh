/*
 * Copyright (C) 2025 The Phosh Develpoers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-connectivity-manager"

#include "config.h"

#include "connectivity-manager.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * PhoshConnectivityManager
 *
 * `PhoshConnectivityManager` monitors the connectivity to the
 * internet via NetworkManager.
 */

enum {
  PROP_0,
  PROP_CONNECTIVITY,
  PROP_ICON_NAME,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshConnectivityManager {
  PhoshManager  parent;

  gboolean      connectivity;
  NMClient     *nmclient;
  GCancellable *cancel;

  char         *icon_name;
};
G_DEFINE_TYPE (PhoshConnectivityManager, phosh_connectivity_manager, PHOSH_TYPE_MANAGER)


static void
phosh_connectivity_manager_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  PhoshConnectivityManager *self = PHOSH_CONNECTIVITY_MANAGER (object);

  switch (property_id) {
  case PROP_CONNECTIVITY:
    g_value_set_boolean (value, self->connectivity);
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
on_connectivity_changed (PhoshConnectivityManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  const char *icon_name;
  NMConnectivityState state;
  gboolean connectivity = FALSE;

  g_return_if_fail (PHOSH_IS_CONNECTIVITY_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  state = nm_client_get_connectivity (nmclient);

  switch (state) {
  case NM_CONNECTIVITY_NONE:
    icon_name = "network-offline-symbolic";
    break;
  case NM_CONNECTIVITY_PORTAL:
  case NM_CONNECTIVITY_LIMITED:
    icon_name = "network-no-route-symbolic";
    break;
  case NM_CONNECTIVITY_UNKNOWN:
  case NM_CONNECTIVITY_FULL:
  default:
    icon_name = "network-transmit-receive-symbolic";
    connectivity = TRUE;
  }

  g_debug ("Connectivity changed (%d), updating icon to '%s'",
           state, icon_name);

  if (connectivity != self->connectivity) {
    self->connectivity = connectivity;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CONNECTIVITY]);
  }

  if (g_strcmp0 (self->icon_name, icon_name)) {
    g_free (self->icon_name);
    self->icon_name = g_strdup (icon_name);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer user_data)
{
  PhoshConnectivityManager *self = PHOSH_CONNECTIVITY_MANAGER (user_data);
  g_autoptr (GError) err = NULL;
  NMClient *nmclient;

  nmclient = nm_client_new_finish (res, &err);
  if (!nmclient) {
    phosh_async_error_warn (err, "Failed to init NM");
    return;
  }

  g_return_if_fail (PHOSH_IS_CONNECTIVITY_MANAGER (self));
  self->nmclient = nmclient;

  g_return_if_fail (NM_IS_CLIENT (self->nmclient));

  g_signal_connect_swapped (self->nmclient, "notify::connectivity",
                            G_CALLBACK (on_connectivity_changed), self);

  on_connectivity_changed (self, NULL, self->nmclient);
}


static void
phosh_connectivity_manager_idle_init (PhoshManager *manager)
{
  PhoshConnectivityManager *self = PHOSH_CONNECTIVITY_MANAGER (manager);

  nm_client_new_async (self->cancel, on_nm_client_ready, self);
}


static void
phosh_connectivity_manager_finalize (GObject *object)
{
  PhoshConnectivityManager *self = PHOSH_CONNECTIVITY_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->nmclient);

  G_OBJECT_CLASS (phosh_connectivity_manager_parent_class)->finalize (object);
}


static void
phosh_connectivity_manager_class_init (PhoshConnectivityManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->get_property = phosh_connectivity_manager_get_property;
  object_class->finalize = phosh_connectivity_manager_finalize;

  manager_class->idle_init = phosh_connectivity_manager_idle_init;

  /**
   * PhoshConnectivityManager:connectivity:
   *
   * Whether there is a connection to the internet
   */
  props[PROP_CONNECTIVITY] =
    g_param_spec_boolean ("connectivity", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_connectivity_manager_init (PhoshConnectivityManager *self)
{
  self->cancel = g_cancellable_new ();
}


PhoshConnectivityManager *
phosh_connectivity_manager_new (void)
{
  return PHOSH_CONNECTIVITY_MANAGER (g_object_new (PHOSH_TYPE_CONNECTIVITY_MANAGER, NULL));
}
