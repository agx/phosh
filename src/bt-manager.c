/*
 * Copyright (C) 2020 Purism SPC
 *               2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-bt-manager"

#include "phosh-config.h"

#include "bt-manager.h"
#include "shell.h"
#include "dbus/gsd-rfkill-dbus.h"
#include "util.h"

#include "gtk-list-models/gtkfilterlistmodel.h"
#include "gnome-bluetooth-enum-types.h"
#include "bluetooth-client.h"
#include "bluetooth-device.h"

#define BUS_NAME "org.gnome.SettingsDaemon.Rfkill"
#define OBJECT_PATH "/org/gnome/SettingsDaemon/Rfkill"

/**
 * PhoshBtManager:
 *
 * Tracks the Bluetooth status
 *
 * #PhoshBtManager tracks the Bluetooth status that
 * is whether the adapter is present and enabled.
 */

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_N_DEVICES,
  PROP_N_CONNECTED,
  PROP_INFO,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshBtManager {
  PhoshManager           manager;

  gboolean               enabled;
  gboolean               present;
  const char            *icon_name;
  guint                  n_connected;
  guint                  n_devices;
  char                  *info;

  BluetoothClient       *bt_client;
  GtkFilterListModel    *connectable_devices;

  PhoshRfkillDBusRfkill *proxy;
};
G_DEFINE_TYPE (PhoshBtManager, phosh_bt_manager, PHOSH_TYPE_MANAGER);


static void
on_adapter_setup_mode_changed (PhoshBtManager *self)
{
  gboolean setup_mode;

  g_assert (PHOSH_IS_BT_MANAGER (self));
  g_assert (BLUETOOTH_IS_CLIENT (self->bt_client));

  g_object_get (self->bt_client, "default-adapter-setup-mode", &setup_mode, NULL);

  g_debug ("Setup-mode: %d", setup_mode);
}


static void
on_adapter_state_changed (PhoshBtManager *self)
{
  BluetoothAdapterState state;
  g_autofree char *name = NULL;

  g_assert (PHOSH_IS_BT_MANAGER (self));
  g_assert (BLUETOOTH_IS_CLIENT (self->bt_client));

  g_object_get (self->bt_client, "default-adapter-state", &state, NULL);
  name = g_enum_to_string (BLUETOOTH_TYPE_ADAPTER_STATE, state);

  g_debug ("State: %s", name);
}


static void
phosh_bt_manager_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (object);

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
  case PROP_N_DEVICES:
    g_value_set_uint (value, self->n_devices);
    break;
  case PROP_N_CONNECTED:
    g_value_set_uint (value, self->n_connected);
    break;
  case PROP_INFO:
    g_value_set_string (value, self->info);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_bt_airplane_mode_changed (PhoshBtManager        *self,
                             GParamSpec            *pspec,
                             PhoshRfkillDBusRfkill *proxy)
{
  gboolean enabled;
  const char *icon_name;

  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));
  g_return_if_fail (PHOSH_RFKILL_DBUS_IS_RFKILL (proxy));

  enabled = !phosh_rfkill_dbus_rfkill_get_bluetooth_airplane_mode (proxy) && self->present;

  if (enabled == self->enabled)
    return;

  self->enabled = enabled;

  g_debug ("BT enabled: %d", self->enabled);
  if (enabled)
    icon_name = "bluetooth-active-symbolic";
  else
    icon_name = "bluetooth-disabled-symbolic";

  if (icon_name != self->icon_name) {
    self->icon_name = icon_name;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
on_bt_has_airplane_mode_changed (PhoshBtManager        *self,
                                 GParamSpec            *pspec,
                                 PhoshRfkillDBusRfkill *proxy)
{
  gboolean present;

  present = phosh_rfkill_dbus_rfkill_get_bluetooth_has_airplane_mode (proxy);

  if (present == self->present)
    return;

  /* Having a BT adapter that supports airplane mode seems to be the best
     indicator for having a device at all */
  self->present = present;
  g_debug ("BT present: %d", self->present);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);

  /* Sync up in case `enabled` got flipped first */
  on_bt_airplane_mode_changed (self, NULL, proxy);
}


