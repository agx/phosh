/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* WIFI Manager */

#define G_LOG_DOMAIN "phosh-wifimanager"

#include "config.h"

#include "wifimanager.h"

#include <NetworkManager.h>

/**
 * SECTION:phosh-wifi-manager
 * @short_description: Tracks the Wifi status
 * @Title: PhoshWifiManager
 */

enum {
  PHOSH_WIFI_MANAGER_PROP_0,
  PHOSH_WIFI_MANAGER_PROP_ICON_NAME,
  PHOSH_WIFI_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_WIFI_MANAGER_PROP_LAST_PROP];

struct _PhoshWifiManager
{
  GObject parent;

  /* Is wifi radio on (rfkill off) */
  gboolean           enabled;
  /* Whether we have a wifi device at all (independent from the
   * connection state */
  gboolean           have_wifi_dev;

  gchar              *icon_name;

  NMClient           *nmclient;
  /* The access point we're connected to */
  NMAccessPoint      *ap;
  /* The active connection (if it has a wifi device */
  NMActiveConnection *active;
  /* The wifi device used in the active connection */
  NMDeviceWifi       *dev;

};
G_DEFINE_TYPE (PhoshWifiManager, phosh_wifi_manager, G_TYPE_OBJECT);


static const char *
signal_strength_descriptive(guint strength)
{
  if (strength > 80)
    return "excellent";
  else if (strength > 55)
    return "good";
  else if (strength > 30)
    return "ok";
  else if (strength > 5)
    return "weak";
  else
    return "none";
}


static void
phosh_wifi_manager_set_property (GObject *object,
                                 guint property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}


static void
phosh_wifi_manager_get_property (GObject *object,
                                 guint property_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER (object);

  switch (property_id) {
  case PHOSH_WIFI_MANAGER_PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_nm_access_point_strength_changed (PhoshWifiManager *self, GParamSpec *pspec, NMAccessPoint *ap)
{
  guint8 strength;
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_ACCESS_POINT (ap));

  strength = phosh_wifi_manager_get_strength (self);
  g_debug ("Strength changed: %d", strength);
  self->icon_name = g_strdup_printf("network-wireless-signal-%s-symbolic",
                                    signal_strength_descriptive (strength));
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_WIFI_MANAGER_PROP_ICON_NAME]);
}


static void
on_nm_device_wifi_active_access_point_changed (PhoshWifiManager *self, GParamSpec *pspec, NMDeviceWifi *dev)
{
  NMAccessPoint *old_ap;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_DEVICE_WIFI (dev));

  old_ap = self->ap;

  self->ap = nm_device_wifi_get_active_access_point (self->dev);
  if (old_ap == self->ap)
    return;

  if (self->ap)
    g_debug("Wifi access point changed to '%s'", nm_access_point_get_bssid (self->ap));
  else
    g_debug("Wifi access point changed");

  if (self->ap)
    g_object_ref (self->ap);
  if (old_ap) {
    g_signal_handlers_disconnect_by_data (old_ap, self);
    g_object_unref (old_ap);
  }

  if(self->ap) {
    g_signal_connect_swapped (self->ap, "notify::strength",
                              G_CALLBACK (on_nm_access_point_strength_changed), self);
    on_nm_access_point_strength_changed (self, NULL, self->ap);
  }
}


static void
on_nm_active_connection_state_changed (PhoshWifiManager *self,
                                       NMActiveConnectionState state,
                                       NMActiveConnectionStateReason reason,
                                       NMActiveConnection *active)
{
   g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
   g_return_if_fail (NM_IS_ACTIVE_CONNECTION (active));

   g_debug("Active connection state changed %d", state);
   g_clear_pointer (&self->icon_name, g_free);
   switch (state) {
   case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
     self->icon_name = g_strdup("network-wireless-acquiring-symbolic");
     break;
   case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
     self->icon_name = g_strdup("network-wireless-connected-symbolic");
     break;
   case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
   case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
   case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
   default:
     self->icon_name = g_strdup("network-wireless-offline-symbolic");
     break;
   }
   g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_WIFI_MANAGER_PROP_ICON_NAME]);
}


static void
on_nmclient_wireless_enabled_changed (PhoshWifiManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  enabled = nm_client_wireless_get_enabled (nmclient);
  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_debug ("Wifi %sabled", self->enabled ? "en" : "dis");

  g_clear_pointer (&self->icon_name, g_free);
  if (self->enabled && self->have_wifi_dev)
    self->icon_name = g_strdup("network-wireless-offline-symbolic");

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_WIFI_MANAGER_PROP_ICON_NAME]);
}


/*
 * Active connections changed
 *
 * Look if we have a connection using a wifi device and listen
 * for changes on that connection.
 */
