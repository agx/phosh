/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#define G_LOG_DOMAIN "phosh-wifimanager"

#include "phosh-config.h"

#include "wifimanager.h"
#include "shell.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * PhoshWifiManager:
 *
 * Tracks the Wifi status and handle wifi credentials entry
 *
 * Manages wifi information and state
 */

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_SSID,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshWifiManager
{
  GObject parent;

  /* Is wifi radio on (rfkill off) */
  gboolean           enabled;
  /* Whether we have a wifi device at all (independent from the
   * connection state */
  gboolean           present;

  const char         *icon_name;
  char               *ssid;

  NMClient           *nmclient;
  GCancellable       *cancel;

  /* The access point we're connected to */
  NMAccessPoint      *ap;
  /* The active connection (if it has a wifi device */
  NMActiveConnection *active;
  /* The wifi device used in the active connection */
  NMDeviceWifi       *dev;
};
G_DEFINE_TYPE (PhoshWifiManager, phosh_wifi_manager, G_TYPE_OBJECT);


static const char *
signal_strength_icon_name (guint strength)
{
  if (strength > 80)
    return "network-wireless-signal-excellent-symbolic";
  else if (strength > 55)
    return "network-wireless-signal-good-symbolic";
  else if (strength > 30)
    return "network-wireless-signal-ok-symbolic";
  else if (strength > 5)
    return "network-wireless-signal-weak-symbolic";
  else
    return "network-wireless-signal-none-symbolic";
}


static gboolean
get_is_hotspot_master (PhoshWifiManager *self)
{
  NMSettingIPConfig *ip4_setting;
  NMConnection *c;

  if (!self->dev || !self->active ||
      nm_active_connection_get_state (self->active) != NM_ACTIVE_CONNECTION_STATE_ACTIVATED)
    return FALSE;

  c = NM_CONNECTION (nm_active_connection_get_connection (self->active));
  ip4_setting = nm_connection_get_setting_ip4_config (c);

  if (ip4_setting &&
      g_strcmp0 (nm_setting_ip_config_get_method (ip4_setting),
                 NM_SETTING_IP4_CONFIG_METHOD_SHARED) == 0)
    return TRUE;

  return FALSE;
}

static const char *
get_icon_name (PhoshWifiManager *self)
{
  NMActiveConnectionState state;
  guint8 strength;

  if (!self->dev) {
    if (self->enabled && self->present) {
      return "network-wireless-offline-symbolic";
    }
    return "network-wireless-disabled-symbolic";
  }

  state = nm_active_connection_get_state (self->active);

  switch (state) {
  case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
    return "network-wireless-acquiring-symbolic";
  case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
    if (get_is_hotspot_master (self)) {
      return "network-wireless-hotspot-symbolic";
    } else if (!self->ap) {
      return "network-wireless-connected-symbolic";
    } else {
      strength = phosh_wifi_manager_get_strength (self);
      return signal_strength_icon_name (strength);
    }
  case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
  case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
  case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
    return "network-wireless-offline-symbolic";
  default:
    return "network-wireless-disabled-symbolic";
  }
}


static void
update_icon_name (PhoshWifiManager *self)
{
  const char *old_icon_name;
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));

  old_icon_name = self->icon_name;
  self->icon_name = get_icon_name (self);

  if (g_strcmp0 (self->icon_name, old_icon_name) != 0) {
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }
}


/* Update enabled state based on nm's state and wifi dev availability */
static void
update_enabled_state (PhoshWifiManager *self)
{
   gboolean enabled;

   g_return_if_fail (NM_IS_CLIENT (self->nmclient));
   enabled = nm_client_wireless_get_enabled (self->nmclient) && self->present;
   g_debug ("NM wifi enabled: %d, present: %d", enabled, self->present);

   if (enabled != self->enabled) {
     self->enabled = enabled;
     g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
   }
}


static void
update_state (PhoshWifiManager *self)
{
  update_enabled_state (self);
  update_icon_name (self);
}