static void
on_proxy_new_for_bus_finish (GObject        *source_object,
                             GAsyncResult   *res,
                             PhoshBtManager *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));

  self->proxy = phosh_rfkill_dbus_rfkill_proxy_new_for_bus_finish (res, &err);

  if (!self->proxy) {
    phosh_dbus_service_error_warn (err, "Failed to get gsd rfkill proxy");
    goto out;
  }

  g_object_connect (self->proxy,
                    "swapped_object_signal::notify::bluetooth-airplane-mode",
                    G_CALLBACK (on_bt_airplane_mode_changed),
                    self,
                    "swapped_object_signal::notify::bluetooth-has-airplane-mode",
                    G_CALLBACK (on_bt_has_airplane_mode_changed),
                    self,
                    NULL);
  on_bt_airplane_mode_changed (self, NULL, self->proxy);
  on_bt_has_airplane_mode_changed (self, NULL, self->proxy);

  g_debug ("BT manager initialized");
out:
  g_object_unref (self);
}


static void
phosh_bt_manager_idle_init (PhoshManager *manager)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (manager);

  phosh_rfkill_dbus_rfkill_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              BUS_NAME,
                                              OBJECT_PATH,
                                              NULL,
                                              (GAsyncReadyCallback) on_proxy_new_for_bus_finish,
                                              g_object_ref (self));
}


static void
phosh_bt_manager_dispose (GObject *object)
{
  PhoshBtManager *self = PHOSH_BT_MANAGER (object);

  g_clear_object (&self->proxy);
  g_clear_object (&self->connectable_devices);
  g_clear_object (&self->bt_client);

  g_clear_pointer (&self->info, g_free);

  G_OBJECT_CLASS (phosh_bt_manager_parent_class)->dispose (object);
}


static void
phosh_bt_manager_class_init (PhoshBtManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->dispose = phosh_bt_manager_dispose;

  object_class->get_property = phosh_bt_manager_get_property;

  manager_class->idle_init = phosh_bt_manager_idle_init;

  /**
   * PhoshBtManager::icon-name:
   *
   * A icon name that indicates the current Bluetooth status.
   */
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The bt icon name",
                         "bluetooth-disabled-symbolic",
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshBtManager::enabled:
   *
   * Whether a Bluetooth is enabled.
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshBtManager::present:
   *
   * Whether a Bluetooth adapter is present
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshBtManager:n-devices:
   *
   * The number of connectable Bluetooth devices
   */
  props[PROP_N_DEVICES] =
    g_param_spec_uint ("n-devices", "", "",
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshBtManager:n-connected:
   *
   * The number of currently connected Bluetooth devices
   */
  props[PROP_N_CONNECTED] =
    g_param_spec_uint ("n-connected", "", "",
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshBtManager:info:
   *
   * If only a single device is connected this gives details about it.
   */
  props[PROP_INFO] =
    g_param_spec_string ("info", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static gboolean
filter_devices (gpointer item, gpointer data)
{
  gboolean connectable;
  BluetoothDevice *device = BLUETOOTH_DEVICE (item);

  g_object_get (device, "connectable", &connectable, NULL);

  return connectable;
}


static void
refilter_cb (PhoshBtManager *self)
{
  g_assert (PHOSH_IS_BT_MANAGER (self));

  gtk_filter_list_model_refilter (self->connectable_devices);
}


static void
recount_cb (PhoshBtManager *self)
{
  g_autofree char *last_info = NULL;
  guint n_devices, n_connected = 0;

  g_assert (PHOSH_IS_BT_MANAGER (self));

  n_devices = g_list_model_get_n_items (G_LIST_MODEL (self->connectable_devices));
  for (int i = 0; i < n_devices; i++) {
    g_autoptr (BluetoothDevice) device = NULL;
    g_autofree char *info = NULL;
    gboolean connected;

    device = g_list_model_get_item (G_LIST_MODEL (self->connectable_devices), i);
    g_object_get (device, "connected", &connected, "alias", &info, NULL);

    if (connected) {
      n_connected++;
      last_info = g_steal_pointer (&info);
    }
  }

  if (g_strcmp0 (self->info, last_info)) {
    g_debug ("New info: %s", last_info);
    g_free (self->info);
    self->info = g_steal_pointer (&last_info);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INFO]);
  }

  if (self->n_connected != n_connected) {
    g_debug ("%d Bluetooth devices connected", n_connected);
    self->n_connected = n_connected;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_CONNECTED]);
  }

  if (self->n_devices != n_devices) {
    g_debug ("%d Bluetooth devices", n_devices);
    self->n_devices = n_devices;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_DEVICES]);
  }
}


static void
on_device_added (PhoshBtManager *self, BluetoothDevice *device)
{
  g_assert (PHOSH_IS_BT_MANAGER (self));
  g_assert (BLUETOOTH_IS_DEVICE (device));

  g_signal_connect_swapped (device, "notify::connectable", G_CALLBACK (refilter_cb), self);
  g_signal_connect_swapped (device, "notify::connected", G_CALLBACK (recount_cb), self);

  refilter_cb (self);
  recount_cb (self);
}