static void
on_nmclient_active_connections_changed (PhoshWifiManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  const GPtrArray *conns, *devs;
  NMActiveConnection *conn;
  NMDevice *dev;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  conns = nm_client_get_active_connections(nmclient);

  for (int i = 0; i < conns->len; i++) {
    conn = g_ptr_array_index (conns, i);
    devs = nm_active_connection_get_devices (conn);
    if (!devs || !devs->len) {
      g_warning("Found active connection but no device");
      continue;
    }

    dev = g_ptr_array_index (devs, 0);
    if (NM_IS_DEVICE_WIFI (dev)) {
      g_debug("conn %d uses a wifi device", i);
      found = TRUE;

      /* active connection changed but wifi device is still the same */
      if (self->dev && dev == NM_DEVICE (self->dev))
        break;

      /* otherwise update the device information */
      if (self->dev) {
        g_signal_handlers_disconnect_by_data (self->dev, self);
        g_object_unref (self->dev);
      }
      self->dev = g_object_ref(NM_DEVICE_WIFI (dev));
      g_signal_connect_swapped (self->dev, "notify::active-access-point",
                                G_CALLBACK (on_nm_device_wifi_active_access_point_changed), self);
      on_nm_device_wifi_active_access_point_changed (self, NULL, self->dev);

      /* Is this still the same connection? */
      if (self->active && conn != self->active) {
        g_signal_handlers_disconnect_by_data (self->active, self);
        g_object_unref (self->active);
        self->active = g_object_ref (conn);
        g_signal_connect_swapped (self->active, "state-changed",
                                  G_CALLBACK (on_nm_active_connection_state_changed), self);
      }
      break;
    }
  }

  if (!found && self->dev) {
    if (self->ap) {
      g_signal_handlers_disconnect_by_data (self->ap, self);
      g_clear_object (&self->ap);
    }
    g_signal_handlers_disconnect_by_data (self->dev, self);
    g_clear_object (&self->dev);
  }
}


static void
on_nmclient_devices_changed (PhoshWifiManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  gboolean have_wifi_dev = FALSE;
  const GPtrArray *devs;
  NMDevice *dev;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  devs = nm_client_get_devices (nmclient);

  if (!devs || !devs->len)
    return;

  for (int i = 0; i < devs->len; i++) {
    dev = g_ptr_array_index (devs, i);
    if (NM_IS_DEVICE_WIFI (dev)) {
      g_debug("Wifi device connected at %d", i);
      have_wifi_dev = TRUE;
      break;
    }
  }

  if (have_wifi_dev == self->have_wifi_dev)
    return;

  self->have_wifi_dev = have_wifi_dev;
  g_clear_pointer (&self->icon_name, g_free);
  if (self->enabled && self->have_wifi_dev) {
    self->icon_name = g_strdup("network-wireless-offline-symbolic");
  }
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_WIFI_MANAGER_PROP_ICON_NAME]);
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer data)
{
  g_autoptr(GError) err = NULL;
  PhoshWifiManager *self;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (data));
  self = PHOSH_WIFI_MANAGER (data);

  self->nmclient = nm_client_new_finish (res, &err);
  if (err) {
    g_warning ("Failed to init NM: %s", err->message);
    return;
  }

  g_return_if_fail (NM_IS_CLIENT (self->nmclient));

  g_signal_connect_swapped (self->nmclient, "notify::wireless-enabled",
                            G_CALLBACK (on_nmclient_wireless_enabled_changed), self);
  g_signal_connect_swapped (self->nmclient, "notify::active-connections",
                            G_CALLBACK (on_nmclient_active_connections_changed), self);
  g_signal_connect_swapped (self->nmclient, "notify::devices",
                            G_CALLBACK (on_nmclient_devices_changed), self);

  on_nmclient_wireless_enabled_changed (self, NULL, self->nmclient);
  on_nmclient_active_connections_changed (self, NULL, self->nmclient);
  on_nmclient_devices_changed (self, NULL, self->nmclient);

  g_debug("Wifi manager initialized");
}


static void
phosh_wifi_manager_constructed (GObject *object)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER (object);

  nm_client_new_async (NULL, on_nm_client_ready, self);

  G_OBJECT_CLASS (phosh_wifi_manager_parent_class)->constructed (object);
}


static void
phosh_wifi_manager_finalize (GObject *object)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER(object);

  g_clear_object (&self->nmclient);

  if (self->ap) {
    g_signal_handlers_disconnect_by_data (self->ap, self);
    g_clear_object (&self->ap);
  }

  if (self->dev) {
    g_signal_handlers_disconnect_by_data (self->dev, self);
    g_clear_object (&self->dev);
  }

  if (self->active) {
    g_signal_handlers_disconnect_by_data (self->active, self);
    g_clear_object (&self->active);
  }

  G_OBJECT_CLASS (phosh_wifi_manager_parent_class)->finalize (object);
}


static void
phosh_wifi_manager_class_init (PhoshWifiManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_wifi_manager_constructed;
  object_class->finalize = phosh_wifi_manager_finalize;

  object_class->set_property = phosh_wifi_manager_set_property;
  object_class->get_property = phosh_wifi_manager_get_property;

  props[PHOSH_WIFI_MANAGER_PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The wifi icon name",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PHOSH_WIFI_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_wifi_manager_init (PhoshWifiManager *self)
{
}


PhoshWifiManager *
phosh_wifi_manager_new (void)
{
  return PHOSH_WIFI_MANAGER (g_object_new (PHOSH_TYPE_WIFI_MANAGER, NULL));
}


guint8
phosh_wifi_manager_get_strength (PhoshWifiManager *self)
{
  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (self), 0);

  if (!self->dev || !self->ap)
    return 0;

  return nm_access_point_get_strength (self->ap);
}