static void
phosh_wifi_manager_get_property (GObject *object,
                                 guint property_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER (object);

  switch (property_id) {
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_SSID:
    g_value_set_string (value, self->ssid);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
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

  update_state (self);
}


static void
on_nm_device_wifi_active_access_point_changed (PhoshWifiManager *self, GParamSpec *pspec, NMDeviceWifi *dev)
{
  NMAccessPoint *old_ap;
  GBytes *ssid;
  g_autofree char *old_ssid = NULL;

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

  old_ssid = self->ssid;
  self->ssid = NULL;

  if(self->ap) {
    g_signal_connect_swapped (self->ap, "notify::strength",
                              G_CALLBACK (on_nm_access_point_strength_changed), self);
    on_nm_access_point_strength_changed (self, NULL, self->ap);

    ssid = nm_access_point_get_ssid (self->ap);
    self->ssid = nm_utils_ssid_to_utf8 (g_bytes_get_data (ssid, NULL), g_bytes_get_size (ssid));
    g_debug ("Wifi network name was set to '%s'", self->ssid);
  }

  if (g_strcmp0 (self->ssid, old_ssid) != 0) {
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SSID]);
  }
}


static void
check_device (PhoshWifiManager *self)

{
  const GPtrArray *devs;
  NMDevice *dev;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));

  devs = nm_active_connection_get_devices (self->active);
  if (!devs || !devs->len) {
      g_warning("Found active connection but no device");
      return;
  }

  dev = g_ptr_array_index (devs, 0);
  if (NM_IS_DEVICE_WIFI (dev)) {
    g_debug("conn %p uses a wifi device", self->active);

    /* Is this still the same device? */
    if (dev != NM_DEVICE (self->dev)) {
      if (self->dev) {
        g_signal_handlers_disconnect_by_data (self->dev, self);
      }
      g_set_object (&self->dev, NM_DEVICE_WIFI (dev));
      g_signal_connect_swapped (self->dev, "notify::active-access-point",
                                G_CALLBACK (on_nm_device_wifi_active_access_point_changed), self);
      on_nm_device_wifi_active_access_point_changed (self, NULL, self->dev);
    }
  }
}


static void
cleanup_device (PhoshWifiManager *self)
{
  if (self->ap) {
    g_signal_handlers_disconnect_by_data (self->ap, self);
    g_clear_object (&self->ap);
  }

  if (self->dev) {
    g_signal_handlers_disconnect_by_data (self->dev, self);
    g_clear_object (&self->dev);
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

   update_state (self);

   switch (state) {
   case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
     check_device (self);
     break;
   case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
   case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
   case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
   case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
   default:
     cleanup_device (self);
     return;
   }
}


static void
on_nmclient_wireless_enabled_changed (PhoshWifiManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  update_state (self);

  if (self->ssid != NULL) {
    g_clear_pointer (&self->ssid, g_free);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SSID]);
  }
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
  const GPtrArray *conns;
  NMActiveConnection *conn;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  conns = nm_client_get_active_connections(nmclient);

  for (int i = 0; i < conns->len; i++) {
    const char *type;

    conn = g_ptr_array_index (conns, i);
    type = nm_active_connection_get_connection_type (conn);

    /* We only care about wireless connections */
    if (g_strcmp0 (type, NM_SETTING_WIRELESS_SETTING_NAME))
        continue;

    found = TRUE;
    /* Is this still the same connection? */
    if (conn != self->active) {
      g_debug ("New active connection %p", conn);
      cleanup_device (self);
      if (self->active)
        g_signal_handlers_disconnect_by_data (self->active, self);
      g_set_object (&self->active, conn);
      g_signal_connect_swapped (self->active, "state-changed",
                                G_CALLBACK (on_nm_active_connection_state_changed), self);
    }
    check_device (self);
    break;
  }

  /* Clean up if there's no active wifi connection */
  if (!found) {
    if (self->active)
      g_signal_handlers_disconnect_by_data (self->active, self);
    g_clear_object (&self->active);
    cleanup_device (self);
  }
}