static void
on_device_removed (PhoshBtManager *self, BluetoothDevice *device)
{
  refilter_cb (self);
  recount_cb (self);
}


static void
setup_devices (PhoshBtManager *self)
{
  g_autoptr (GListStore) devices = NULL;
  guint n_items;

  /* Keep a list of connectable devices */
  devices = bluetooth_client_get_devices (self->bt_client);
  self->connectable_devices = gtk_filter_list_model_new (G_LIST_MODEL (devices),
                                                         filter_devices,
                                                         self,
                                                         NULL);
  g_object_connect (self->bt_client,
                    "swapped-object-signal::device-added", on_device_added, self,
                    "swapped-object-signal::device-removed", on_device_removed, self,
                    NULL);

  /* cold plug existing devices */
  n_items = g_list_model_get_n_items (G_LIST_MODEL (devices));
  for (int i = 0; i < n_items; i++) {
    g_autoptr (BluetoothDevice) device = NULL;

    device = g_list_model_get_item (G_LIST_MODEL (devices), i);
    on_device_added (self, device);
  }

  recount_cb (self);
}


static void
phosh_bt_manager_init (PhoshBtManager *self)
{
  self->icon_name = "bluetooth-disabled-symbolic";

  self->bt_client = bluetooth_client_new ();
  g_object_connect (self->bt_client,
                    "swapped-signal::notify::default-adapter-state",
                    on_adapter_state_changed, self,
                    "swapped-signal::notify::default-adapter-setup-mode",
                    on_adapter_setup_mode_changed, self,
                    NULL);

  setup_devices (self);
}


PhoshBtManager *
phosh_bt_manager_new (void)
{
  return PHOSH_BT_MANAGER (g_object_new (PHOSH_TYPE_BT_MANAGER, NULL));
}


const char *
phosh_bt_manager_get_icon_name (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), NULL);

  return self->icon_name;
}


gboolean
phosh_bt_manager_get_enabled (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), FALSE);

  return self->enabled;
}


void
phosh_bt_manager_set_enabled (PhoshBtManager *self, gboolean enabled)
{
  g_return_if_fail (PHOSH_IS_BT_MANAGER (self));

  if (!self->present)
    return;

  if (enabled == self->enabled)
    return;

  g_return_if_fail (self->proxy);

  self->enabled = enabled;
  phosh_rfkill_dbus_rfkill_set_bluetooth_airplane_mode (self->proxy, !enabled);
}


gboolean
phosh_bt_manager_get_present (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), FALSE);

  return self->present;
}

/**
 * phosh_bt_manager_get_connectable_devices:
 * @self: The Bluetooth manager
 *
 * Gets the currently connectable devices.
 *
 * Returns:(transfer none): The connectable devices
 */
GListModel *
phosh_bt_manager_get_connectable_devices (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), NULL);

  return G_LIST_MODEL (self->connectable_devices);
}


guint
phosh_bt_manager_get_n_connected (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), 0);

  return self->n_connected;
}


const char *
phosh_bt_manager_get_info (PhoshBtManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BT_MANAGER (self), NULL);

  return self->info;
}


static void
on_service_connected (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GError *err = NULL;
  GTask *task = G_TASK (user_data);
  gboolean success;

  success = bluetooth_client_connect_service_finish (BLUETOOTH_CLIENT (source_object), res, &err);
  if (!success)
    g_debug ("Failed to connect: %s", err->message);

  if (!success) {
    g_task_return_error (task, err);
    return;
  }

  g_task_return_boolean (task, success);
}


void
phosh_bt_manager_connect_device_async (PhoshBtManager      *self,
                                       BluetoothDevice     *device,
                                       gboolean             connect,
                                       GAsyncReadyCallback  callback,
                                       GCancellable        *cancellable,
                                       gpointer             user_data)
{
  const char *object_path;
  GTask *task = g_task_new (self, cancellable, callback, user_data);

  object_path = bluetooth_device_get_object_path (device);

  g_debug ("%s device %s", connect ? "Connecting" : "Disconnecting", object_path);
  bluetooth_client_connect_service (self->bt_client,
                                    object_path,
                                    connect,
                                    cancellable,
                                    on_service_connected,
                                    task);
}


gboolean
phosh_bt_manager_connect_device_finish (PhoshBtManager  *self,
                                        GAsyncResult    *result,
                                        GError         **error)
{
  g_autoptr (GTask) task = G_TASK (result);

  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (task, error);
}