static void
on_nmclient_devices_changed (PhoshWifiManager *self, GParamSpec *pspec, NMClient *nmclient)
{
  gboolean have_wifi_dev = FALSE, present;
  const GPtrArray *devs;
  NMDevice *dev;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  present = self->present;

  devs = nm_client_get_devices (nmclient);
  if (!devs || !devs->len) {
    update_state (self);
    self->present = FALSE;
    if (self->present != present)
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
    return;
  }

  for (int i = 0; i < devs->len; i++) {
    dev = g_ptr_array_index (devs, i);
    if (NM_IS_DEVICE_WIFI (dev)) {
      g_debug("Wifi device connected at %d", i);
      have_wifi_dev = TRUE;
      break;
    }
  }

  self->present = have_wifi_dev;
  if (self->present != present)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
  update_state (self);
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer data)
{
  g_autoptr(GError) err = NULL;
  PhoshWifiManager *self;
  NMClient *client;

  client = nm_client_new_finish (res, &err);
  if (client == NULL) {
    g_message ("Failed to init NM: %s", err->message);
    return;
  }

  self = PHOSH_WIFI_MANAGER (data);
  self->nmclient = client;

  g_signal_connect_swapped (self->nmclient, "notify::wireless-enabled",
                            G_CALLBACK (on_nmclient_wireless_enabled_changed), self);
  g_signal_connect_swapped (self->nmclient, "notify::active-connections",
                            G_CALLBACK (on_nmclient_active_connections_changed), self);
  g_signal_connect_swapped (self->nmclient, "notify::devices",
                            G_CALLBACK (on_nmclient_devices_changed), self);

  update_state (self);

  on_nmclient_active_connections_changed (self, NULL, self->nmclient);
  on_nmclient_devices_changed (self, NULL, self->nmclient);

  g_debug("Wifi manager initialized");
}


static void
phosh_wifi_manager_constructed (GObject *object)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER (object);

  self->cancel = g_cancellable_new ();
  nm_client_new_async (self->cancel, on_nm_client_ready, self);

  G_OBJECT_CLASS (phosh_wifi_manager_parent_class)->constructed (object);
}


static void
phosh_wifi_manager_dispose (GObject *object)
{
  PhoshWifiManager *self = PHOSH_WIFI_MANAGER(object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  if (self->ap) {
    g_signal_handlers_disconnect_by_data (self->ap, self);
    g_clear_object (&self->ap);
  }

  if (self->nmclient) {
    g_signal_handlers_disconnect_by_data (self->nmclient, self);
    g_clear_object (&self->nmclient);
  }

  cleanup_device (self);

  if (self->active) {
    g_signal_handlers_disconnect_by_data (self->active, self);
    g_clear_object (&self->active);
  }

  g_clear_pointer (&self->ssid, g_free);

  G_OBJECT_CLASS (phosh_wifi_manager_parent_class)->dispose (object);
}


static void
phosh_wifi_manager_class_init (PhoshWifiManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_wifi_manager_constructed;
  object_class->dispose = phosh_wifi_manager_dispose;

  object_class->get_property = phosh_wifi_manager_get_property;

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The wifi icon name",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SSID] =
    g_param_spec_string ("ssid",
                         "ssid",
                         "The wifis ssid, if connected",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether wifi is enabled and a wifi device is available",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether wifi hardware is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_wifi_manager_init (PhoshWifiManager *self)
{
  self->icon_name = "network-wireless-disabled-symbolic";
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

const char *
phosh_wifi_manager_get_icon_name (PhoshWifiManager *self)
{
  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (self), NULL);

  return self->icon_name;
}


const char *
phosh_wifi_manager_get_ssid (PhoshWifiManager *self)
{
  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (self), NULL);

  return self->ssid;
}

gboolean
phosh_wifi_manager_get_enabled (PhoshWifiManager *self)
{
  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (self), FALSE);

  return self->enabled;
}

void
phosh_wifi_manager_set_enabled (PhoshWifiManager *self, gboolean enabled)
{
  int DEFAULT_TIMEOUT_MSEC = -1;

  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (self));

  if (enabled == self->enabled)
    return;

  g_return_if_fail (NM_IS_CLIENT (self->nmclient));

  nm_client_dbus_set_property (
    self->nmclient, NM_DBUS_PATH, NM_DBUS_INTERFACE,
    "WirelessEnabled", g_variant_new_boolean (enabled),
    DEFAULT_TIMEOUT_MSEC, NULL, NULL, NULL
  );
}


gboolean
phosh_wifi_manager_get_present (PhoshWifiManager *self)
{
  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (self), FALSE);

  return self->present;
}
